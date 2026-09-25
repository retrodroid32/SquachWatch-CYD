// SquachWatch-CYD — TFT_eSPI setup for the Freenove 3.2" CYD
// (FNK0103L / FNK0114L, ST7789, resistive XPT2046).
//
// Pins follow Freenove's own FNK0114L 3.2" ST7789 setup. The display and
// XPT2046 share HSPI, matching the RL Phantom resistive bus topology.
#pragma once

#define USER_SETUP_INFO "SquachWatch-CYD / Freenove 3.2 inch resistive / ST7789"

#define ST7789_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define USE_HSPI_PORT
#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1

#define TFT_BL    27
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS  33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT

// Freenove's own setup drives the ST7789 at 80 MHz. Touch transactions remain
// independently limited to 2.5 MHz by TFT_eSPI.
#define SPI_FREQUENCY         80000000
#define SPI_READ_FREQUENCY    20000000
#define SPI_TOUCH_FREQUENCY    2500000

#define TFT_INVERSION_ON
#define TFT_RGB_ORDER TFT_BGR
