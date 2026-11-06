# PR187 — CYD32 rotation: old screen glyphs reused after sprite stride change

**Physical evidence:** On November 6 the 3.2 ST7798 CYD still showed severe screen corruption after rotation: LOG footer with overlapping repeated AIRTAG text and Settings showing pixels from the former orientation behind its list. Both portrait and landscape were affected after the PR182–186 LAB firmware.

## Source investigation

The working 2.8 and 3.2 share the same 240x320 pixel count; PR185 enabled a safe full 8-bit framebuffer on the 3.2 where memory permits. The common rotation code **relabels the same allocation from 320×240 to 240×320** in `ResizableSprite::resizeInPlace()` without clearing its old bytes. This is allocation-safe, but its contents then represent pixels with a different row stride.

The LOG and Settings animated backgrounds do not guarantee an opaque paint of every pixel on every frame. Before the fix they may therefore reveal glyphs and pixels from the pre-rotation layout. A row-readback transition glitch could further scatter those unrepainted old pixels.

## Targeted correction

- CYD32 only: immediately after `resizeInPlace()`, reset viewport and clear **the existing offscreen sprite allocation** with background color, exactly once per rotation.
- Keep existing `FramePush::invalidate()`, which repaints all physical rows using the new panel orientation; do **not** directly clear the TFT, create another buffer, or allocate/free memory.
- Skip the short readback-based CRT transition glitch only for a rotation; ordinary screen navigation transitions remain.
- Works in either full-frame (one buffer) or the existing memory-safe 2/4/8/16-band fallback; stable 2.8 path untouched.
- Include source check `tools/test_cyd32_rotation_scrub.py` in CI.

## Physical checks

1. Flash correct ST7798 3.2 LAB image without erase. On USB serial `2000000`, capture `[frame] CYD32 active renderer bands=N` and `[rotate] CYD32 scrubbed W×H sprite; bands=N panel=W×H`.
2. Enter LOG/DEVICES with seven or more rows, rotate through **all four orientations** 0→1→2→3→0. Check old rows do not overlay or appear behind the SCAN / LOG / CLR footer.
3. In LOG switch EVENTS/DEVICES and scroll through last row. Rotate rapidly and check button labels/text remain properly aligned, not smeared or doubled.
4. Open Settings and its subpages, rotate through all orientations, check no previous screen text behind items and BACK/UP/OK remains reachable.
5. Repeat with multiple animated background modes (especially Digital Rain, Terminal, Snowfall and Black) and with Log empty.
6. Test BLE↔Wi-Fi mid-scan, Snooze ALWAYS/OFF, SD hotplug, UBX GPS 115200, Wi-Fi saved status, mesh messaging.
7. After Wi-Fi USE fragmentation, rotate again to exercise 2+ band fallback if available. Compare 2.8 ST7789 for regression.

**Draft/unmerged. Hardware testing still required.** No production releases, tags, merge or erase-flash without explicit user direction.
