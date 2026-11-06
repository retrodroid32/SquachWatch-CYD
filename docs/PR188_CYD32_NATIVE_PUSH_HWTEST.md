# PR188 — isolate CYD32 ST7798 rotation corruption: native TFT_eSPI transfer

## Hardware FAIL being investigated

The 3.2 ST7798 CYD still shows overlapping LOG card text, rotated footer controls, and old orientation pixels behind Settings after PR187 R2. PR187's offscreen erase after rotation and before each recovery band **did not** fix the physical display. The 2.8-inch ST7789 display remains the hardware-stable reference.

## Change under test, NOT yet validated

1. For `CYD32` only, send the already-drawn sprite to the ST7798 through the display driver's native `frame.pushSprite(x,y)` path, rather than the custom `FramePush::push()` direct SPI register writes and row hashes. All screens, both full-frame and recovery bands, use the same native driver transfer. Expect a performance cost: full-frame pushes transfer every pixel, even when unchanged. This is an **isolation test**, not a claimed performance improvement.
2. When rotating, first call the existing `tft.setRotation()` and `applyColorOrder()` then clear physical display GRAM **once** with `tft.fillScreen(Theme::BG)`. The LCD may retain previous image bytes when MADCTL is changed; old pixels will otherwise be visible until overwritten. Do **not** clear the physical panel in normal render loops.
3. Keep `FramePush::invalidate()` for logical cache consistency, original no-reallocation sprite resize, existing touch geometry, GPS/SD/Wi-Fi/NVS, and the **unchanged 2.8** fast renderer.
4. Isolated from PR187 sprite scrubs, which remain pending and unmerged. Combined LAB will contain PR182–187 for apples-to-apples comparison.

## Physical acceptance and diagnosis

- Flash the **3.2 ST7798 variant**, without erase-flash. Serial 2000000 baud must show `[push] CYD32 NATIVE TFT_eSPI pushSprite; FramePush bypassed`. On rotation also `[rotate] CYD32 native TFT panel clear`.
- In LOG/DEVICES with numerous AirTags, rotate 0→1→2→3→0; confirm card text and bottom SCAN/LOG/CLR render correctly without ghosted rotated controls.
- Repeat in SETTINGS (and Power Saver) across all four orientations; check no underlying previous screen, black bands, stuck Back/Up, text clipping, or double-paint.
- Switch BLE→Wi-Fi mid-scan; verify buttons and touch; test snooze ALWAYS/OFF and Wi-Fi live icon.
- Compare overall speed/frame timing to the earlier fast transfer. Native pushing may be noticeably slower; **do not optimize it until corruption cause is isolated**.
- Test any available 2.8 ST7789 with its own correct image for regression.
- If artifacts remain with the native driver and a one-time panel clear, look above the SPI transfer path: inspect sprite bytes/layout, font clipping and rotation-specific TFT init. Do not add more footer overlays.
- Confirm screen behavior after Wi-Fi USE/SD hotplug/GPS for full-vs-recovery band differences.

No merge, production tag/release, or NVS wipe. User physical approval required.
