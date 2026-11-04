# PR #182 — hardware acceptance: upstream v1.40 multi-tap SNOOZE

**Draft LAB only.** Physical testing required on 2.8 ST7789 and 3.2 ST7798.
Keep v1.22.0 production, pairings, SD contents, GPS and existing saved settings.

## Expected selection
1. Trigger a normal detection alert. Tap SNOOZE once and release: shows **10 MIN** and **TAP AGAIN / LONGER** above it with a three-second progress strip.
2. Tap again on the SNOOZE button: **30 MIN**. Tap anywhere on the card: **1 HOUR**, then **4 HOURS**, then wraps to **10 MIN**.
3. After the last tap, wait three seconds: toast names the duration and the alert closes. Detect/log behavior remains active; the device must not interrupt until the chosen duration expires.
4. **Hold** SNOOZE for >=900 ms: the legacy 10 MIN / 30 MIN / 1 HOUR / ALWAYS modal opens. CANCEL changes no policy; ALWAYS survives a reboot, appears in SNOOZED DEVICES, and can be unsnoozed.
5. Confirm a physically *held* finger never advances multiple duration steps and that the card cannot auto-dismiss during a selection.
6. Confirm other alerts, WATCH and HUNT buttons, info overlays, PIN lock, portrait/landscape and SquachMesh remain correct. Test on both hardware boards.
7. Validate 4-hour snooze expiry behavior with accelerated host/runtime tests if available; serial/heap monitoring for scan/SD/GPS regressions.
8. Do not erase flash. Prior ALWAYS records are kept under the existing NVS key and can be changed to timed or unsnoozed.

Only merge after user confirms physical verification. Do not tag or publish as production.
