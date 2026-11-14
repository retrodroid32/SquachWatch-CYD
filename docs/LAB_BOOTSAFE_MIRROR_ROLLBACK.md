# Boot-safe isolation LAB — SquachWatch Nov 14, 2026

## Why
User reports a device boot loop immediately after installing LAB `MIRROR-CAPTURE-RELIABILITY-R1` from PR #235. This is a serious hardware regression even though GitHub CI and firmware size passed.

## Isolation design
This branch starts at **the exact earlier PR #234 LAB source**, commit `30122af91c74e33fd5fc4d16b7ec4f3d75cfa1d1`, whose Build and release firmware #43598234543 passed. It deliberately EXCLUDES **all newly introduced PR #235 device-side mirror-source checking, source-status telemetry, 2,048-byte TX buffer, and forced recovery-band redraws**. It retains PR #234's partial native UI model and PR #232's older USB RGB332 mirror protocol, which does NOT provide corrected pixel fidelity.

The ONLY functional differences from PR #234 are release-publisher recovery: tools/mirror_pages.py with a narrowly constrained GitHub-release signature fallback and regression tests. This allows the emergency LAB firmware to publish without silently skipping production site files.

This is a **diagnostic rollback, not proof of fixing the reset cause**. An earlier baseline may still have startup issues. If it also boot loops, obtain the boot ROM crash log and investigate flash layout, wrong CYD board image, peripheral wiring, NVS state, and earliest initialization.

## Recovery procedure
1. Disconnect the SquachEmit Windows Workbench and any external modules; power CYD from a standalone charger to test whether serial mirror traffic triggers the loop. Do not erase NVS.
2. For known-good recovery, use the previously verified production v1.22.0 firmware for the correct physical board (2.8 ST7789 or 3.2 ST7798). Avoid the current PR #235 LAB.
3. Optionally flash this separate `BOOTSAFE-MIRROR-ROLLBACK-R1` LAB after its Build / Build-and-release / Web-flasher publication all pass. This contains no PR #235 device changes.
4. Capture first 40–80 serial boot lines with ESP32 exception/reset reason. CYD app UART is 2,000,000 baud; boot-ROM messages may use 115,200. Preserve full backtrace.
5. Once the board starts, confirm boot, screen, touch, GPS, Wi-Fi/BLE scan, SD, SquachMesh, then USB mirror with Workbench disconnected vs connected. Test both hardware targets separately.

## Restrictions
- PRs #225, #232, #233, #234 and #235 remain preserved and unmerged.
- No SquachEmit changes, production release/tag, flash wipe, or automatic merge.
- Minimum 8,192-byte final OTA reserve remains mandatory.
- Hardware verification remains pending. CI cannot validate an actual boot.
