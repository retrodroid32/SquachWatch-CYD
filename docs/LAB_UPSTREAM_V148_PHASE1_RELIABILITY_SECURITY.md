# Upstream v1.48 Phase 1 — security and detection reliability (LAB R1)

Comparison: upstream skizzophrenic/SquachWatch-CYD v1.48.0; fork master cc153909e7c0e0058da34e8f6b60f00e4e44db21.
This draft does not merge upstream. Hardware validation pending.

## Included confirmed gaps
1. Upstream v1.45: creating a first PIN must set LOCK AT BOOT. Preserves an explicit existing user's Off setting when replacing a PIN. Tested using persistent Preferences across Security::begin().
2. Upstream v1.45: accept subtype 10 disassociation frames in addition to subtype 12 deauthentication frames. Retain existing fixed-size queue and evidence semantics.
3. Upstream v1.45: allow threshold deauth flood alerts in first 15 seconds after boot; initial zero fire timestamp means never-fired, not within cooldown.

## Deferred and still to audit
- Upstream alert queue/missed alerts/duplicate event guards versus our customized Watch/Hunt/alert sequence.
- Sender-based and multi-source broadcast deauth thresholds require queue/schema change and captured hardware test vectors; existing fork currently counts global frame window.
- v1.44 scan-memory recovery and v1.45 radio memory optimization versus existing GNSS/NimBLE raw scan patches.
- Duress wipe for LoRa DM, SquachMesh, replay and contact namespaces: verify existing NVS before destructive changes.
- OTA rejected-image SWITCH TO protection versus custom OTA stack.
- All v1.45 memory reductions deferred to Phase 2.

## Hardware verification checklist
- CI: host security test, Python guard, PC emulator, full firmware matrix and >=8192 B final OTA reserve.
- CYD2.8 & CYD3.2: fresh PIN then reboot must lock; unlock; explicit boot lock Off; replace PIN; reboot stays Off.
- Controlled isolated deauth/disassoc events must not alert. Controlled >=6 management frames within 3 s must alert when type enabled, including first 15 s after boot. Verify cooldown and avoid false positives.
- Wi-Fi and BLE, GPS, SD, SquachMesh, display rotation, Watch/Hunt, heap largest internal contiguous block and queue drops.
- Physical hardware results not yet available; no blind merges or production releases.
