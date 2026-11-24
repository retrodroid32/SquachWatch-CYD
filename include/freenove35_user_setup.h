// TFT_eSPI is used only as an offscreen RGB565 sprite painter in this
// ST77922-QSPI target. Actual panel bus is owned by esp-idf spi_master
// in src/freenove35_main.cpp. Never call TFT_eSPI::init() on this board.
#pragma once
#define USER_SETUP_INFO "Freenove FNK0104N ST77922 320x480 QSPI sprite-only"
#define ILI9341_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 480
#define TFT_MISO -1
#define TFT_MOSI -1
#define TFT_SCLK -1
#define TFT_CS -1
#define TFT_DC -1
#define TFT_RST -1
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
