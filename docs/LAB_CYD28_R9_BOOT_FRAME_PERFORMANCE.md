# SquachWatch CYD 2.8 — R9 Boot intro frame-rate optimization (LAB)

## Hardware report and preserved baseline
R7 was confirmed stable on MAIN/menus/SD, with only the animated intro flickering.
R8 buffered BOOT through the stable R1 8/16-band display path and removed
deliberate glitch bursts; user confirmed intro no longer flashes, but the
animation became unacceptably slow and choppy.

R9 branches from R8 PR #251. Its goal is to improve the full-buffered boot
animation without touching the now-stable R7 MAIN, SETTINGS or SD.

## Cause identified in the source
The intro renders the thick SQUACHWATCH Bangers heading using about two dozen
complete glyph-outline draws, followed by a fill pass. On R8, the entire
intro is rendered once for each of 8 or 16 framebuffer stripes, even when
the logo or subtitle appears only in two to four of them. A burst of many
unnecessary glyph renders, along with repeated mascot/pixel work, explains
a plausible CPU bottleneck behind the low observed boot frame rate.

## R9 changes
- Reuse Theme::drawBangersOutline(), already proven in the firmware as a
  single-pass replacement for many repeated outline passes. This is used
  only when suppressGlitch is true (CYD28 intro); all other boot variants
  retain their original art path.
- Skip boot title, gradient divider, subtitle, cameo, version text and
  scanline drawing on stripe passes with no matching physical pixels.
  The underlying synthwave background and animated mascot remain enabled.
  The existing DrawBand hint is intentionally pixel-only; scene animation
  timestamps still come from the same now for every stripe.
- In the CYD28 BOOT case only, log [boot-r9] once per second with real
  completed frames/sec, full-frame rendering average and worst-case
  microseconds, band count and free heap. No extra framebuffer allocations.

R1/R7 low-heap SD mount protections, R7 SD FILES buffered layout, R7 MAIN
and FramePush are unchanged. R8 boot buffering and suppression of forced
glitch flashes remain enabled.

## Hardware test before any merge
1. Flash LAB CYD28 BOOT FRAME PERFORMANCE R9 onto CYD 2.8 ST7789.
2. Cold boot with SD inserted 3x. Confirm no flashing on BOOT and no
   choppy/stalled title, mascot wave, sunset or transition to MAIN.
3. Capture USB serial at 2,000,000 baud; report the [boot-r9] line for
   observed FPS and average/worst microsecond timings. A build PASS is not
   evidence of smooth animation.
4. Observe MAIN for 60 seconds with same Synthwave background; verify no
   flicker or stale pixels. Check SYSTEM > SD FILES > BACK (both rotations).
5. Confirm SD remains mounted, basic Wi-Fi/BLE, MeshTalk and GPS unchanged.

LAB-only. PR stacked on R8 #251, R7 #250, R1 #241 and Phase 1 #239.
No merge, production tag/release, or branch deletion without approval.
