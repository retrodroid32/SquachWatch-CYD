# PR #182 — hardware acceptance: six-choice tap-through SNOOZE

**Draft LAB only. Physical testing required on 2.8 ST7789 and 3.2 ST7798.**
Preserve v1.22.0 production, SD, GPS, pairings and saved preferences.

## Expected behavior

1. On an alert, tap **SNOOZE** once: **10 MIN** shows with **TAP AGAIN / TO CHANGE** and the three-second progress strip.
2. Each new tap anywhere on the alert cycles **30 MIN → 1 HOUR → 4 HOURS → ALWAYS → OFF → 10 MIN**. A held finger never repeats. No long-press picker exists.
3. Stop tapping for three seconds to apply. Timed choices expire automatically; **ALWAYS** persists across reboot and remains in **SNOOZED DEVICES** until removed.
4. **OFF** removes the current device's snooze, including saved **ALWAYS** in NVS, so alerts are allowed again. Selecting OFF when unsnoozed is an accepted no-op. Detection and logging never stop.
5. Validate **ALWAYS → reboot → OFF → reboot**: the device must remain unsnoozed and no longer appear in **SNOOZED DEVICES**.
6. Test no auto-dismiss mid-choice, WATCH/HUNT, lock, info panels, portrait/landscape, touch, GPS/SD and SquachMesh.
7. Use the correct images for **2.8 ST7789** (`cyd`) and **3.2 ST7798** (`cyd32-st7798`).

**Known unresolved issue:** Earlier PR182 firmware produced a black screen on the 2.8 ST7789 while snooze was later reported working. A green CI result does NOT resolve this. Physically validate boot/display and capture serial logs if it recurs.

No erase-flash, production tag/release or merge until both boards pass.
