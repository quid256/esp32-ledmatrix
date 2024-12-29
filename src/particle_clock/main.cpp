#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "matrix_config.h"

#include <WiFi.h>
#include <cstring>
#include "secrets.h"

extern const float data_start[] asm("_binary_mockups_particle_clock_data_bin_start");
extern const float data_end[] asm("_binary_mockups_particle_clock_data_bin_end");

const bool MILITARY_TIME = false;

const uint8_t DIGIT_WIDTH = 12;
const uint8_t DIGIT_HEIGHT = 20;

// The display object
MatrixPanel_I2S_DMA* display = makePanel(true);

const uint16_t color_active = display->color565(133, 255, 145);
// const uint16_t color_semiactive = display->color565(116, 153, 120);
// const uint16_t color_inactive = display->color565(50, 50, 50);
const uint16_t black = display->color565(0, 0, 0);

/// Interpolate between two colors
uint16_t interpolate_color(double amnt, uint16_t start, uint16_t end) {
    uint8_t rs, gs, bs, re, ge, be;
    display->color565to888(start, rs, gs, bs);
    display->color565to888(end, re, ge, be);

    return display->color565(
        static_cast<uint8_t>((1 - amnt) * rs + amnt * re),
        static_cast<uint8_t>((1 - amnt) * gs + amnt * ge),
        static_cast<uint8_t>((1 - amnt) * bs + amnt * be)
    );
}

const float SPRING_CONST = 1.0f;
const float DECAY = 0.8f;
const float INIT_VELOCITY = 7.0f;
const float MASS = 1.0f;

// void print_clock() {
//     display->clearScreen();
//     for (size_t i = 0; i < 4; i++) {
//         digits[i].print(display);
//     }
//     display->fillRect(31, 27, 3, 3, color_semiactive);
//     display->fillRect(31, 33, 3, 3, color_semiactive);
//     display->flipDMABuffer();
// }

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

//
//
//
//

const size_t SCREEN_SIZE = 64;
const size_t NUM_CELLS = SCREEN_SIZE * SCREEN_SIZE;

class Buffer {
  public:
    float cells[SCREEN_SIZE][SCREEN_SIZE];

    Buffer() { clear(); }

    void clear() {
        for (size_t j = 0; j < SCREEN_SIZE; j++) {
            for (size_t i = 0; i < SCREEN_SIZE; i++) {
                cells[i][j] = 0.0;
            }
        }
    }

    void draw() {
        for (size_t j = 0; j < SCREEN_SIZE; j++) {
            for (size_t i = 0; i < SCREEN_SIZE; i++) {
                uint16_t col = interpolate_color(cells[i][j], black, color_active);
                display->drawPixel(i, j, col);
            }
        }
    }

    void copyFrom(Buffer* other) { memcpy(cells, other->cells, sizeof(cells)); }

    void overlay(Buffer* other) {
        for (size_t j = 0; j < SCREEN_SIZE; j++) {
            for (size_t i = 0; i < SCREEN_SIZE; i++) {
                cells[i][j] = min(cells[i][j] + other->cells[i][j], 1.0f);
            }
        }
    }

    void drawDigit(size_t x, size_t y, uint8_t d) {
        for (size_t j = 0; j < DIGIT_HEIGHT; j++) {
            for (size_t i = 0; i < DIGIT_WIDTH; i++) {
                size_t offset = (DIGIT_WIDTH * DIGIT_HEIGHT) * d + (i + j * DIGIT_WIDTH);
                cells[i + x][j + y] = data_start[offset];
            }
        }
    }
};

template <size_t S> struct MultiTimer {
    unsigned long curTime;

    unsigned long groupTimes[S] = {0L};

  public:
    MultiTimer() { curTime = millis(); }

    void checkpoint(size_t groupNum) {
        assert(groupNum < S);

        unsigned long newTime = millis();
        groupTimes[groupNum] += newTime - curTime;
        curTime = newTime;
    }

    void print(const char* name) {
        for (size_t i = 0; i < S; i++) {
            Serial.printf("%s[%d]: %ld\n", name, i, groupTimes[i]);
        }
    }
};

class InterpolatedBuffer {

    struct ColoredPoint {
        float x, y;
        float color;
        float score;
    };

    ColoredPoint from[NUM_CELLS];
    ColoredPoint to[NUM_CELLS];

    size_t first;

    struct {
        float x, y;
    } velocities[NUM_CELLS];

    float energies[NUM_CELLS];

    inline void swap(ColoredPoint& a, ColoredPoint& b) {
        ColoredPoint tmp = a;
        a = b;
        b = tmp;
    }

