# SquachWatch LAB R10 — SquachEmit R12–R14.7 rendering parity audit (CYD 3.2 ST7798)

## Scope
SquachEmit reference repository retrodroid32/SquachEmit PRs:
- #58 R12: eliminate physical ST7798 clear/redraw flashing (R12's frozen animation was a temporary diagnostic only; DO NOT import).
- #63 R14.2: one logical frame timestamp, DrawBand clipping, advance once.
- #64 R14.3: original Theme::drawActiveBackground with s_animK, FPS timings.
- #70 R14.6: optional FULL8 76,800B one-pass if safe, half-height fallback.
- #72 R14.7: row-hash FramePush, optimized transfer and A/B switch.

Target repository: retrodroid32/SquachWatch-CYD. R10 branches from R9 CYD28 LAB
(PR #252) and retains the previous R1/R7 stability and SD memory safeguards.
SquachEmit is NOT modified.

## Already present — retained
- Animated MAIN is offscreen buffered with drawTwoBandCore, not wiped on the live LCD.
- loop() samples now=millis() once for its full logical frame, and both passes
  receive the same now through the drawTwoBand lambda.
- First band passes advance=true, later bands advance=false.
  DrawBand limits pixels to each physical panel stripe.
- uiClearTick calls Theme::drawActiveBackground to control s_animK and
  memory releases, instead of calling independent backgrounds directly.
- ST7798 uses hardware-debugged native TFT_eSPI::pushSprite, NOT raw
  FramePush row hashing/SPI writer. Cached rows invalidated after direct pushes.
- GPS CYD32 retains two 8-bit half-height stripes (~38,400 bytes at 320x240),
  avoiding risky 76,800B FULL8 that could starve Wi-Fi/BLE/SD/GPS/Mesh.
- CYD28 R1/R7/R8/R9 boot, MAIN, SD browser and memory logic untouched.

## Missing behavior fixed in R10
1. Theme::drawGibson() used now==lastMs as dt=16 and advanced flyZ again in
   stripe 2. R10 uses dt=0 for repeated timestamps; one camera position
   per whole display, preserving first-frame 16ms and 100ms clamp.
2. Digital Rain's red-glyph hold condition read fresh millis() on each
   stripe. R10 uses the shared logical now so a 500ms expiry cannot
   split a displayed frame.
3. ST7798's buffered BOOT used uiBootTick(t, now), so an offscreen
   half-height sprite supplied wrong physical height for Squachy,
   status and Synthwave. R10 passes actual rotated panel width/height to
   the boot and optional crash card, and suppresses only scripted CRT
   bursts on this board. No static/frozen animation.
4. drawTwoBandCore prints real completed BOOT (~1s) and MAIN (~5s) FPS,
   full-frame average and worst draw and native push time, band count,
   free heap and largest contiguous heap. No extra framebuffer allocation.

## Deliberate non-changes
- Do not copy R12's frozen animation or R14.7's second fast-push writer.
- No FULL8 allocation experiment without hardware memory measurements.
- No changes to native TFT_eSPI push, R1/R7 2.8 memory, display or SD.
- One-time rotation LCD clear/static system menus are separate from
  animated MAIN drawing; their behavior requires specific hardware evidence.

## Physical verification required before any merge
1. Full CI board matrix (2.8 ST7789 and 3.2 ST7798), host tests,
   emulator, LAB firmware build/release and Pages publication.
2. Flash each CYD with SD inserted and cold boot 3 times; no restart loops.
3. Capture ST7798 [cyd32-frame] over 2,000,000-baud USB serial, recording
   real BOOT and MAIN FPS, draw_us/push_us, worst timings and free/largest.
4. Observe 60 seconds on each Synthwave, The Gibson, Digital Rain,
   Snowfall; check continuous seamless top/bottom background + Squachy.
5. BOOT: no flash, half-height placement, clipped intro/crash status or
   choppy animation. MAIN: no periodic flashes or blank strips.
6. Rotate portrait/landscape and back; check first post-rotation repaint.
7. Test notifications/popups, SD FILES read/back, Wi-Fi/BLE scan, GPS
   FIX/SAT/UTC, SquachMesh two-way, WATCH/HUNT and snooze.
8. On 2.8, repeat boot, MAIN, SD and rotation smoke checks to ensure
   the stable baseline did not regress.

This is LAB-only draft work. Preserve all branches and PRs; no merge,
production release/tag or deletion without user permission.
