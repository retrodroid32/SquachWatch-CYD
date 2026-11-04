// SquachWatch-CYD — LCDWiki ES3C28P / ES3N28P V1.0
// 2.8" ESP32-S3 N16R8, ILI9341V 240x320, FT6336G capacitive touch.
#pragma once

#define USER_SETUP_INFO "SquachWatch-CYD / LCDWiki ES3C28P / ILI9341V"
#define ILI9341_DRIVER

#define TFT_BL 45
#define TFT_BACKLIGHT_ON HIGH

#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   46
#define TFT_RST  -1

#define SPI_FREQUENCY      40000000
#define SPI_READ_FREQUENCY 20000000

#define LOAD_GLCD
#define LOAD_FONT2
