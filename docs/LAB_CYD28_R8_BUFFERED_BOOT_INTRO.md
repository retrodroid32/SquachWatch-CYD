# SquachWatch CYD 2.8 — Boot splash flicker recovery LAB R8

## User hardware report
R7 restored stable MAIN and menus, including SD FILES. The ONLY
remaining flashing is the 3-second intro splash before MAIN on boot.

## Direct source cause (R7)
On classic CYD28, R1 successfully allocates a retained 8/16-band
RGB332 sprite and sets canvas=&tft to reserve heap while SD is mounted.
The MAIN case always calls drawTwoBand() and buffers sprite transfers.
But enterBoot() called uiBootInit(*canvas) and the BOOT case called
uiBootTick(*canvas,now): in recovery, *canvas is the physical tft.
Thus each animated boot pass repainted and erased the real LCD,
revealing intermediate sky/text/mascot/scanline primitives, unlike MAIN.
Also uiBootTick() deliberately fired two glitch bursts at 500ms and
1750ms; these intentionally flicker even with an otherwise stable LCD.

## R8 narrow fix, stacked on the successful R7 source
1. On classic CYD28 in 8/16-band recovery only, initialize the intro
   with uiBootInit(frame) instead of uiBootInit(tft).
2. Draw BOOT using R1's original drawTwoBand() already used by MAIN,
   not the direct physical LCD. The original sprite is reused;
   no extra RAM, SPI driver changes or allocation.
3. Pass real rotated tft width/height to uiBootTick() and the optional
   crash panel so title, subtitle, mascot, scanline, and bottom status
   draw at full-panel coordinates even though the sprite is short.
4. Disable boot-only scripted glitch bursts for classic CYD28 to
   eliminate visual flashes; keep the original boot effects on all
   other boards.
5. Cache chromatic subtitle jitter by actual frame time, so bands of
   the same displayed frame cannot jitter independently.
6. If sprite creation failed, preserve the direct LCD safety fallback,
   with intentionally triggered glitch bursts disabled.

No changes to R7 MAIN case, sprite/band renderer, FramePush writer,
SD filesystem driver, SD mount thresholds, detection, MeshTalk, GPS,
or any other board's boot splash. No public release/merge/tag.

## Validation
- Exact-head full Build (host tests, emulator, board firmware OTA
  margin >=8192 bytes), followed by Build and release firmware LAB.
- Cold boot CYD 2.8 ST7789 with SD inserted three times.
- Intro: no repeated full-screen clear/flicker; title, Squachy,
  background, INITIALIZING and any crash card should remain legible.
- Verify transition to MAIN remains stable, observe 60 seconds.
- Check SYSTEM > SD FILES > BACK, both orientations, and SD read.
- Repeat normal boot and optional crash card condition if available.
- Do NOT merge before user confirms hardware behavior.

Source PR stacked on R7 #250, which itself is on R1 #241 and #239.
R2-R6 remain independent, untouched and unmerged.
