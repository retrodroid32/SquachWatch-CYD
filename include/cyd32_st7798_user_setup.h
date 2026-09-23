// SquachWatch-CYD — TFT_eSPI user setup for the 3.2" CYD family
// ESP32-2432S032R / E32R32P, 240x320, resistive XPT2046 touch.
#pragma once

#define USER_SETUP_INFO "SquachWatch-CYD / 3.2 inch / ST7798-ST7789 compatible"
#define ST7789_DRIVER
#define TFT_WIDTH   240
#define TFT_HEIGHT  320
#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    27
#define TOUCH_CS  33
#define TFT_BACKLIGHT_ON HIGH
#define USE_HSPI_PORT
#define PWM_FREQ           5000
#define PWM_MAX_DUTY        255
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY       40000000
#endif
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF
#define LOAD_GLCD
#define LOAD_FONT2
