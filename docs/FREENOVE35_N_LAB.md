## R5 — upstream SquachWatch ST77922 driver replacement

The R1-R4 custom QSPI controller produced a static scrambled display in real hardware testing. **Replaced the custom SPI transaction layer with the exact upstream SquachWatch `src/st77922_qspi.cpp` and `src/st77922_touch.cpp` drivers**, sourced from `skizzophrenic/SquachWatch-CYD` (FNK0104N). The Freenove entry point now calls `St77922::begin()`, `St77922Touch::begin()`, `St77922::reinit()`, and streams native 320x480 RGB565 sprite pixels via `St77922::setWindow()` and `St77922::queuePixels()`. The original SquachWatch fork's assets, animation, radio scanner and screen widgets are unchanged.

The working upstream driver explicitly sends single-lane 0x2C RAMWR before 0x3C QSPI continuation pixels; the earlier experimental driver sent mismatched commands. It also uses the real board's SPI2 IOMUX pin mapping. Limit first hardware trial to a conservative 40 MHz clock via `SQW_QSPI_HZ`. Verify a live moving Squachy screen and screenshot/frame integrity at boot. RGB byte order and touch input still require real device verification.

# Freenove FNK0104N 3.5-inch — SquachWatch native UI R2

**R2 changes the firmware's primary screen to the real SquachWatch interface.** The old R1 hardware diagnostic dashboard was not the requested product and is replaced.

R2 calls SquachWatch's existing uiBootTick(), uiClearTick(), Squachy animation and backgrounds, uiSettingsTick(), and uiLogTick() directly against a 320x480 16-bit PSRAM framebuffer, which is sent to the Freenove ST77922 using its QSPI bus. The real DetectionEngine runs BLE and Wi-Fi scanning; the classic CYD SPI SD mount is deliberately disabled on FNK0104N because its wiring differs.

Current navigation: native boot splash -> main screen, mascot taps, left/right edge background selection, settings gear, supported appearance/background settings, LOG, and the original SCAN picker plus BLE and Wi-Fi raw-scan pages. Settings rows requiring special hardware or complex subflows explicitly signal that they are not ported yet rather than silently pretending to work. Watch/Hunt, full raw-scan picker, SD logging, mesh and complete settings actions remain hardware-specific integration tasks. R2's aim is original SquachWatch look and live scanning; it is not yet complete functional parity.

Panel/touch: ST77922 QSPI 320x480, I2C 0x55; native portrait only until on-device rotation verified. Verify that screen, mascot art, bubble, outfits, backgrounds, bottom button bar, settings rows and LOG match SquachWatch, and check scan and touch logs on USB at 115200.

# Freenove FNK0104N 3.5-inch ST77922 capacitive — LAB R1

**Hardware:** Freenove FNK0104N 3.5-inch 320 x 480 ST77922 QSPI display; ESP32-S3 N16R8, 16 MB flash, 8 MB OPI PSRAM; display CS10 CLK12 D0=11 D1=13 D2=14 D3=9, BL41; touch integrated ST77922 I2C 0x55 SDA38 SCL39, INT47 and RESET48.

**IMPORTANT:** This is **NOT** Freenove 2.8-inch FNK0104B (ILI9341/FT6336), and not Freenove 4.0 FNK0104S (ST7796). Do not flash those images to this board.

**R1 features:** a real ST77922 QSPI panel initialization and RGB565 framebuffer transfer; capacitive touch; USB status; PSRAM/free heap; battery ADC GPIO8 diagnostics; Wi-Fi async signature scanning from the same signatures.cpp as SquachWatch v1.50; three touchscreen-selectable informational pages.

**R1 limitations:** Hardware bring-up firmware, **not** complete SquachWatch UI. BLE scan/parse, Watch/Hunt, alerts, SDMMC logging, costumes/background animations, SquachMesh and OTA still require a port. Capacitive rotations and actual hardware cold-boot remain unverified.

**Flash:** Build PlatformIO environment `freenove35-st77922` (ESP32-S3 16 MB). For full USB install use the matching bootloader at 0x0, partition table at 0x8000, otadata at 0xE000, firmware at 0x10000. A separate PR Actions artifact and a hidden LAB web-flasher manifest are provided. Never install the classic ESP32 firmware.

**Hardware verification:** 3 cold boots; display status page visible with stable brightness; 8 MB PSRAM; touch responds to left/right bottom page selection; no QSPI errors; Wi-Fi scan reports detected access points on serial; time running 5 minutes without resets. Test reset using the physical RESET button so earlier screen initialization cannot mask the true cold-boot behavior.

**Display caveat:** The ST77922 uses QSPI framing (command opcode 0x02 and color opcode 0x32 with 0x3C continuation) rather than TFT_eSPI's ordinary SPI display transfer. The panel's technical init parameters are based on the Freenove/LCDwiki hardware examples. Reference: Freenove_ESP32_S3_Display and the ES3C35P ST77922 board notes.

No production release/tag/merge permitted without owner approval.

## Complete fork parity remains the acceptance criterion
The Freenove must not be considered complete merely because its splash/mascot matches the CYD. Port exactly the existing fork's behavior for Squachy costumes and backgrounds, all navigation/settings, detection categories and signature interpretation, raw BLE/Wi-Fi scans and device actions, LOG and saved history, alert queue/actions/snooze, Watch/Hunt, DEX/BINGO, Wi-Fi settings, privacy/PIN/security, SD logging/browser, OTA update flows, and SquachMesh if supported by the ESP32-S3 BLE hardware. Implement the existing board-independent application state machine rather than reimplementing lookalike screens; keep hardware-specific drivers isolated. Require independent hardware verification of 320x480 portrait and 480x320 landscape, touchscreen hitboxes, PSRAM allocation, BLE/Wi-Fi coexistence, animations/FPS and cold boot before merging. Current R2/R3 is incomplete and MUST NOT be presented as complete.
