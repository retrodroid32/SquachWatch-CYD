# PR184 — ST7798 3.2-inch GUI redraw and Wi-Fi badge audit
Draft LAB only. Physical tests on 3.2 ST7798 and 2.8 ST7789 required.

## Findings / changes
- Nine noncritical menu/list screens redrew their full sprite on every engine loop. On the 3.2 CYD low-memory path, a single frame may be 2 to 16 separate drawing and pushing passes. These pages now refresh at most once every 125ms while idle. Touch-down, touch-up, the next frame after committing a menu value, screen transitions and rotations paint immediately.
- Main screen, live alerts, raw Wi-Fi/BLE scans, Wi-Fi keyboard/OTA, messages and mesh radio processing are not capped.
- CYD32 changed-row transfer previously forced a complete band refresh every 64 push calls, potentially only one band of an otherwise unchanged screen. All bands refresh together every 240 complete frames. Other boards retain prior interval.
- The Wi-Fi badge green icon reflected a saved JOINED result from NVS, not a live connection. Green now requires a live station association. Cyan means a network was saved but station is disconnected; amber not found/failed; red invalid credentials; yellow connecting; gray no saved network. The board normally disconnects after Wi-Fi boot check or USE so detection can scan. It must not claim it is connected after that.

## Hardware tests
1. Save a network and verify USE; reboot with AP out of range. Wi-Fi icon must not be green. An explicit failed check should be amber if network not found.
2. Retry USE and test connecting, wrong password, unavailable AP. Only show green with live STA association, never simply on a historical JOINED.
3. Open Settings, Power Saver, Security, LOG, Ignore, Detection Filter and Diagnostics in portrait/landscape. No repeated full redraw/tearing, touch and back must remain responsive, menu values must refresh right after release.
4. Use animated backgrounds, switch and rotate repeatedly. Check all bottom bands and no stale content; compare PR183 Back fix when combining.
5. Run USE twice then immediately navigate menus to exercise 2/4/8/16-band recovery and direct-panel fallback.
6. Read serial at 2,000,000 baud: [ui32] menu draw/skip and [bands] to quantify. Verify alerts, WATCH/HUNT, GPS, SD hotplug, SquachMesh, BLE/Wi-Fi scanning, USB and BLE OTA.
7. Do not erase flash/NVS. No merge or production release until hardware validation.

Note: The Wi-Fi icon is not an in-range survey while Wi-Fi station is off. Cyan does not assert that the AP is reachable, only that a saved network exists.
