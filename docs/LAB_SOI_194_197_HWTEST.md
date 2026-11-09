# SquachWatch-CYD SOI algorithm ports — combined LAB hardware validation
Source PRs: #194 Pager SOIs, #195 Google/Apple false positives, #196 Wi-Fi
Remote ID decoder, #197 channel-yield scheduler.
Integration LAB only; these four feature PRs stay draft and unmerged.

## Hardware
- CYD 2.8 ST7789 and CYD 3.2 ST7798, separately and on both rotations
- SD card for logging; optional UBX GPS module at 115200 baud
- Keep existing saved networks and hardware settings. Do not ERASE ALL unless
  recovering from a boot fault, so persistence and radio scheduling are tested.

## Pass/fail matrix on both boards
1. Boot, stable touch in portrait+landscape, settings/back, no black bars/flicker
2. BLE RAWSCAN and continuous scanning return live devices; Wi-Fi RAWSCAN
   returns APs and repeated scans do not stick BUSY
3. Known benign Bluetooth serial module (JDY-16/BT05/HC-05 if available) is
   a *LOW confidence candidate*, not confirmed skimmer; unknown names ignored
4. Lab AP SSIDs 'pager_open' and 'pager-open' are shown as HACKER/PinePager
   when explicitly broadcast; similar 'pager-printer' must NOT match
5. Generic Eddystone beacon with 0xFEAA does not show GOOGLE_TAG
6. Apple Find My 0x12/0x19 frames still detect; generic proximity pairing
   pattern remains a candidate but LOW not MED. Hardware vectors optional
7. Remote ID BLE legacy still detects; synthetic 802.11 Beacon Message Pack
   and malformed variant are host tested. If Wi-Fi Remote ID emitter is
   available, verify serial and coordinates from beacon; NAN remains presence
8. Wi-Fi channels 1–13 continue cycling; compare radio frames per second,
   new network discovery, BLE queue drops and abnormal scan gaps
9. GPS UBX/NMEA fix, UTC, SD hot-insert log, SquachMesh invites/messages
   in both directions, and Wi-Fi USE/wifi icon are unaffected
10. Reboot, reenter menus and scan. No unexpected resets, hangs or redraw bugs

## Acceptance policy
Host/CI PASS is required but does NOT replace real CYD hardware validation.
Use existing ?lab=1 browser flasher. Record failing device, screen orientation,
the step number, expected/observed behavior, and relevant serial logs.
No source PR or integration PR merges without explicit user approval.

## Upstream origins
- https://github.com/justcallmekoko/wifipineapplepager-payloads
  (Find Hackers / SkimmerScanner identification ideas)
- https://github.com/justcallmekoko/ESP32Marauder
  (RemoteIdDecoder.cpp Wi-Fi Beacon vendor message geometry)
- https://github.com/justcallmekoko/ESP32DualBandWardriver
  (src/ChannelDwell.cpp normalized discovery yield)
