# Upstream v1.41.0 addendum — reviewed 2026-11-08

Latest verified upstream published release: [v1.41.0 “Return to Sender”](https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.41.0), released 2026-11-07. The v1.40 checklist below remains a backlog, not a claim of full upstream parity. Fork master has selectively merged #188 native CYD32 LCD transfer; v1.22.0 is still the last published fork release.

## Newly verified general fixes to prioritize on classic 2.8 / 3.2 CYDs

| Upstream v1.41 item | Confirmed fork gap (master audited 2026-11-08) | Action / acceptance |
|---|---|---|
| #114: remove false Arlo camera OUI `BC:DD:C2`, add `FC:9C:98`, `48:62:64`, `A4:11:62` high-confidence cameras | Fork `src/signatures.cpp` still has the false Espressif-as-Arlo row and lacks three Arlo rows | New isolated signature-fix PR; host signature regression + hardware BLE/Wi-Fi detections |
| #114: passive WIFI SCAN / update network picker | Fork implementation not yet audited conclusively for all scan entry points | Audit Wi-Fi active/passive flags, preserve pairing/network UX and verify no probes on both CYDs |
| #102: Privacy Mode should mask self-names, not type/manufacturer labels | Fork `src/main.cpp` still calls `privLabel(s_confirmLabel)` in LOG and RAWSCAN irrespective of name origin | Add a captured is-name flag, pass through both confirm paths and test RING/vendor versus custom name |
| CI: passive-scan guard and OUI ownership verification | Not yet compared | Add targeted tests on isolated PRs, then board matrix |
| Emulator bench UI and real independent T-Deck/T-Watch/Cardputer builds | Existing fork emulator #164 already merged but this multi-board bench is new | Separate browser-emulator PR; check emulator board fidelity, CI and flasher links |

## New hardware-specific features, NOT automatic classic-CYD parity

- T-Deck Meshtastic PKI direct messages, delivery ACK / retry indicators and private DMs list.
- T-Deck MeshCore Ed25519 identity, advert/contact discovery, path-return or bare ACK receipts, and private DMs; LoRa radar-dot shortcut; MCKEYS/MCADVERT/MCDM console.
- Waveshare ESP32-C5 Touch LCD 2.8" beta build: compile-only until actual Waveshare board is available. Distinct from the RockBase NM-CYD-C5.
- Freenove S3 GPS-specific build; our classic CYDs already have independently implemented UBX+NMEA GPS, WiGLE and UTC. Maintain separate board validation.
- Tab5 touch repair and BOOT-button power-off build flag: board-specific.

## Recommended implementation order

1. Correct #114 Arlo false-positive and ensure passive Wi-Fi scans in isolated PR(s).
2. Correct #102 LOG privacy confirmation labels in isolated PR.
3. Hardware-verify staged v1.40 feature PRs #178 (clock format), #179 (CROWD NONE), #180 (fonts); keep #182 snooze separate until its feature and known 2.8 boot issue are confirmed on the exact build.
4. Add missing POCKET MODE and optional XP tiles / WATCH radar in isolated PRs. Protect the proven 2.8 / 3.2 renderer and #188 fix.
5. Extend browser emulator separately; compile-only T-Deck/C5/Freenove work remains pending respective boards.

No branch deletions, production merges, version bump, tag or release without explicit hardware verification and user approval.

---

# Upstream v1.40.0 feature parity — selective fork roadmap

