# PR185 — CYD32 3.2 full-frame parity and BLE/Wi-Fi scan control repair

## Root causes audited
- 2.8 ST7789 allocates a full-screen 8-bit sprite and sets `canvas=&frame`.
- Fork 3.2 ST7798 GPS build **always** allocated half a screen, setting `canvas=&tft` for geometry. Screen entry functions such as `uiRawScanInit`, `uiSettingsInit` and `uiPowerInit` performed opaque clears on the *physical* TFT. These unbuffered writes appeared as flashes/black bars, especially when switching BLE → Wi-Fi with a scan in progress.
- The scanner drew buttons using the font left by Squachy or SCANNING text; reused sprite font state could make BACK/BLE/WIFI labels mis-size. Button hit tests also extended below their drawn rectangles.
- Upstream `src/main.cpp` prefers a full-frame sprite without this forced CYD32 GPS two-band branch. Its full sprite can be unsafe on low-memory GPS/SD configurations, so this fork uses measured free/contiguous heap headroom.

## Code path
1. When a safe 77 KB allocation plus 32 KB contiguous / 64 KB total memory reserves exist, CYD32 uses the same **single full-frame** buffer path as the physically working 2.8. If not, the current buffered two-band path stays, without forced reboot.
2. For the two-band fallback, state-only screen entries clear the *sprite*, not the panel. Main-loop touch hit-testing and dimensions continue to use the real TFT; do not change page geometry.
3. Raw-scanner buttons explicitly reset font and wrap, and their touch rectangles match their visible bounds.

## Mandatory hardware acceptance
- **3.2 ST7798**: record `[frame] CYD32 full-sprite eligible` or `2-band fallback`, and `active renderer bands=...`. This distinguishes which path the board actually took. Confirm no boot loop, normal BLE/Wi-Fi/GPS/SD/memory operation.
- Start BLE scan, press WIFI *before* BLE completes; check buttons display **[BACK] / [BLE]** centered, do not overlap/ghost/flicker, and function with touch. Switch back to BLE while Wi-Fi is still scanning, repeat after results appear.
- In Settings, Power Saver, LOG, Security and Diagnostics, repeatedly rotate portrait/landscape; no black horizontal bands, missing Back or stale old page pixels. Check both normal and post-Wi-Fi USE low-memory recovery.
- Test multiple backgrounds, font settings, Snooze ALWAYS/OFF, SD hotplug, UBX GPS at 115200 and SquachMesh.
- Recheck 2.8 ST7789 with its correct image and no regressions.

**Do not merge or tag production before physical tests.** Preserve existing NVS. If 3.2 stays in two-band fallback, further work on its background/band state may be necessary; this is not a claim that all display artifacts are solved.
