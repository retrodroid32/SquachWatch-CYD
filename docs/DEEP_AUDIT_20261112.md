# SquachWatch-CYD — detection, UI, performance and OTA audit
**Review date:** 2026-11-12 US Central / 2026-11-13 UTC  
**Review scope:** fork `retrodroid32/SquachWatch-CYD`, master at `3192c768`; HUNT LAB PR #224 R9/R10; SquachEmit workbench PR #36 source only.  
**Status:** static-source and CI audit in progress; **not** a hardware-certified end-to-end RF audit. No production release/merge is authorized.

## High-priority findings and evidence

| Priority | Finding / impact | Evidence | Remediation / validation |
| --- | --- | --- | --- |
| P0 | R9 failed OTA packaging even though 12 compiles succeeded: ILI9341 had 8,016 B available, fast ST7789 8,128 B, ILI9341-fast 8,080 B; all below 8,192 B | GitHub Build `43367633395` firmware job | R10 isolates optional HUNT serial tracing on classic CYD and reduces redundant dimension branches. Keep 8 KiB gate; verify exact matching Build, then LAB publication. |
| P1 | Main screen counters are non-interactive and decay to zero without showing *which* devices remain/left | `src/ui_clear.cpp` grouped counter text; `DetectionEngine::expireStale` | Separate category-pill draft PR. Each pill opens a current/recent category roster with active-first ordering, RSSI or last-seen time, and the existing Watch/Hunt action menu. |
| P1 | A repeat BLE advert does not increment `Detection::hits` until the device deactivates and returns; SquachEmit workbench PR #36 can reject a real continuing observation as stale because its matching rule requires `hits` increasing or a new row | `src/detection.cpp::applyBle`; SquachEmit `workbench/core.js::matchWatchDetection` | Do **not** change detection counts to satisfy the workbench. Treat an unchanged hit count as inconclusive, correlate receiver `lastSeen` or a test-only observation sequence with exact MAC and known baseline. Needs a separate SquachEmit/workbench change and physical trace. |
| P1 | RAW BLE screen deliberately bypasses the normal classified detection LOG (raw list only); tests run in this mode can appear to be misses even when packets are seen | `BleScanCallbacks::handle` early return for `RawScanMode::BLE` | Test classification in normal monitor mode; show explicit UI reminder before a future `RUN` session. Do not count RAW results as classified detection. |
| P1 | Multi-band sprites on 3.2/other panels can report band height, not physical screen height. HUNT footer in R9 was mispositioned; CLEAR/LOG also need full-panel dimensions | `drawTwoBand`, `uiHuntTick`, `clearLayoutFor`, `uiLogTick` | PR #224 R9 fixes HUNT; category PR passes tft screen dimensions into CLEAR/LOG and tests both orientations. Add hardware touch/rotation regression. |
| P2 | First manufacturer-data AD field is used for normal BLE classification, whereas some assembled broadcasts may include multiple manufacturer fields (ad plus response); a valid supported signature in a later field could be missed | `src/detection.cpp::BleScanCallbacks::handle` uses `advFieldAt(..., 0,...)`; Mesh separately iterates all | Implement a bounded all-fields classifier in a *separate*, size-budgeted PR, with positive/negative/malformed vectors and unambiguous precedence. No broad fuzzy signature guesses. |
| P2 | Low-heap gate and finite NimBLE->loop mailbox can drop adverts; Wi-Fi scan also pauses/competes with BLE. A transmitter reporting `TRUTH END ok` does not prove receiver airtime or scan participation | `s_heapLow`, `_bleQDropped`, radio diagnostics, scan duty-cycle code | Record `RADIO` before/after runs, queue depth/drop and scan state, BLE advert totals, Wi-Fi sweep state. Distinguish RF/OS scheduling from classifier misses. |
| P2 | Device type enabled/disabled filter applies before logging. Ignore/snooze/quiet affect alerting, not necessarily classification. Users can conflate silent alerts with missing detections | `Settings::typeEnabled`, `applyBle`, `alertGate`, `IgnoreList` | Workbench preflight should verify filter and radio mode; category roster remains visible independently of suppressed alerts. |
| P2 | Category counts aggregate TRACKER (Apple/Google/Samsung/Tile), CAMERA (Camera/Ring) and HACKER (HACKER/EvilTwin); presenting the group name as the exact device ID risks misidentification | `ui_clear.cpp::counterCount` | Shared `category_roster.h` grouping + per-device *actual* `DetectionType` retained on drilldown; host test proves each real type belongs to exactly one category. |
| P2 | Source-confidence limits: BLE name and Wi-Fi SSID can be user-set; vendor MAC/OUI can be spoofed. Their presence does not authenticate a branded device | `signatures.cpp` confidence rules and `detection.cpp` evidence | Preserve confidence/evidence grades; do not label uncertain signals as authenticated devices. Confirm BLE data, company/service identifiers, or Remote ID message structure when available. |
| P2 | Full LOG repeatedly draws animated backgrounds and uses variable-sized touch targets, which can exacerbate redraw/touch lag under banded buffering | `ui_log.cpp::uiLogTick`, `main.cpp::drawTwoBand` | Category view uses a calm opaque backing and single shared footer geometry. Evaluate frame p95, push durations and missed touch counts on physical 3.2. |
| P3 | Saved BlackBox rows do not hold a monotonic receiver last-seen timestamp; treating their `firstSeen` epoch as millis creates a fake countdown | `ui_log.cpp::uiLogRow`, `BlackBox::DetRecord` | Category roster is explicitly the 64-entry current/recent RAM ring; saved history remains in full LOG. Show `HISTORY` rather than fabricate a live RSSI/time for restored records. |