Audited: 2026-11-04. Upstream latest published release: [v1.40.0](https://github.com/skizzophrenic/SquachWatch-CYD/releases/tag/v1.40.0), published 2026-10-23. Fork production release: **v1.22.0**, independently developed; these version numbers do not imply equivalent source trees.

**Invariants:** retain BLE OTA, external UBX/NMEA GPS and trusted UTC, SD hot-swap/VFS recovery, user-validated classic 2.8-inch ST7789 and 3.2-inch ST7798 visuals, two-way SquachMesh, enhanced WATCH/HUNT, detection rules, and existing saved settings. Never wholesale merge upstream or release unverified hardware.

## 1. User-facing features — workstream 4 (start here)

| Upstream release / PR | Change in our fork | Status |
|---|---|---|
| v1.38 | **CLOCK FORMAT 12/24-hour** in Desk Mode; use same NVS key for zone card and corner clock | Implemented on **draft fork PR #178**, CI/hardware pending |
| v1.38 | **FONTS** page: six independent 0–4 size sliders (Menus, Text, Buttons, Bubbles, Pets, XP Tiles), choice of faces where supported, on-screen sample and clipping guards | Missing; new isolated UI+Theme PR |
| v1.34 | **POCKET MODE**: screen sleeps, no accidental touch wake; deliberate BOOT/power-button wake. Decide whether scanning/logging stays on; differentiate from PIN screen lock | Missing; new hardware-gated PR |
| v1.40 | **Multi-tap SNOOZE**: 10 min → 30 min → 1 h → 4 h, commit after 3 s idle | Partially covered: our fork already has timed and ALWAYS snoozes, manager and UNSNOOZE. Add only interaction, preserve persistence |
| v1.23 | **WATCH radar** presentation and alert-dismiss behavior | Not identical to our customized WATCH alert; visual enhancement and separate regression PR |
| v1.31–v1.32 | **XP detection tiles / CLASSIC** display selection, readable counter labels | Missing toggle; preserve current status icons, WATCH/HUNT pills, and bubble clearances |
| v1.37 | **CROWD NONE** option: suppress SquachMesh visitor animations but keep messaging, alerts and peer discovery | Confirmed gap: fork crowd cycles ONE, 3–8 without NONE |
| v1.31–v1.37 | **C1iPPY**, **T0@$TY**, **M3@ty**, ball-and-chain / prison gag, unlocks, throw interactions | Optional additions; fork already has distinct SHAGGY/YETI and a STRIPES jail-ball effect. Preserve pet IDs and costumes |
| v1.27–v1.40 | Further appearance animations, costumes, auras, gestures and effects | Compare per animation; many costumes and aura already ported in fork #121/#153 |
| v1.34–v1.36 | **SEEN AT** on location-aware log rows, WiGLE positioning and wardrive UI | GPS, UTC and WiGLE *format* exist; runtime/user-facing parity requires review and hardware tests |
| v1.34–v1.39 | Battery lava gauge, sounds, volume controls, BOOT sleep/power behavior | Hardware-specific on speakers/battery boards; hide where absent |
| [upstream draft #112](https://github.com/skizzophrenic/SquachWatch-CYD/pull/112) | XL desk clock and optional default full-screen desk alerts | **Beyond published v1.40**; optional follow-up after parity, not part of v1.40 release |

## 2. General missing fixes and additions (separate from UI workstream)

| Upstream reference | Fork-specific action | Evidence |
|---|---|---|
| [#114](https://github.com/skizzophrenic/SquachWatch-CYD/pull/114) | Remove `BC:DD:C2` mislabeled Arlo (Espressif), add real Arlo prefixes, set explicit passive raw Wi-Fi / Wi-Fi picker scans | Confirmed defects on fork master |
| [#102](https://github.com/skizzophrenic/SquachWatch-CYD/pull/102) | Don't truncate type/vendor names in LOG confirm under Privacy Mode | Confirmed `privLabel(s_confirmLabel)` on fork master |
| [#115](https://github.com/skizzophrenic/SquachWatch-CYD/pull/115) | Protect *internal* SRAM on PSRAM boards: `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT` | Fork still uses `MALLOC_CAP_8BIT` for scan pressure |
| [#98](https://github.com/skizzophrenic/SquachWatch-CYD/pull/98) | Add password-protected, time-limited **SHARE LOGS** over Wi-Fi; port SD writer fixes only where applicable | No Wi-Fi log server in fork; our SD writer already has custom buffering, hot-swap, evidence |
| v1.38 | More Bluetooth bodycam, gunshot sensor, tracker and drone/glasses signature evidence | Compare and unit-test individually; don't remove confidence gates |
| v1.39 | Flipper BUSY Bar false-positive fix, calmer deauth thresholds | Port after signature audit |
| v1.39–1.40 | T-Deck LoRa/Meshtastic names, radios/channels, radar/chat and transmission improvements | Device-specific; not classic-CYD testable |

## 3. Hardware-only v1.40 parity

- RockBase ESP32-C5 / 5 GHz: existing fork draft [#142](https://github.com/retrodroid32/SquachWatch-CYD/pull/142) / [#144](https://github.com/retrodroid32/SquachWatch-CYD/pull/144).
- CYD32C capacitive + finder: existing fork drafts #128–#130.
- CYD35C capacitive + RGB + overlay: existing fork drafts #131/#143/#147.
- Freenove S3, CrowPanel, LCDWiki, T-Watch: already split into fork drafts #132–#134/#170/#122/#125–#127. Hardware acceptance pending on variants not owned.
- Upstream also supports boards such as Marauder V8 and T-Deck Plus. Treat them as new independently verified profiles, never as automatically shipping to classic CYDs.

## 4. Already in the fork — keep and regression-test

- Classic CYD GPS **UBX + NMEA**, 115200-baud live fix, UTC, coordinates/satellites, icon preferences.
- SD indicator, file browser, buffered SD logging, hot removal and remount recovery.
- Wi-Fi USE repair and double-keyboard/flicker fixes on verified 2.8/3.2 CYDs.
- Two-way SquachMesh, invites, encrypted messages, outfit sync, watch/hunt rosters, per-device timed/ALWAYS snoozes.
- Alert rules / device policies, privacy, evidence/history, WiGLE CSV formatting, frame timing instrumentation and validated costumes.
- **BLE OTA retained intentionally**: upstream removed it in one release; we must not regress.

## Verification and approval

Each independently scoped feature PR: host tests + full firmware matrix + emulator rendering, then actual 2.8 ST7789 and 3.2 ST7798 validation before approval. Hardware-only features remain compile-only unless tested on their own board. No release/tag and no merge without explicit approval. Maintain published v1.22.0 immutable; prepare the next fork release only after user acceptance.


## 2026-11-08 item 1–8 implementation tracking

| Item | Work | Gate |
|---|---|---|
| 1 | #189 Arlo OUI detection fix; #191 passive Wi-Fi surveys | New draft PRs; CI and both classic CYDs |
| 2 | #190 Privacy Mode LOG/RAWSCAN confirmation labels | New draft PR; CI and physical privacy checks |
| 3 | #178 12/24 clock | Draft; test with #188-integrated master |
| 4 | #179 CROWD NONE | Draft; paired SquachMesh hardware acceptance |
| 5 | #180 appearance font controls | Draft; TEXT/XP scaling remains intentionally partial |
| 6 | #182 tap-to-step snooze | Draft; exact-build 2.8 boot and ALWAYS/OFF persistence not fully verified |
| 7 | #192 short-BOOT Pocket Mode | New draft; test boot-strapping, screen, alerts, radios on both CYDs |
| 7 | XP tiles and WATCH radar | Not implemented; requires own UI/layout branches |
| 8 | New v1.41 multi-board emulator | Not implemented; upstream sim/Makefile includes per-board WASM/LoRa simulation targets absent from fork; isolate after reviewing simulator dependencies |

**Dependency warning:** Earlier #178–#180/#182 were authored against master before CYD32-native LCD #188. Revalidate integration on current master, not just their earlier CI. Do not wholesale replace fork SD/GPS/mesh/display implementation with upstream source. Keep untestable T-Deck/Waveshare/Freenove profiles compile-only.
