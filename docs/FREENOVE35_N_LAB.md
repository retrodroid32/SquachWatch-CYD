# Freenove FNK0104N 3.5-inch ST77922 capacitive — LAB R1

**Hardware:** Freenove FNK0104N 3.5-inch 320 x 480 ST77922 QSPI display; ESP32-S3 N16R8, 16 MB flash, 8 MB OPI PSRAM; display CS10 CLK12 D0=11 D1=13 D2=14 D3=9, BL41; touch integrated ST77922 I2C 0x55 SDA38 SCL39, INT47 and RESET48.

**IMPORTANT:** This is **NOT** Freenove 2.8-inch FNK0104B (ILI9341/FT6336), and not Freenove 4.0 FNK0104S (ST7796). Do not flash those images to this board.

**R1 features:** a real ST77922 QSPI panel initialization and RGB565 framebuffer transfer; capacitive touch; USB status; PSRAM/free heap; battery ADC GPIO8 diagnostics; Wi-Fi async signature scanning from the same signatures.cpp as SquachWatch v1.50; three touchscreen-selectable informational pages.

**R1 limitations:** Hardware bring-up firmware, **not** complete SquachWatch UI. BLE scan/parse, Watch/Hunt, alerts, SDMMC logging, costumes/background animations, SquachMesh and OTA still require a port. Capacitive rotations and actual hardware cold-boot remain unverified.

**Flash:** Build PlatformIO environment `freenove35-st77922` (ESP32-S3 16 MB). For full USB install use the matching bootloader at 0x0, partition table at 0x8000, otadata at 0xE000, firmware at 0x10000. A separate PR Actions artifact and a hidden LAB web-flasher manifest are provided. Never install the classic ESP32 firmware.

**Hardware verification:** 3 cold boots; display status page visible with stable brightness; 8 MB PSRAM; touch responds to left/right bottom page selection; no QSPI errors; Wi-Fi scan reports detected access points on serial; time running 5 minutes without resets. Test reset using the physical RESET button so earlier screen initialization cannot mask the true cold-boot behavior.

**Display caveat:** The ST77922 uses QSPI framing (command opcode 0x02 and color opcode 0x32 with 0x3C continuation) rather than TFT_eSPI's ordinary SPI display transfer. The panel's technical init parameters are based on the Freenove/LCDwiki hardware examples. Reference: Freenove_ESP32_S3_Display and the ES3C35P ST77922 board notes.

No production release/tag/merge permitted without owner approval.
