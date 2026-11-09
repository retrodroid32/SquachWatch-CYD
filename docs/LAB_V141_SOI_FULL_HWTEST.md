# SquachWatch-CYD combined v1.41 + SOI LAB — hardware validation

Source base: PR #202 / firmware #176 (`2e680467`) plus selected changes from PRs
#193, #194, #195 and #197. Draft integration only; **do not merge** without
explicit owner authorization. Test only the matching CYD 2.8-inch ST7789 and
CYD 3.2-inch ST7798 profiles on `?lab=1`.

## Preflight (both boards)

1. Note the current firmware LAB label and confirm boot version, touch
   responsiveness and absence of boot loops.
2. Portrait -> landscape -> portrait, checking Settings, Log, Power Saver,
   WiFi list, active scan and Back on **both** boards.
3. With retained settings, perform BLE scan, passive WiFi survey, and WiFi
   USE/reconnect; confirm no scan-busy state, lost keys, keyboard flicker,
   unexplained GUI redraw, false-green WiFi icon or stuck scanner.
4. Remove/reinsert SD card without reboot: status + file viewer correct;
   no FAT/VFS errors. Validate GPS UBX at 115200 and NMEA (when connected).
5. Verify SquachMesh pairing and messaging both directions (2.8 ↔ 3.2);
   WATCH/HUNT, snooze, BLE OTA, existing alerts, logs and screen orientation.

## Feature checks

- **Pineapple Pager**: `pager_open` / `pager-open` identify HACKER
  candidate (not high-confidence); `pager-open-guest` and `pager-office`
  must NOT be flagged solely by prefix.
- **Eddystone/Google**: BLE service UUID `0xFEAA` alone must NOT be
  identified as GOOGLE TAG; other tracker detection continues.
- **Serial modules**: names such as JDY-16, BT05 and linvor are generic
  low-confidence candidate hardware, **not confirmed payment skimmers**.
  Do not treat a lack of a physical module as a failure.
- **Apple/Find My**: generic Apple-like BLE advertisements are LOW
  confidence; specific Find My subtype `4C 00 12 19` retains the
  stronger confidence. Do not confuse an ordinary AirPods discovery
  advertisement with an identified AirTag.
- **Remote ID**: WiFi Beacon OpenDroneID Message Packs decode Basic ID,
  Location and System messages when available; BLE legacy continues.
  If no compliant drone is available, mark as not testable on hardware.
- **Wardriver channel yield**: run repeated passive scans in sparse and
  crowded 2.4 GHz areas. Check that quiet channels are still revisited
  and discovery rate does not collapse; scan does not starve GUI.
- **Arlo OUI**: generic Espressif `BC:DD:C2` must no longer display
  as Arlo. Verified Arlo OUIs should still classify correctly.
- **Privacy**: LOG and RAWSCAN long-press dialogs mask user-supplied
  names/SSIDs while preserving vendor/type fallback labels.
- **Pocket Mode**: after BOOT is released at startup, short BOOT press
  (<1.2 s) darkens screen; touches cannot wake it; short BOOT again
  restores screen. Check with Desk mode and NEVER timeout, alert wake
  while pocket remains active, reboot, radios/scanning and SD logging.

## Acceptance gate

Capture PASS / FAIL / N/A for **both** boards and include UART excerpts
for any failures. Record frame times, free/largest heap if possible and
check for crashes. Automated CI alone is not hardware verification.
Fix/rebuild same LAB branch until physical verification passes.
**No merge, production tag or production firmware release until explicitly approved.**
