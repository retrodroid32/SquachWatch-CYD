# SquachWatch-CYD — Master Feature Backlog
Updated: 2026-11-09 · Research inventory (NOT authorization to merge)

## Sources and interpretation

| Source | Branch / reference | Reuse principle |
|---|---|---|
| SquachWatch fork | https://github.com/retrodroid32/SquachWatch-CYD | Preserve validated CYD base |
| SquachWatch upstream | https://github.com/skizzophrenic/SquachWatch-CYD | Audit upstream features/commits independently |
| ESP32 Marauder | https://github.com/justcallmekoko/ESP32Marauder | Port bounded passive detection and evidence techniques |
| ESP32 Dual Band Wardriver | https://github.com/justcallmekoko/ESP32DualBandWardriver | Survey/channel/GPS/export, C5 sensor ideas |
| WiFi Pineapple Pager payloads | https://github.com/justcallmekoko/wifipineapplepager-payloads | Adapt alert/recon concepts, not DuckyScript/Bash directly |
| Pwnagotchi (jayofelony) | https://github.com/jayofelony/pwnagotchi/tree/noai | Analyze channel strategy/cache/peer/session/plugin patterns; GPL-3 review before copying |

Pwnagotchi's current default branch is **noai**. Its README explicitly says the older AI system was removed in favor of uptime/battery stability. Its Raspberry Pi + bettercap + Python features are **not direct ESP32 firmware ports**. The fork should remain passive-first. Do not add active deauth transmission, rogue AP impersonation, credential cracking or capture workflows.

Statuses: **LAB** = contained in PR #203 R2 and *not yet physically accepted/merged*; **DRAFT** = existing separate PR; **NEW** = proposed new isolated PR; **DEFER** = separate hardware-dependent effort. Priorities P0 critical, P1 high, P2 medium, P3 hardware-specific.

