# SquachWatch native UI v2 — LAB handoff for Windows Workbench

## Status
This SquachWatch-only development PR builds on the newer authentic-pixel PR #232, which was green on CI. The old native-state PR #233 is preserved unchanged because it is behind master and failed the OTA reserve check. This v2 is a PARTIAL authoritative state transport, **not** a faithful screen renderer for all 42 SquachWatch AppStates. No SquachEmit changes are made in this branch.

The firmware transmits real screen identity, orientation and dimensions, privacy status, grouped live/boot category counters and a bounded set of recent RAM log rows. Unmodeled graphical content, animations, settings, diagnostic pages, alert overlays, graphs, complete BlackBox history and other screens remain UNSUPPORTED for the native renderer. Use the authentic RGB332 MIRROR protocol as the visual fallback. Never invent a screen.

## Exact ASCII USB protocol
USB serial line-oriented (LF). Connect at the baud configured for the receiver firmware (many SquachWatch builds use 2,000,000 baud; do NOT assume SquachEmit's 115,200). Serial model records have their own UI prefix and do not alter the authoritative receiver LOG CSV format.

Host → device:

    UI HELLO              capabilities, no implicit activation
    UI ON                 activate native state and disable pixel mirror
    UI GET                request state (bounded to 5 per second)
    UI COUNTS             request grouped pill counter snapshot
    UI LIST 0 6           recent DEVICES log or focused category roster
    UI TOUCH 140 80 DOWN  remote press in current rotated LCD coordinates
    UI TOUCH 140 90 MOVE  drag held press; requires prior DOWN
    UI TOUCH 140 90 UP    release press, including after rotation
    UI OFF                end native state and release remote press
    MIRROR ON             switch to authentic RGB332 pixel fallback
    MIRROR FRAME          request authentic full frame
    MIRROR FRAME 12       changed rows based on acknowledged frame 12
    MIRROR OFF            disable pixel capture
    LOG                   existing authoritative receiver evidence export

Device → host:

    UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL
    UI READY 2 SQUACHWATCH TOUCH PARTIAL
    UI STATE 2 <seq> <screen_id> <width> <height> <rotation> <uptime_ms> <privacy> <kind>
    UI COUNTS 2 <seq> <type>:<fresh>:<sinceboot> ...
    UI LIST 2 <request_seq> <category_id> <start> <total_ram_rows> <return_count>
    UI ROW 2 <request_seq> <rank> <type> <fresh> <rssi_dbm> <age_sec> <first_seen_raw> <restored> <mac_text> <name_hex>
    UI END 2 <request_seq>
    UI OFF OK
    UI TOUCH OK
    UI ERROR <reason>

UI STATE details: screen_id is numeric AppState 0..41 in include/state.h; width/height are actual physical LCD pixels, rotation 0..3, uptime_ms is time since boot, privacy is 0 or 1. kind=0 means native screen content UNSUPPORTED (require authentic pixel fallback), kind=1 means partial MAIN model, kind=2 means partial DEVICES LOG model (only RAM rows). LOG EVENTS is kind=0. An identifier does not authorize the EXE to invent its on-screen contents.

UI COUNTS: eleven visible groups in display order, optional IBEACON only if enabled: FLOCK(1), AXON(2), META(3), SKIMMER(4), RAVEN(5), AIRTAG/TRACKER(6), DRONE(7), ALPR(8), CAMERA(9), HACKER(17), DEAUTH(14), optional IBEACON(16). Uses firmware CategoryRoster grouping and 4s BLE / 6.5s WiFi radio freshness. Sinceboot is a unique detection count, not lifetime NVS total. Emits at most once per second while MAIN active or on request.

UI LIST: start 0..63, count 1..8, valid only on current LOG DEVICES page/filtered category roster. It includes bounded RAM log (up to 64), NOT all retained BlackBox history or EVENTS. Category roster orders fresh devices ahead of stale devices. Each actual row's time is original firstSeen raw: uptime milliseconds for current-boot entries, epoch seconds for restored rows (restored=1). Name is max 19 real bytes ASCII-hex-encoded or "-" if redacted or unavailable. MAC is colon delimited and privacy-masked (last three octets XX) when privacy mode active. The host must validate matching LIST, ROW and END request sequence; never display incomplete snapshots as complete pages. If privacy toggles mid-export, the firmware aborts with UI ERROR PRIVACY_CHANGED.

No PIN digits, keys, WiFi passwords or raw network packet contents are transmitted by native UI. Locked state revokes active sessions and refuses serial commands. Remote holds time out after 1.8 seconds without refresh; physical touch is preferred over mouse touch. MODE switches are mutually exclusive. The previous pixel mirror's authentic capture, bounded changed rows, fallback error and evidence prioritization remain intact.

The native model is polled in the SquachWatch firmware loop after detection engine processing. No radio service is created. UI traffic is opt-in, capped at 5 short state records per second, counters 1Hz, and at most 1 recent-device row per loop with nonblocking serial queue checks. LOG output on that same serial port is synchronous and remains the independent source for Actual/Expected test scoring. Missing receiver evidence is UNVERIFIED, never PASS.

## Exact Windows EXE changes — separate SquachEmit session only
1. Keep SquachEmit's working emitter UI READY 1 320 240 MODEL and its existing 115200-state Canvas renderer unchanged. The SquachWatch receiver uses a **different UI protocol version 2**.
2. Connect SquachWatch COM at its actual configured baud, send UI HELLO, verify CAPS 2, then UI ON and wait for READY 2. Parse STATE 2 and only attempt partial native rendering when kind=1 or kind=2. Treat all other screens and unmodeled decorations as needing pixel fallback.
3. When MAIN, parse COUNTS and place the eleven/optional twelve groups in the exact firmware order. On DEVICES LOG/category roster, request UI LIST 0 6, assemble ROW records using request_seq, distinguish fresh from stale, handle restored timestamps and masked privacy. UI LIST does not replace the independent LOG CSV evidence collector.
4. For unmodeled screens, send UI OFF then MIRROR ON, request authentic MIRROR FRAME and subsequent MIRROR FRAME N deltas (existing SquachEmit MirrorAssembler already understands this format). Use the pixel fallback; do not draw imaginary menus. Before switching back to UI ON send MIRROR OFF.
5. Translate Windows pointer to current STATE width x height; send UI TOUCH coordinates and DOWN/MOVE/UP, including UP on disconnect or pointer cancellation. Do not guess firmware button actions. Show stale/offline/unsupported explicitly.
6. Maintain independent receiver and emitter USB command queues. Reserve LOG and STOP priority. Do not feed UI STATE/COUNTS/ROW into receiver LOG CSV parser, and never claim expected emitter type as Actual receiver detection. RF automation must continue during UI telemetry. Reconnect requires capability renegotiation and fresh state.

## Hardware validation gate
Required on physical 2.8 ST7789 and 3.2 ST7798: boot, USB CAPS/READY, 4 rotations, accurate live category counters, recent row values, physical touch and remote click/hold/swipe/scroll, privacy toggle, PIN lock/revocation, 30 minute USB reconnect soak, LOG during BLE/WiFi detections, no false Actual, GPS fix/sats/UTC, SD/logging, SquachMesh, WATCH/HUNT, scanning, OTA and full RGB332 pixel fallback. Check memory and redraw jitter. Do not merge or create production releases without approval.

Build CI must pass full firmware matrix and >=8192 bytes real final OTA reserve. Green CI alone is not physical validation and cannot establish full native Windows UI parity.
