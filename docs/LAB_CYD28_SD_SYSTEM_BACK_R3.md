# CYD 2.8 — SD FILES return to SYSTEM navigation repair (LAB R3)

Board: CYD 2.8 ST7789, ESP32-2432S028. Tested predecessor:
`CYD28-SD-FILES-LANDSCAPE-R2` (PR #245). User confirmed the
landscape SD FILES redraw fix works, but returning from SD FILES makes the
SYSTEM menu's bottom navigation button vanish.

## Root cause and fix

R2 deliberately renders SD FILES once directly to the *physical* TFT.
After BACK from the SD browser, `enterSettings()` opens the SYSTEM page,
which resumes the classic CYD 2.8 8/16-band sprite renderer. Unlike
the SD browser, `uiSettingsTick()` took the small sprite's `height()`
(15–40px) as its logical page height. The bottom pinned navigation strip
then computed an invalid/off-screen Y instead of physical `H - 26`.

In R3, only the classic CYD28 multi-band SETTINGS call passes the
real rotated `tft.height()` to `uiSettingsTick`. Its layout, rows,
scrollbar, and pinned `[ UP ]` / `[ BACK ]` strip now use the
real 320x240 or 240x320 canvas geometry. Each band still uses
its existing clipping viewport and FramePush; this does NOT
switch Settings to a flickering direct draw. Other boards, the direct
fallback, the SD FILES fix, SD mount, radios, firmware versions and
production releases are unchanged.

## Hardware verification

1. Flash LAB **CYD28-SD-SYSTEM-BACK-R3**, board **2.8 ST7789**.
2. Cold boot with SD inserted; verify SD mounts and mesh still works.
3. Open SETTINGS > SYSTEM > SD FILES in landscape; confirm no repeated
   icons, filenames, redraw or wrapping.
4. Tap SD FILES **[ BACK ]**, confirm SYSTEM shows its pinned **[ UP ]**
   along the bottom; tap it and verify Settings main shows **[ BACK ]**.
5. Repeat in portrait and again after rotating with SD browser open.
6. Check that tapping buttons works at their actual positions and that
   SETTINGS scroll and background are not corrupted.
7. Enter/exit SD FILES several times; ensure no stale lines, no new
   boot loop, and no visible back-button disappearance.

R3 is a separate unmerged LAB stacked on PR #245, and carries
the prior SD cold-boot memory guard PR #241. Do not merge or publish
production firmware until user approval and hardware verification.
