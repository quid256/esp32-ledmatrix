#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define R1 6
#define G1 5
#define BL1 9
#define R2 11
#define G2 10
#define BL2 12
#define CH_A 8
#define CH_B 14
#define CH_C 15
#define CH_D 16
#define CH_E 17
#define CLK 13
#define LAT 38
#define OE 39

// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64 // Panel height of 64 will required CH_E to be defined.
#define PANELS_NUMBER 1 // Number of chained panels, if just a single panel, obviously set to 1

#define PANE_WIDTH PANEL_WIDTH * PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT
#define NUM_LEDS PANE_WIDTH * PANE_HEIGHT

// Construct the MatrixPanel Object
MatrixPanel_I2S_DMA* makePanel(bool double_buffer) {
    HUB75_I2S_CFG::i2s_pins _pins = {
        R1, G1, BL1,
        R2, G2, BL2,
        CH_A, CH_B, CH_C, CH_D, CH_E,
        LAT, OE, CLK,
    };
    HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER, _pins);

    mxconfig.gpio.e = CH_E;
    mxconfig.driver = HUB75_I2S_CFG::FM6126A; // for panels using FM6126A chips
    mxconfig.double_buff = double_buffer; // <------------- Turn on double buffer
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;
    // mxconfig.clkphase = false;
    return new MatrixPanel_I2S_DMA(mxconfig);
}
