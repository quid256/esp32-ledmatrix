#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "matrix_config.h"

// MatrixPanel_I2S_DMA dma_display;
MatrixPanel_I2S_DMA* dma_display = nullptr;

void setup() {
    dma_display = makePanel(false);
    dma_display->begin();
    dma_display->setBrightness8(90); // 0-255
    dma_display->setLatBlanking(4);
    dma_display->clearScreen();
}

uint8_t wheelval = 0;
uint8_t x = 20, y = 0;
int8_t dx = 1, dy = 0;
uint8_t rectWidth = 8;

void loop() {
    //   drawText(wheelval);
    //   wheelval +=1;
    //   delay(20);
    // dma_display->flipDMABuffer(); // Show the back buffer, set currently output buffer to the back (i.e. no longer being sent to LED panels)
    delay(15);
    x += dx;
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
    dma_display->fillRect(x, 0, rectWidth, dma_display->width(), dma_display->color444(0, 15, 0));
    dma_display->fillRect(x, x, rectWidth, rectWidth, dma_display->color444(15, 15, 0));
}
