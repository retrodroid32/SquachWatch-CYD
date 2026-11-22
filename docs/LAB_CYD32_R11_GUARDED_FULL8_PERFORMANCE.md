# SquachWatch CYD32 LAB R11 — Guarded single-pass performance recovery

## Report
Hardware feedback after R10: animations on the CYD 3.2 ST7798 are unacceptably
choppy/laggy. R10 fixed Gibson top/bottom state divergence, Digital Rain's
clock, and boot layout; its GPU path still sent two half-height RGB332
stripes every frame, redrawing the scene twice. R10 did not change the
physical SPI speed or allocate a full framebuffer.

## R11 experimental improvement
SquachEmit R14.6 gained smoothness by rendering the original animation,
Squachy and chrome ONCE into a full RGB332 320x240 framebuffer (76,800B),
with a safe half-buffer fallback. SquachWatch has additional GPS, BLE, Wi-Fi,
SD and mesh pressure, and raw SPI FramePush caused stale-row problems;
so R11 does not unconditionally switch to FULL8 or alter ST7798 push.

At BOOT, FULL8 is attempted only when BOTH of the following are true:
1. Free heap before allocation >= framebuffer bytes + 64KiB + 8KiB cushion.
2. Largest allocatable 8-bit heap block before allocation >= framebuffer
   bytes + 24KiB + 8KiB cushion.
After successfully allocating FULL8, require BOTH:
- Free heap >= 64KiB.
- Largest allocatable 8-bit block >= 24KiB.
If any check fails, release the optional full sprite and fall back to R10
HALF8 (two bands). The same reserve guards apply after saved-network/OTA
frame lending, so a fragmented Wi-Fi heap cannot silently reacquire FULL8.
This is a one-time selection, not a per-frame allocation.

FULL8 uses the existing animation wrapper and native TFT_eSPI pushSprite
in ONE pass instead of two. HALF8 remains R10's synchronized two-pass
render. In both modes, real LCD GRAM is not erased between animated frames.
Rotation uses resizeInPlace, not a 76.8KB reallocation.

## Diagnostics
R11 BOOT prints [cyd32-r11] mode=FULL8/HALF8 with heap and largest.
R10 HALF8 logs [cyd32-frame] with actual completed fps and draw/push times.
R11 FULL8 logs the same fields every 1s on BOOT and every 5s on MAIN with
mode=NATIVE_FULL8, plus free and largest 8-bit allocatable heap.

## Testing still needed
Full CI and LAB firmware/release/Pages must pass. But **the board must
select FULL8** for the single-pass speedup to apply. If it prints HALF8,
the actual memory guards are protecting radio/GPS and further work must
target measured per-band CPU/SPI cost. Never call source tests proof of
smooth hardware animation.

Hardware 3.2: flash R11, cold-boot with SD 3x, record [cyd32-r11] and
[cyd32-frame] at 2,000,000 baud; observe boot/MAIN and all four backgrounds
Synthwave, Gibson, Digital Rain, Snowfall, 60s each; inspect same-time seam,
rotate portrait/landscape; verify SD read/back, BLE, Wi-Fi, GPS FIX/SAT/UTC,
SquachMesh pairing/messages, WATCH/HUNT. Compare actual R10 FPS, draw/push,
free/largest, not subjective labels alone. If FULL8 creates any instability,
revert to R10; no automatic production merge.

CYD28 R1/R7/R8/R9 boot, MAIN and SD memory recovery are unchanged. 
LAB-only, stacked on draft R10 PR #253. Preserve branches and do not
merge, tag, or release production without user approval.