## P0 — Verify the current integrated LAB and prevent regressions

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-001 | LAB P0 | PR #203 R2 full hardware test: Privacy LOG→EVENTS and fullscreen Snooze 10m/30m/1h/4h/ALWAYS/OFF | Both classic CYDs, no merge before user approval |
| SW-002 | LAB P0 | Check v1.41 Arlo OUI (#189), Privacy (#190), passive scans (#191), Pocket (#192), SOI confidence/Pager/Remote ID/channel yield (#194–197, #199–201) | Physical detection vectors and negatives |
| SW-003 | NEW P0 | Golden regression suite for past verified functionality: screen rotation/redraw, scanning/Wi-Fi USE, SD hot swap, UBX/NMEA GPS, Mesh, WATCH/HUNT, BLE OTA, Snooze | Prevent the R1-style modal regression |
| SW-004 | DRAFT P1 | Reconcile overlapping #193/#198 and component #189–#197, #199–#202; document source attribution/supersession, preserve original branches | After verified R2 |

## Upstream parity and GUI

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-005 | DRAFT P1 | #178 12/24-hour Desk clock | Persistence, portrait/landscape |
| SW-006 | DRAFT P1 | #179 CROWD NONE | Mesh invitation/delivery and visibility |
| SW-007 | DRAFT P1 | #180 Appearance/fonts/sliders/preview | Memory, redraw and hitboxes on both CYDs |
| SW-008 | DRAFT P1 | #182 multi-tap Snooze UX reconciliation with R2 | Prior #182 had 2.8 black-screen failure; do not copy blindly |
| SW-009 | DRAFT P1 | #183–#188 3.2 redraw/rotation/native LCD transfer | Preserve known-good master; avoid stale pixel artifacts |
| SW-010 | DRAFT P2 | #181 Pages publishing resilience and #177 README cleanup | Build/release workflow and docs |
| SW-011 | NEW P1 | Exact upstream feature parity ledger: upstream release/commit, fork paths, PR, physical acceptance | Do not assume upstream v1.41 release is already published |

## SOIs and passive wireless anomaly detection

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-012 | NEW P1 | Passive disassociation/deauth rate, suspicious sequences, and evil-twin AP correlation | Distinguish anomalies from proven attacks |
| SW-013 | NEW P1 | Evidence-based Pwnagotchi peer/device fingerprint candidate classification | No name-only high-confidence or imaginary universal marker |
| SW-014 | NEW P1 | Versioned signature packs, provenance, positive and negative test vectors | Conservative Apple/Google/serial-module false-positive handling |
| SW-015 | NEW P1 | Known AP/device allowlists and trusted/ignored policy exclusions | Exact MAC/SSID rules; do not suppress safety-critical evidence |
| SW-016 | NEW P2 | Bounded passive AP-client/probe/beacon relationships where promiscuous callback allows | Privacy, driver compatibility |
| SW-017 | NEW P2 | BLE iBeacon/Eddystone categories separate from confirmed tracking devices | Avoid false GOOGLE_TAG |
| SW-018 | LAB P1 | Verify OpenDroneID Wi-Fi Beacon message packs and legacy BLE coexistence | Real Remote ID test signal or mark N/A |

## Wi-Fi adaptive scanning and history (Pwnagotchi / Wardriver)

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-019 | NEW P1 | Per-channel visits, unique BSSID count, RSSI, discovery yield, scan time, AP/STA histogram | Build on existing #197 yield |
| SW-020 | NEW P1 | Exploit/explore policy: productive + quiet channels with guaranteed revisits | No channel starvation, BLE or UI stalls |
| SW-021 | NEW P2 | Bounded AP cache with first/last seen, status/TTL, channel, RSSI trend | Limit heap and SD writes |
| SW-022 | NEW P2 | Scanner watchdog/health telemetry for empty scans, busy radio and recoveries | Protect prior Wi-Fi fixes |

## HUNT, recorded sessions, visualization, GPS

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-023 | NEW P1 | Advanced HUNT RSSI rolling graph, peak, last-seen and channel focus | Do not infer precise distance from RSSI |
| SW-024 | NEW P1 | Recon session metadata: unique devices, rates, counts, UTC, location if trusted, drop stats | CSV/JSON to SD with rotation |
| SW-025 | NEW P1 | GPS event sidecars and offline map export | Trusted fix from existing UBX/NMEA stack; location redaction |
| SW-026 | NEW P2 | Opt-in, bounded passive packet evidence PCAP/PCAPNG for WATCH/abnormal events | Safe SD file limits, no capture/cracking UX |
| SW-027 | NEW P2 | Session statistics screen and lightweight web graphs | Per-channel, heap, radio health, GPS status |
| SW-028 | NEW P2 | Redacted export and retention/privacy settings distinct from on-screen Privacy Mode | No unexpected PII leakage |

## Wardriving, events, optional C5 multi-node

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-029 | NEW P1 | GPS geofences to pause logging, SSID exclusion filters | Detection continues during logging pause; stale fix guard |
| SW-030 | NEW P2 | Dock mode WiGLE-compatible upload, duplicate marker and backoff | Auth, explicit user opt-in, offline storage |
| SW-031 | NEW P2 | Local authenticated status/log/map/config web UI | No default exposure of sensitive device info |
| SW-032 | NEW P2 | Safe event-driven action engine (alert threshold, SD full, GPS lost) | No arbitrary shell or offensive payload execution |
| SW-033 | DEFER P2 | Secure Core/Node multi-receiver observation transport | Preserve SquachMesh messaging and privacy |
| SW-034 | DEFER P2 | RockBase C5 5 GHz observation link; #142/#144 | C5 hardware tests required; 2.8/3.2 are 2.4 GHz only |
| SW-035 | DEFER P3 | BT tether, battery/fuel-gauge, Pi UPS-like functionality where physically supported | Not generic classic-CYD features |

## Existing hardware branch backlog

| ID | Status | Task | Test / dependency |
|---|---|---|---|
| SW-036 | DRAFT P3 | Capacitive CYD32C #128–#130 | Hidden/compile-only until hardware test |
| SW-037 | DRAFT P3 | CYD35C #131/#143/#147, LCDWiki #132, CrowPanel #133/#134, ESP32-S3 #170 | Board-specific validation |
| SW-038 | DRAFT P3 | T-Watch #122/#125–#127 | Physical T-Watch test |

## Development and verification rules

1. **Keep each new item in its own draft PR**, with scope, source attribution, tests and hardware checklist.
2. **Every firmware code change must produce flashable LAB firmware for the supported 2.8 ST7789 and 3.2 ST7798, with GitHub Actions link**. No development stopped at “committed” only.
3. Test on hardware: PASS/FAIL/N/A for both boards. CI/emulator alone does not authorize merge.
4. Preserve feature branches; no production merge/release/tag without explicit approval.
5. Inspect actual code/licenses before implementation. Pwnagotchi is **GPL-3**; adapt algorithms from concepts only unless licensing review supports reuse.
6. Master acceptance for functionality means *physically verified and user-approved*, not merely committed.

## Recommended implementation sequence
R2 acceptance → permanent regression tests → clock/CROWD/fonts parity (#178–#180) → SOI anomaly fingerprints / rule pack → scan statistics + advanced HUNT → sessions/GPS maps + privacy export → dock/web UI → optional C5 Core/Node and other hardware.
