#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <TetrisMatrixDraw.h>
#include "matrix_config.h"


#define ANIMATION_TIME 50000

// portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//
// hw_timer_t* displayTimer = NULL;
// hw_timer_t * animationTimer = NULL;

MatrixPanel_I2S_DMA* display = makePanel(false);
TetrisMatrixDraw tetris(*display);

// void IRAM_ATTR display_updater() {
//     // Increment the counter and set the time of ISR
//     portENTER_CRITICAL_ISR(&timerMux);
//     display->flipDMABuffer(); // Show the back buffer, set currently output buffer to the back (i.e. no longer being sent to LED panels)
//     portEXIT_CRITICAL_ISR(&timerMux);
// }
//   
// void animationHandler()
// {
//     portENTER_CRITICAL_ISR(&timerMux);
//     display->clearScreen();
//     tetris.drawText(5, 20);
//     portEXIT_CRITICAL_ISR(&timerMux);
// }

void setup() {
    display->begin();
    display->setBrightness8(15); // 0-255
    display->clearScreen();

    tetris.scale = 2;
    tetris.setTime("12:34");
}

int i = 0;
void loop() {
    if (i == 5) {
        display->clearScreen();
        tetris.drawNumbers(-5, 40);
        // display->fillRect(0, x, dma_display->height(), rectWidth, dma_display->color444(15, 0, 0));
        i = 0;
    }
    i++;
    // display->flipDMABuffer();

    delay(30);
}