    void siftDown(ColoredPoint pointArray[NUM_CELLS], size_t root, size_t end_index) {
        while (2 * root + 1 < end_index) {
            size_t child_index = 2 * root + 1;
            if (child_index + 1 < end_index &&
                pointArray[child_index].score < pointArray[child_index + 1].score) {
                child_index++;
            }

            if (pointArray[root].score < pointArray[child_index].score) {
                swap(pointArray[root], pointArray[child_index]);
                root = child_index;
            } else {
                return;
            }
        }
    }

    void heapify(ColoredPoint pointArray[NUM_CELLS]) {
        size_t i = 0;
        for (size_t start = (NUM_CELLS - 2) / 2 + 1; start > 0;) {
            start--;
            siftDown(pointArray, start, NUM_CELLS);
        }
    }

    void sortPointArray(ColoredPoint pointArray[NUM_CELLS]) {
        // Heapsort
        heapify(pointArray);
        for (size_t end_index = NUM_CELLS; end_index > 0;) {
            end_index--;
            swap(pointArray[0], pointArray[end_index]);
            siftDown(pointArray, 0, end_index + 1);
        }
    }

  public:
    Buffer* buf = NULL;

    void setup(Buffer* start, Buffer* end) {
        MultiTimer<4> timer;

        // Set up + sort list of pixels
        size_t first_nonzero_from = 0;
        size_t first_nonzero_to = 0;

        for (size_t j = 0; j < SCREEN_SIZE; j++) {
            for (size_t i = 0; i < SCREEN_SIZE; i++) {
                size_t k = i + SCREEN_SIZE * j;
                float score = start->cells[i][j] + 0.01;
                score *= exp(random(-100, 100) / 1000.0);
                from[k] = {(float)i, (float)j, start->cells[i][j], score};

                if (score == 0)
                    first_nonzero_from++;

                score = end->cells[i][j] + 0.01;
                score *= exp(random(-100, 100) / 1000.0);
                to[k] = {(float)i, (float)j, end->cells[i][j], score};
                if (score == 0)
                    first_nonzero_to++;
            }
        }

        timer.checkpoint(0);

        // Sort arrays
        sortPointArray(from);
        timer.checkpoint(1);
        sortPointArray(to);

        timer.checkpoint(2);

        first = min(first_nonzero_to, first_nonzero_from);

        for (size_t k = first; k < NUM_CELLS; k++) {
            // Box-Muller to generate Normal initial velocity
            float angle = random(0, 100) / 100.0 * TWO_PI;
            float norm = INIT_VELOCITY * sqrt(-2.0 * log(random(1, 100) / 100.0));

            // float dirNorm = sqrt(pow(from[k].x - 32.0f, 2) + pow(from[k].y - 32.0f,
            // 2)); float dirX = (from[k].y - 32.0f) / dirNorm; float dirY = (from[k].x
            // - 32.0f) / dirNorm;

            velocities[k] = {norm * cos(angle), norm * sin(angle)};
            // velocities[k] = {norm * dirX, norm * dirY};

            // Compute Hamiltonian at the start of the interpolation
            energies[k] = energy(k);
        }
        timer.checkpoint(3);
        timer.print("fullsort");
    }

    float energy(size_t k) {
        float dx = to[k].x - from[k].x;
        float dy = to[k].y - from[k].y;
        float potential = 0.5f * SPRING_CONST * (dx * dx + dy * dy);
        float kinetic =
            0.5f * MASS *
            (velocities[k].x * velocities[k].x + velocities[k].y * velocities[k].y);

        return potential + kinetic;
    }

    void step(float dt) {
        for (size_t i = first; i < NUM_CELLS; i++) {
            // if (round(from[i].x) == round(to[i].x) &&
            //     round(from[i].y) == round(to[i].y)) {
            //     from[i].x = to[i].x;
            //     from[i].y = to[i].y;
            //     continue;
            // }

            from[i].x += velocities[i].x * dt;
            from[i].y += velocities[i].y * dt;

            float dx = to[i].x - from[i].x;
            float dy = to[i].y - from[i].y;
            float norm = constrain(pow(dx * dx + dy * dy, 0.5f), 0.25f, 5.0f) / 5.0f;
            dx /= norm;
            dy /= norm;

            velocities[i].x *= DECAY;
            velocities[i].y *= DECAY;
            // velocities[i].x += SPRING_CONST / MASS * dx / norm * dt;
            // velocities[i].y += SPRING_CONST / MASS * dy / norm * dt;
            velocities[i].x += SPRING_CONST / MASS * dx * dt;
            velocities[i].y += SPRING_CONST / MASS * dy * dt;

            // Clip velocity
            // float velnorm = sqrt(
            //     velocities[i].x * velocities[i].x + velocities[i].y * velocities[i].y
            // );
            // velocities[i].x *= constrain(velnorm, 0, INIT_VELOCITY * 2) / velnorm;
            // velocities[i].y *= constrain(velnorm, 0, INIT_VELOCITY * 2) / velnorm;
        }
    }

