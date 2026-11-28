// T-Deck Plus / ESP32-S3 16MB, 320x240 ST7789, SPI pins per LILYGO.
// Deliberately isolated from CYD/T-Watch pin and touch configurations.
#pragma once
#define USER_SETUP_INFO "SquachWatch T-Deck Plus ST7789"
#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 38
#define TFT_MOSI 41
#define TFT_SCLK 40
#define TFT_CS 12
#define TFT_DC 11
#define TFT_RST -1
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
