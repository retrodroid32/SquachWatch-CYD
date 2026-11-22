# First-seen preservation — CYD hardware verification

## Expected behavior
The purple timestamp in a category roster/LOG row is the **original detection time** of that MAC + DetectionType in the current in-RAM log. It no longer changes on a return. The LAST age (for stale rows), RSSI, active state, hit counters and alerts continue updating.

**Scope limit:** RAM log retains 64 identities. If a row is evicted, cleared, or the board reboots, a later detection starts a new row and therefore a new firstSeen. SD/BlackBox chronological records remain unchanged; durable first-ever lookup across evictions and boots is a separate future feature.

## Both physical targets
- CYD 2.8 ST7789, portrait and landscape.
- CYD 3.2 ST7798, portrait and landscape.

1. Boot, start without GPS/Wi-Fi synchronized time. Detect a BLE tracker and a Wi-Fi camera from controlled emitters. Confirm category rows display `UP+M:SS`.
2. Stop the emitter beyond stale expiry (the category list may become stale earlier); leave the original log entry retained. Confirm its FIRST timestamp does not change.
3. Transmit the same MAC + type again. Confirm it returns ACTIVE and RSSI/LAST/hits refresh while the original first timestamp remains unchanged.
4. Repeat several times; test WATCH/HUNT/IGNORE action panel and ensure no duplicate LOG rows for the same MAC + type.
5. Trigger controlled deauth-burst detection twice, with an idle interval, and check the same timestamp behavior without changing anti-spam alert gating.
6. Synchronize the clock and verify the FIRST stamp is mapped to local clock time; check day/date formatting.
7. Verify SD, GPS, SquachMesh, OTA update, both orientations, and radio scanning still work. Ensure no redraw/flicker regressions.
8. Check a cold reboot starts a new session stamp. Test LOG clear if practical; new identities after clear receive a new first time.

## CI gates
- Host test: `test/detection_visit_test.cpp` validates independent visit timestamps and millis rollover.
- Static integration guard: `tools/test_first_seen_stability.py` validates all three reactivation paths and `SpamWatch::noteVanish` current-visit duration.
- Full firmware matrix and mandatory >=8192-byte minimum OTA reserve must pass before creating any LAB images.

**Do not merge based only on CI.** Confirm actual 2.8/3.2 devices before merge. No production tag or GitHub Release.