    void renderToOutputBuf() {
        if (buf == NULL) {
            buf = new Buffer();
        } else {
            buf->clear();
        }
        for (size_t i = first; i < NUM_CELLS; i++) {
            float x = from[i].x;
            float y = from[i].y;

            float colorInterp = constrain(energy(i) / energies[i], 0, 1);
            float col = colorInterp * from[i].color + (1 - colorInterp) * to[i].color;
            // float col = from[i].color;

            for (int offsetX = 0; offsetX < 2; offsetX++) {
                for (int offsetY = 0; offsetY < 2; offsetY++) {
                    int32_t ix = (int32_t)floor(x) + offsetX;
                    int32_t iy = (int32_t)floor(y) + offsetY;

                    if (ix >= 0 && iy >= 0 && ix < SCREEN_SIZE && iy < SCREEN_SIZE) {
                        float w = (1 - abs(x - ix)) * (1 - abs(y - iy));
                        buf->cells[ix][iy] += w * col;
                        buf->cells[ix][iy] = min(buf->cells[ix][iy], 1.0f);
                    }
                }
            }
            //
            // int32_t ix = (int32_t)floor(x + 0.5);
            // int32_t iy = (int32_t)floor(y + 0.5);
            //
            // if (ix >= 0 && iy >= 0 && ix < SCREEN_SIZE && iy < SCREEN_SIZE) {
            //     float w = (1 - abs(x - ix)) * (1 - abs(y - iy));
            //     buf->cells[ix][iy] += col;
            //     buf->cells[ix][iy] = min(buf->cells[ix][iy], 1.0f);
            // }
        }
    }
};

Buffer* makeRandomNumberBuffer() {
    Buffer* buf = new Buffer();
    static struct {
        size_t x, y;
    } offsets[4] = {
        {5, 20},
        {20, 20},
        {35, 20},
        {50, 20},
    };

    for (size_t oi = 0; oi < 4; oi++) {
        uint8_t d = random(10);
        for (size_t j = 0; j < DIGIT_HEIGHT; j++) {
            for (size_t i = 0; i < DIGIT_WIDTH; i++) {
                size_t offset = (DIGIT_WIDTH * DIGIT_HEIGHT) * d + (i + j * DIGIT_WIDTH);
                buf->cells[i + offsets[oi].x][j + offsets[oi].y] = data_start[offset];
            }
        }
    }

    return buf;
}

uint8_t cur_digits[4] = {0};

class Clock {
    Buffer* same = new Buffer();
    Buffer* different_from = new Buffer();
    Buffer* different_to = new Buffer();
    Buffer* composite = new Buffer();
    InterpolatedBuffer* interp = new InterpolatedBuffer();

    uint8_t cur_digits[4] = {0};

    struct tm currentTime;

  public:
    void draw() {
        composite->clear();

        composite->overlay(same);

        interp->renderToOutputBuf();
        composite->overlay(interp->buf);

        composite->draw();
    }

