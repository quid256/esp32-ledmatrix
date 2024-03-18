#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "matrix_config.h"


// MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA* dma_display = nullptr;

void setup() {
    dma_display = makePanel(true);
    dma_display->begin();
    dma_display->setBrightness8(15); // 0-255
    dma_display->clearScreen();
}

uint8_t wheelval = 0;
uint8_t x = 20, y = 0;
uint8_t tx = 0, ty = 0;
int8_t dx = 1, dy = 0;
uint8_t rectWidth = 8;

void loop() {
    dma_display->flipDMABuffer(); // Show the back buffer, set currently output buffer to the back (i.e. no longer being sent to LED panels)
    delay(30);
    //x += dx;
    // y += dy;
    if (x == 0) {
        dx = +1;
    }
    // if (y == 0) { dy = +1; }
    if (x + rectWidth == dma_display->width()) {
        dx = -1;
    }
    // if (y + rectWidth == dma_display->height()) { dy = -1; }

    dma_display->clearScreen();
    dma_display->fillRect(0, x, dma_display->height(), rectWidth, dma_display->color444(15, 0, 0));

    tx = dma_display->width() / 2;
    ty = dma_display->height() / 2;
    dma_display->fillTriangle(tx, ty, tx - 20, ty + 20, tx + 20, ty + 20, dma_display->color444(0, 0, 15));
    //   dma_display->fillRect(x, 0, rectWidth, dma_display->width(), dma_display->color444(0, 15, 0));
    //   dma_display->fillRect(x, x, rectWidth, rectWidth, dma_display->color444(0, 0, 15));
}
