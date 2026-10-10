// SquachWatch-CYD — TFT_eSPI setup for Sunton ESP32-3248S035C
// 3.5" 320x480 ST7796 with directly wired GT911 capacitive touch.
#pragma once

#define USER_SETUP_INFO "SquachWatch-CYD / ESP32-3248S035C / ST7796 / GT911"

#define ST7796_DRIVER
#define TFT_WIDTH   320
#define TFT_HEIGHT  480

#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    27
#define TFT_BACKLIGHT_ON HIGH

// GT911: intentionally no TOUCH_CS. Touch is I2C, not XPT2046 SPI.
#define PIN_I2C_SDA    33
#define PIN_I2C_SCL    32
#define PIN_TOUCH_RST  25
#define PIN_TOUCH_INT  21

#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY       40000000
#endif
#define SPI_READ_FREQUENCY  20000000

#define LOAD_GLCD
#define LOAD_FONT2

#define TFT_RGB_ORDER TFT_BGR