## Verified-by-source classifier contracts
- Flipper: name containing `Flipper` is a *MED* self-asserted signal; UUIDs `0x3081/0x3082/0x3083` and company ID `0x0E29` are exact stronger signature paths. `0x0FBA` belongs to another vendor and must not be made a Flipper match.
- AirTag: Apple company ID `0x004C` alone is insufficient; require the narrower Find My structure.
- Drone: BLE Remote ID legacy uses AD service data `0x16` with `0xFFFA` and valid OpenDroneID content, not a bare service-list UUID. Neither successful signal reception nor a self-reported serial proves aircraft authenticity.
- Generic Bluetooth serial modules are potential serial modules, **not confirmed payment-card skimmers**. Confidence should stay low.
- GPS UTC, SD logging, NVS, squachmesh, Wi-Fi USE, OTA and the 8 KiB app-slot reserve are regression requirements, not expendable features.

## Hardware acceptance matrix (2.8 ST7789 and 3.2 ST7798)
1. Flash **exact PR-head** LAB build only after **Build** and OTA size checks pass; check firmware SHA/version and available OTA margin.
2. In portrait and landscape: tap each 11/12 main category pill, including a zero-count category; confirm no overlap with the background edge gestures, WATCH/HUNT pills, status bar and bottom buttons.
3. Feed BLE name `Flipper SQE`, Flipper UUID `0x3081/0x3082/0x3083`, company `0x0E29`; verify **actual LOG type**, confidence/evidence and exact canonical MAC, not only an emitter `TRUTH ok`.
4. Keep each signal broadcasting beyond 60 seconds; verify RSSI updates while live, expected engine deactivation when transmissions stop, category `LAST` age advancing (without flashing to fake 0 dBm), and stability when packets resume.
5. Tap a category device's name; verify Watch, Hunt, Cancel, More Info and Ignore/Unignore; repeat after removing SD, with privacy enabled and after rotation.
6. Run negative vectors: generic MAC that merely looks like a vendor, unknown adjacent UUIDs, `0x0FBA` (not Flipper), Apple company ID without Find My, generic Eddystone without FMDN identity, Remote ID UUID-only without data. No false device ID.
7. Switch RAW BLE/manual Wi-Fi scan on and off; verify NORMAL monitoring and `RADIO` BLE drop/scan counters recover.
8. Verify Wi-Fi USE, GPS UBX at 115200 with FIX/UTC, SD insert/eject, SquachMesh 2.8 ↔ 3.2 and OTA smoke checks.
9. Compare redraw/frame p95, max frame jitter, heap low-water, queue depth/drops and button responsiveness against the current validated master.

**Merge gate:** all automated tests and both physical board checks pass; user expressly authorizes merge. Branches, tags, and production releases remain unchanged.

## 2026-11-13 #226 combined HUNT #224 OTA-size audit

PR #224 is merged into master. #226 merges that exact master with the bounded
category pill/roster implementation, and now passes all native tests and
emulator regression; all 12 firmware targets compile. [Full Build #43464731478](https://github.com/retrodroid32/SquachWatch-CYD/actions/runs/43464731478)
**correctly failed** the immutable 8192-byte final OTA-image reserve check:
classic cyd 6720 bytes free, ILI9341 6544, cyd-fast 6640, ILI9341-fast
6464 (worst deficit: 1728 bytes). 3.2 ST7798 has 11888 bytes free.
The isolated no-unwind-table experiment did not recover a single byte
([Build #43466581574](https://github.com/retrodroid32/SquachWatch-CYD/actions/runs/43466581574)); it has been reverted.

The CI size-symbol report before the size gate identifies the largest
linked code symbols for source-level optimization. No partition changes,
release/OTA reserve relaxation, hidden feature removal, LTO, merge, or tag.
