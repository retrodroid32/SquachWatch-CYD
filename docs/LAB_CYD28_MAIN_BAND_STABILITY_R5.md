# CYD 2.8 — Main screen redraw stability — LAB R5

## Evidence
The user's 44-second hardware video of R4 shows visible redraw/flicker during
SETTINGS/Main transitions (roughly 8–16 s and 18–21 s), with incomplete or
missing controls and old menu content before stable main-screen frames later
in the recording (around 28–39 s). R4 alone is not hardware validated.

## Root cause and narrow changes
R1 low-heap recovery retains a 240/320-wide, 8/16-stripe RGB332 sprite
rather than allocating an unsafe 76,800-byte full-screen buffer on an SD boot.

Two hazards remained:
1. FramePush counted each stripe push as a new frame, so its periodic
   full refresh ran every 4–8 actual screen frames, causing recurring
   full-band writes and uneven on-screen updates.
2. The small sprite was reused between stripes without explicitly clearing
   it or resetting the viewport. Old stripe pixels could survive whenever
   the next scene did not fully cover the reused sprite.

R5 counts physical refresh periods once per UI-loop frame for classic CYD28
only, holds the decision constant across its 8/16 stripes, resets the sprite
viewport and clears the small stripe in RAM before painting each new region,
and invalidates the cached row hashes when returning to the main screen.
It preserves the panel until the new screen is ready (no full LCD blanking).

No buffer-size increase, SD mount guard, OTA partition, radio, display
board variant outside classic CYD28, or production tag/release is changed.
R4's event-driven SETTINGS direct redraw remains unchanged.

## Hardware verification
1. Flash LAB R5 on CYD 2.8 ST7789; cold boot with SD inserted three times.
2. Select Synthwave background and observe Main untouched for 45 seconds:
   Squachy, counters and button bar should remain intact, no periodic flash.
3. Open SETTINGS, use BACK, and return to Main ten times. Confirm all rows
   repaint and no old menu or missing main buttons remain.
4. Repeat in landscape/portrait and SYSTEM > SD FILES > BACK.
5. Confirm animation, speech bubble, touch, SD access, mesh self-test,
   Wi-Fi/BLE scan, and GPS if connected.
6. Capture 2,000,000-baud serial and [bands] timing if artifacts persist.

Stacked on unmerged PR #247 (R4), #246 (R3), #245 (R2), #241 (R1)
and phase 1 #239. Hardware gate; no merge or production release without approval.
