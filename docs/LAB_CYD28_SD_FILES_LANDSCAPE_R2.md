# CYD 2.8 SD FILES landscape redraw — LAB R2

**Board:** ESP32-2432S028 ST7789 (CYD 2.8). **Prior boot fix:** PR #241
CYD28-SD-IN-BOOT-MEMORY-R1. The user confirmed SD mounts and file browsing
works but uploaded photos show repeated icons/text and overlapping filenames
in landscape.

## Root cause

The R1 boot memory recovery replaces the 76800-byte full frame buffer with
8/16 horizontal strips. SD FILES still called `drawTwoBand(SdBrowser::draw)`,
but the browser uses `t.height()` to compute its row and button positions.
Each strip's `height()` is 15–30px, not the physical panel's 240/320px,
and the browser was redrawn in every strip. Long filenames also wrapped at
font-1 size 2 onto the next row.

## Changes

- On classic CYD 2.8 **only when multi-band recovery is active**, draw this
  *non-animated* SD FILES screen once directly to the **rotated physical TFT**
  on dirty events, as CYD 3.2 already does. No additional framebuffer.
- Invalidate `FramePush` row cache after the direct render so the next
  sprite-based screen fully replaces the menu.
- Preserve old draw path when classic 2.8 has a full framebuffer or is in
  direct-panel fallback; other boards unchanged.
- Disable wrapped text for SD browser list/status lines, limit filename
  characters to fit 320x240 landscape or 240x320 portrait with margins.
- Add `tools/test_cyd28_sd_files_landscape.py` to CI.
- No change to SD mount/recovery guard, Wi-Fi, GPS, mesh, or releases.

## Physical hardware validation

1. Flash **CYD28-SD-FILES-LANDSCAPE-R2** choosing **2.8 ST7789**.
2. Cold boot three times with SD card already inserted; no reboot loop.
3. Open SYSTEM > SD FILES in portrait and landscape. Title, status icons,
   directory/file rows, WIPE ALL and BACK each show **once** with no residue,
   overlap, smear, or clipped multi-row wrapping.
4. Open a long filename and FILE VIEW; switch rotation, next page, go BACK;
   verify touch targets and preview page/scroll still work.
5. Leave SD FILES and visit main/settings; verify no ghosted SD pixels.
6. Remove/reinsert SD, check mount state; review serial for version,
   `[meshtalk] crypto self-test`, and SD ready vs deferred.
7. Do not WIPE ALL during verification unless deliberately testing deletion.

## Change management

LAB only, from existing PR #241 boot-memory branch. Do not merge PRs or
create production tags/releases without explicit user authorization.
