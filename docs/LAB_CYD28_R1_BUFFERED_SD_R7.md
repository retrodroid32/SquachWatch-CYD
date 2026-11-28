# CYD 2.8 LAB R7 — Restore R1 renderer; repair SD FILES inside the sprite

## Reproduction and baseline
User's CYD 2.8 ST7789 still flickers/tears/restarts after firmware R6.
User reports R1 did not exhibit the flicker, and suspects an SD-card fix.
An exact git comparison (R1 head 88e6c820b84f1da0290c142fecf28297b0498fa1)
against R6 shows no changes in detection.cpp / sd_log.cpp across R2-R6.
R2 PR #245 was the FIRST source change to move SD FILES from the R1
8/16-band framebuffer path to the physical LCD directly. R3-R4 changed
SETTINGS drawing to compensate, and R5-R6 modified the main renderer.
These changes are not present in this new branch.

## R7 approach
Start directly from the R1 commit, NOT from R6. Retain byte-for-byte
R1 copies of the complete main-screen CLEAR case, drawTwoBandCore(),
enterClear(), frame_push.cpp, detection.cpp, sd_log.cpp, ui_clear.cpp,
and the R1 SD/mesh/free-heap safeguards.

* Keep CYD32 SD FILES' existing native TFT behavior unchanged.
* Classic CYD28 SD FILES stays inside R1 drawTwoBand sprite, using
  the actual rotated TFT width/height to lay out rows, buttons and file
  previews, not the tiny band height.
* Limit SD filenames to the screen width and disable text wrap.
* In classic CYD28 low-memory mode, initialize SETTINGS on the small
  framebuffer instead of clearing the physical TFT immediately on
  returning from SD FILES. All R1 SETTINGS render-loop behavior remains.
* Do NOT cherry-pick R2's direct LCD SD browser or R3-R6 renderer changes.
* Historical Pages 503-signature fallback is copied without modifying
  any runtime firmware code, so LAB publication does not fail on an old
  optional firmware signature. Required files still fail closed.

## What R7 does NOT establish
R1 defers SD FAT mounting when heap < 45 KB or largest contiguous
block < 28 KB; the mount guards remain in force. R7 cannot promise SD
will mount on every cold boot. R1's SETTINGS layout still uses the
original band-aware render path; any separate menu-layout regression
requires measured hardware evidence before extending this baseline.
A green CI build is NOT evidence the display is fixed.

## Required physical hardware test (before merge)
1. Flash CYD 2.8 ST7789 R7 LAB image.
2. Boot with SD inserted three times. Record firmware label, full
   reset reason, [frame-boot] bands, [sd] mounted/deferred, heap.
3. Leave MAIN with Synthwave background untouched for 60 seconds.
   Compare against the R6 failure: no full-screen flashes, blackouts,
   redraw tearing, or unexpected CHECKING FOR UPDATES sequence.
4. Navigate SYSTEM > SD FILES; inspect file rows, filenames,
   preview, bottom BACK and orientation. Confirm data reads.
5. Return to SYSTEM and MAIN 10 times. Confirm no blank screen,
   missing menu controls or stale labels.
6. Repeat portrait/landscape, then test Wi-Fi/BLE, MeshTalk and GPS.
7. If main still flickers, capture reboot serial at 2,000,000 baud:
   code already prints resetReasonName in the R1 boot banner.

LAB-only, PR stacked on R1 #241, which sits on upstream Phase 1 #239.
R2-R6 remain separately unmerged and preserved. No production release,
PR merge, branch deletion, or tag without explicit user instruction.
