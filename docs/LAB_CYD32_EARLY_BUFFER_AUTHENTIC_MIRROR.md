# LAB CYD32 early framebuffer recovery and authentic mirror provenance (R1)

## Hardware report
CYD 3.2 ST7798 / native-ui-cyd32-transport-r1: `ERROR: frame buffer allocation failed (low memory)`; 11,384 B free / 1,332 B largest block at first loop, then 10,876 B / 820 B. The previous normal initial CYD32 sprite requests 320×120×1 = 38,400 contiguous bytes **after** an optional boot-time Wi-Fi OTA check. On allocation failure that path can request `esp_restart()` if the boot check ran. The report does not provide a panic backtrace proving any other reset mechanism.

## R1 changes (SquachWatch ONLY)
- Allocate one 8-bit CYD32 display sprite immediately after TFT rotation/panel initialization and before LEDC/status LEDs, optional Wi-Fi update check, BLE/SD/GPS service initialization. Try 2, 4, 8 and 16 vertical bands with 1 KB allocator headroom; 320×240 bytes needed respectively: 38,400 / 19,200 / 9,600 / 4,800. Diagnostic `[frame-early]` lines report free and largest 8-bit heap before each attempt.
- No *second* framebuffer for mirror, no boot-time free/recreate, and no new big buffer. The normal later setup allocation is excluded for CYD32/GPS builds. Existing in-place resize on rotation is preserved. Smaller band options can increase rendering cost and require hardware validation.
- Automatic boot-time OTA update check disabled for GPS-enabled CYD32 only, preventing that optional TLS/Wi-Fi operation from fragmenting the internal sprite heap at startup; manual `UPDATE OVER WIFI` remains available. Do not erase NVS.
- When no sprite fits, continue with direct-panel fallback and explicit `MIRROR ERROR NO_AUTHENTIC_BUFFER`; do not fabricate pixels. The CYD32 SD_FILES direct TFT page explicitly reports `MIRROR ERROR DIRECT_PANEL_UNSUPPORTED`.
- `MIRROR STATUS` reports 2 <source> <bands> <screen> <rotation> <enabled> <streaming> <nextrow>. Screen/rotation/source changes while a keyframe is in flight abort with a visible error and invalidate delta hashes; no false "live" pixels.
- Existing RGB332 authentic sprite row transfer is bounded at two sent/32 scanned rows per render pass, with original 1024-byte UART TX buffer and independent receiver LOG priority. No TX buffer enlargement or PR #235 risky redraw changes.

## Limitations and tests before acceptance
- This does **not** prove ST7798 TFT GRAM readback. MISO configured in TFT_eSPI is not proof the physical panel exposes a working SDO path. Do not use readRect or claim exact panel bytes without an independent controlled color-bar readback test and checking RGB565→RGB332 quantization, BGR and inversion.
- Capturing a reused band requires physical redraws until each row is covered. Static pages or screens drawn directly to TFT may still return `NO_FRAME_REFRESH_OR_TX` or an explicit unsupported error; no full-screen exact mirror claim yet. Test first at the CLEAR screen with animation, then settings, alerts, rotations and SD_FILES.
- No PC/native UI reconstruction qualifies as an authentic screenshot. The Windows Workbench must accept complete BEGIN/ROW/END frames and discard partial/stale frames, with receiver at 2,000,000 baud and PIN unlocked.
- Hardware boot test: no resets/OTA skip loop, log `[frame-early] ready` before radio workers, free/largest heap after startup, boot for 10 min; then BLE/Wi-Fi, SD, GPS, SquachMesh, OTA, touch and both orientations. Capture full serial boot trace and mirror command/response. Test both CYD32 ST7798 and CYD 2.8 ST7789 independently.
- Build host, emulator, complete firmware matrix, final image OTA reserve >=8192 B, then LAB-only firmware + Pages. **Draft, no merge or production tag/release without approval.**