    void setTarget(uint8_t new_digits[4]) {
        same->clear();
        different_from->clear();
        different_to->clear();

        int32_t cur_positions[4];
        if (cur_digits[0] == 0) {
            cur_positions[0] = -1;
            cur_positions[1] = 10;
            cur_positions[2] = 28;
            cur_positions[3] = 42;
        } else {
            cur_positions[0] = 3;
            cur_positions[1] = 17;
            cur_positions[2] = 35;
            cur_positions[3] = 49;
        }

        int32_t new_positions[4];
        if (new_digits[0] == 0) {
            new_positions[0] = -1;
            new_positions[1] = 10;
            new_positions[2] = 28;
            new_positions[3] = 42;
        } else {
            new_positions[0] = 3;
            new_positions[1] = 17;
            new_positions[2] = 35;
            new_positions[3] = 49;
        }

        if (cur_digits[0] != new_digits[0] || cur_digits[1] != new_digits[1]) {
            if (cur_positions[0] > 0) {
                different_from->drawDigit(cur_positions[0], 20, cur_digits[0]);
            }
            different_from->drawDigit(cur_positions[1], 20, cur_digits[1]);
            different_from->drawDigit(cur_positions[2], 20, cur_digits[2]);
            different_from->drawDigit(cur_positions[3], 20, cur_digits[3]);
            if (new_positions[0] > 0) {
                different_to->drawDigit(new_positions[0], 20, new_digits[0]);
            }
            different_to->drawDigit(new_positions[1], 20, new_digits[1]);
            different_to->drawDigit(new_positions[2], 20, new_digits[2]);
            different_to->drawDigit(new_positions[3], 20, new_digits[3]);
        } else if (cur_digits[2] != new_digits[2]) {
            if (cur_positions[0] > 0) {
                same->drawDigit(cur_positions[0], 20, cur_digits[0]);
            }
            same->drawDigit(cur_positions[1], 20, cur_digits[1]);

            different_from->drawDigit(cur_positions[2], 20, cur_digits[2]);
            different_from->drawDigit(cur_positions[3], 20, cur_digits[3]);
            different_to->drawDigit(new_positions[2], 20, new_digits[2]);
            different_to->drawDigit(new_positions[3], 20, new_digits[3]);
        } else if (cur_digits[3] != new_digits[3]) {
            if (cur_positions[0] > 0) {
                same->drawDigit(cur_positions[0], 20, cur_digits[0]);
            }
            same->drawDigit(cur_positions[1], 20, cur_digits[1]);
            same->drawDigit(cur_positions[2], 20, cur_digits[2]);
            different_from->drawDigit(cur_positions[3], 20, cur_digits[3]);
            different_to->drawDigit(new_positions[3], 20, new_digits[3]);
        }
        interp->setup(different_from, different_to);
        memcpy(cur_digits, new_digits, sizeof(cur_digits));
    }

    bool isDirty() {
        struct tm timeInfo;
        getLocalTime(&timeInfo, 15);
        return currentTime.tm_min != timeInfo.tm_min;
    }

    void setTargetBasedOnTime() {
        struct tm timeinfo;
        for (size_t i = 0; !getLocalTime(&timeinfo, 15); i++) {
            if (i == 10) {
                return;
            }
            Serial.println("Failed to obtain time");
            delay(1000);
        }

        uint8_t hour = timeinfo.tm_hour;
        if (!MILITARY_TIME) {
            hour %= 12;
            if (hour == 0) {
                hour = 12;
            }
        }
        uint8_t min = timeinfo.tm_min;

        uint8_t targets[4] = {
            static_cast<uint8_t>(hour / 10),
            static_cast<uint8_t>(hour % 10),
            static_cast<uint8_t>(min / 10),
            static_cast<uint8_t>(min % 10),
        };
        setTarget(targets);
        getLocalTime(&currentTime, 15);
    }

    void step(float dt) { interp->step(dt); }
};

Clock cl;

void setup() {
    // Activate Serial
    Serial.begin(115200);
    while (!Serial)
        ;

    // delay(3000);
    // Serial.println("starting");

    // Init display
    display->begin();
    display->setBrightness8(15); // 0-255

    display->clearScreen();
    display->fillRect(27, 27, 10, 10, display->color565(200, 0, 0));
    display->flipDMABuffer();

    // Connect to WiFi
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("connecting to wifi...");
    }
    Serial.println(WiFi.localIP());
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    display->clearScreen();
    display->fillRect(27, 27, 10, 10, display->color565(0, 200, 0));
    display->flipDMABuffer();

    cl.setTargetBasedOnTime();
    cl.draw();
    display->flipDMABuffer();

    // // Load the variant data in to `variants`
    // const uint8_t* step_data = data_start;
    // for (size_t i = 0; i < 10; i++) {
    //     variants[i] = step_data;
    //     step_data += 1 + (*step_data) * 6;
    // }
    //
    // // initialize the digit objects
    // for (size_t i = 0; i < 4; i++) {
    //     digits[i].step_data_start = step_data;
    //     digits[i].init();
    // }
    // printTime(2);
}

// size_t ind = 0;
void loop() {
    if (cl.isDirty()) {
        cl.setTargetBasedOnTime();
    } else {
        cl.step(0.1);
    }
    cl.draw();
    display->flipDMABuffer();

    // if (ind >= 150) {
    //     cur_num = (cur_num + 1) % 10;
    //     interp->setup(interp->buf, number_buffers[(cur_num + 1) % 10]);
    //     // interp->setup(number_buffers[cur_num], number_buffers[(cur_num + 1) %
    //     // 10]);
    //     ind = 0;
    // }
    //
    // interp->step(0.1);
    // interp->draw();
    // display->flipDMABuffer();
    // ind += 1;
    //
    delay(10);
}
