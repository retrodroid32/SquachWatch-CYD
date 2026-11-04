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
