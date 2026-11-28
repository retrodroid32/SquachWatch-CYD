# SquachWatch — LILYGO T-Deck Plus experimental firmware

**Status: R1 hardware and Wi-Fi signature bring-up. Not full feature parity.**

The T-Deck Plus is an ESP32-S3, not a classic ESP32 CYD. Never flash a CYD firmware image onto it. Its display, GT911 touch, I2C keyboard, trackball, GPS, SD, and optional SX1262 need dedicated drivers.

## Build and upload
- PlatformIO environment: tdeck-plus.
- Command: pio run -e tdeck-plus -t upload
- Serial log: pio device monitor -b 115200
- PR Actions artifact: lab-pr-258-tdeck-plus (actual PR number varies).
- Board: ESP32-S3 16MB flash / 8MB OPI PSRAM, dual 4MB OTA partition table.
- Upload the full matching bootloader, partition table and firmware from the T-Deck Plus artifact, never the CYD/ESP32 images. Firmware offset 0x10000; partition table 0x8000; bootloader offset must match the ESP32-S3 build flash command.

If USB download fails, hold the trackball center (BOOT) while attaching USB, release it, and reset after flashing.

## R1 functions
- Board power GPIO10, ST7789 320x240, GPIO42 pulse-based backlight.
- SPI MOSI41/MISO38/SCLK40, LCD CS12/DC11; SD CS39 and radio CS9 held deselected.
- GT911 touch probe 0x5D/0x14, shared I2C SDA18/SCL8.
- Trackball directions, basic I2C keyboard event read 0x55.
- Onboard GPS UART 9600, ESP32 RX44/TX43 (upstream T-Deck UnitTest utilities.h / GPSShield): NMEA status display.
- Wi-Fi scans and detection matches using the fork's full v1.50 signature table.
- Hardware status screen shows memory, PSRAM, GT911, and last keyboard character.

## Explicit limitations
This is an experimental board bring-up firmware, **not the complete SquachWatch UI**. BLE detection, LoRa mesh, SD logging, full Squachy UI, Watch/Hunt, alerts, keyboard text editing and OTA are NOT ported in R1. Do not expose this image as a production flasher choice.

## Hardware acceptance
1. Three cold boots without reset loops, screen working and backlight visible.
2. Touch bottom left/right, all trackball directions, keyboard input.
3. Wi-Fi scan and actual matched OUI/SSID devices.
4. GPS reports real NMEA sentences in open sky; if silent check module baud and variant against official schematic.
5. Verify 8MB PSRAM, heap and 5-minute idle soak.
6. Then implement SD/LoRa shared-SPI arbitration and BLE/full UI in follow-on PRs.

Reference: Official LILYGO T-Deck Plus Quick Start and LilyGoLib hardware pin map.

## Official upstream cross-check (2026-11-24)
LILYGO T-Deck repo UnitTest/utilities.h: POWERON10, TFT CS12/DC11, SPI 41/38/40, GT911 INT16 and I2C 18/8, keyboard address 0x55, keyboard INT46, backlight42, GPS RX44/TX43. UnitTest::setBrightness(16) uses GPIO42 LOW for 3ms, then HIGH for 30us to select maximum brightness. GPSShield starts at 9600 baud with optional detection/recovery; other documentation examples show different GPS UART mappings (21/48), so the real onboard GNSS UART must be confirmed on hardware. The R1 port follows upstream UnitTest and GPSShield pin definitions, not audio codec pins 21/48.

## CI isolation repair
Build #1097 compiled the T-Deck S3 image successfully (776397 firmware bytes), but other board builds also included the new standalone sketch. R2 wraps the standalone entry points with TDECK_PLUS so classic CYD, T-Watch and Freenove keep their previous main.cpp without duplicate setup()/loop(). No changes to those hardware paths.
