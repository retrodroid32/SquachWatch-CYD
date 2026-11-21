# CYD 2.8 SETTINGS physical-panel redraw — LAB R4

**Target:** ESP32-2432S028 ST7789 only, cold boot with SD inserted.

## Hardware report
R2 (PR #245) fixed the landscape SD FILES browser. R3 (PR #246)
corrected SETTINGS' logical page height after returning from SD FILES, but
the user's new photo shows corrupted text and background fragments across
the SYSTEM page and a still-missing navigation strip.

## Explanation and fix
R3 continued to send SETTINGS' text, animated background, scrollbar and
pinned controls into a 15–40px-tall TFT_eSprite with a negative-Y viewport.
Passing the physical height only corrected the geometry calculation; it did
not make the sprite's drawing/text routines behave like a full-size panel.
The result was visual corruption and missing navigation.

R4 routes **only classic CYD 2.8 with an 8/16-band recovery buffer**
to the real, rotated TFT for SETTINGS, just as R2 did for SD FILES.
Settings is repainted on entry, page changes, touch edges, after a tap
commits, during drag (at most every 250ms), and every 30 seconds.
The background is painted as a snapshot rather than animating continuously,
to avoid direct-panel flicker. The original buffered animation path is
preserved for all other boards and for a full-size framebuffer.
No memory allocation, SD mount, radio, touch calibration, SPI bus, or
firmware version changes. Invalidate FramePush cache after every direct
render so the next buffered screen does not retain stale pixels.

## Hardware validation before merge
1. Build and release LAB R4 for the exact branch head; select CYD 2.8 ST7789.
2. Cold boot three times with SD inserted; check firmware is stable and SD mounts.
3. In landscape open SETTINGS > SYSTEM, scroll to bottom: rows, scrollbar,
   top icons, background and pinned [ UP ] are crisp, with no ghost characters.
4. Open SD FILES; navigate files and BACK to SYSTEM; [ UP ] must still show
   and tap correctly. Return to SETTINGS main and test [ BACK ].
5. Repeat in portrait and after changing rotation inside Settings.
6. Verify tap/release redraws the changed value without lag, and scrolling
   redraws without severe flashing.
7. Leave SETTINGS for MAIN/LOG and re-enter repeatedly; no stale text.
8. Verify SD status, MeshTalk crypto startup, Wi-Fi scan and GPS smoke checks.

**LAB only.** R4 stacks on unmerged R3 #246, R2 #245, R1 #241,
and upstream phase 1 #239. No merges, public tags or production releases
without the user's express approval.
