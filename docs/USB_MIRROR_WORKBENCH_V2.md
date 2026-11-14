# SquachWatch USB Workbench mirror v2 (LAB / unmerged)

## Design decision
Keep authentic RGB332 sprite capture from draft PR #225, but replace blocking full-frame serial bursts with bounded row transfers and optional change-only DELTA frames. This is based on current SquachWatch master. PR #225 and SquachEmit PR #37 remain unchanged. SquachEmit's Windows-native UI ON/GET/STATE screen must remain independent.

## Exact USB serial protocol

The device and EXE share the existing serial UART at the firmware profile's configured SERIAL_BAUD; there is no automatic baud negotiation. A matching 2 Mbps serial setting is preferred. At 115200, keyframes can take over 12 seconds, so increase the EXE's timeout to at least 45 seconds and use progress tracking. All messages are LF-terminated ASCII, with row pixels encoded as strict base64.

Host commands:
- MIRROR HELLO: request capability negotiation; replies MIRROR CAPS 2 RGB332 FULL DELTA ROWS TOUCH PAUSE NO_FAKE_USB. Other boards reply MIRROR CAPS 2 UNSUPPORTED_BOARD.
- MIRROR ON: enable session and clear previous baseline; replies MIRROR READY RGB332 320x240/240x320 TOUCH DELTA V2 (backward-compatible READY prefix).
- MIRROR FRAME: request full keyframe.
- MIRROR FRAME 12: request delta only if 12 is the device's most recently committed sequence. Otherwise the device sends a full keyframe.
- MIRROR TOUCH 143 80 DOWN: inject mouse button-down using current physical screen pixel coordinates.
- MIRROR TOUCH 155 87 MOVE: update a held press for drag/swipe.
- MIRROR TOUCH 155 87 UP: release the press.
- MIRROR PAUSE: abort partial capture, release remote press and suspend image streaming before LOG evidence.
- MIRROR RESUME: resume, invalidate the previous baseline and require full keyframe for resync.
- MIRROR OFF: disable session and remote press.

Device image records:
- MIRROR BEGIN <seq> <width> <height> RGB332: full frame; all physical rows must follow.
- MIRROR DELTA <seq> <base-seq> <width> <height> RGB332: changed-row delta based on a previously committed and explicitly acknowledged sequence.
- MIRROR ROW <seq> <y> <base64>: exactly width RGB332 bytes; y is actual physical screen row, not recovery-band offset.
- MIRROR END <seq>: commit marker. The Windows decoder MUST validate sequence, orientation, row completeness, row uniqueness, and DELTA base before showing pixels.
- MIRROR ERROR <reason>: discard any unfinished image and resynchronize with a full keyframe. Reasons include DISABLED, PAUSED, BUSY, LOCKED, ROTATION_RESTART, EVIDENCE_PAUSE, NO_AUTHENTIC_BUFFER, BAD_SPRITE_GEOMETRY, ENCODE_OVERFLOW, BASE_SEQUENCE, COMMAND, NO_FRAME_REFRESH_OR_TX (no valid framebuffer refresh / USB backpressure for 6 seconds).
- MIRROR PAUSED, MIRROR RESUMED, MIRROR OFF OK, MIRROR TOUCH OK: state/control acknowledgments. MOVE omits ACK to reduce serial noise.

Example:
    PC: MIRROR ON
    CYD: MIRROR READY RGB332 320x240/240x320 TOUCH DELTA V2
    PC: MIRROR FRAME
    CYD: MIRROR BEGIN 1 320 240 RGB332
    CYD: MIRROR ROW 1 0 <320 real RGB332 bytes in base64>
    CYD: ... remaining physical rows ...
    CYD: MIRROR END 1
    PC: MIRROR FRAME 1
    CYD: MIRROR DELTA 2 1 320 240 RGB332
    CYD: ... changed rows, if any ...
    CYD: MIRROR END 2

If the host loses END or a row, request a full keyframe. Never apply a delta to a different base. After disconnect/reset/rotation, clear cached live status until a complete real frame is received.

## Performance, provenance and security

- Captures use only real 8-bit RGB332 pixels associated with the firmware's actual sprite-to-LCD push. Nothing is invented by the board or host when an authentic buffer is unavailable.
- Capture scans at most 32 rows and transmits at most 2 changed rows per render pass; serial sends a whole line with one nonblocking write and available TX space. UART TX buffer reserve is 1024 bytes for classic CYDs.
- Two fixed 480-entry 32-bit hash arrays use about 3840 static bytes, not a duplicated full framebuffer. For recovery-band rendering, only the currently pushed band is read; a multi-pass frame may take longer.
- Physical touchscreen wins if local and remote presses happen together. Remote touch follows native rotation/hit-test/gesture flow, expires after 1.8s without another MOVE, and cancels on PIN lock, PAUSE, OFF.
- Serial command parser rejects all MIRROR commands while PIN-locked. Streaming itself also checks locked state before sending any image.
- Firmware pauses mirror before LOG evidence and never synthesizes detection success. Keep transmitter expected classification separate from SquachWatch observed LOG type/MAC. Missing actual evidence is UNVERIFIED, not PASS.
- No Wi-Fi/BLE service is added; USB only.

## Required changes in the separate SquachEmit Windows EXE session (DO NOT do here)

SquachEmit PR #37 already has a MirrorAssembler that decodes RGB332 BEGIN, DELTA, ROW, END and renders each complete image, but its receiver command path still always requests plain MIRROR FRAME and turns off receiver mirroring during testing. Update there:
1. On receiver COM connect, send MIRROR HELLO then MIRROR ON, automatically show live receiver canvas alongside existing independently rendered SquachEmit screen.
2. Once a complete frame N is displayed, request MIRROR FRAME N for the next delta. On sequence error, reconnect or request full MIRROR FRAME.
3. During each actual receiver LOG snapshot, prioritize MIRROR PAUSE and LOG on the same receiver serial command queue, then MIRROR RESUME and full FRAME. Keep other times actively mirroring even during automated testing; avoid simultaneous LOG/mirror writes.
4. Do not parse MIRROR rows as LOG. Receiver LOG must be collected through existing strict row/type/MAC parser. Expected transmitter classifications must never count as observed receiver detections.
5. Use adaptive frame timeout/polling for 115200 vs 2 Mbps, bounded in-flight requests, resync after orientation/COM loss, UP on pointer cancel, correct orientation-aware canvas mapping and an explicit unsupported-buffer message.
6. Do not replace or change SquachEmit's working UI ON / UI GET / UI STATE / UI TOUCH native-rendered screen.

## Physical acceptance before merge

Required for both CYD 2.8 ST7789 and 3.2 ST7798, portrait and landscape:
1. USB handshake / full image matching actual display (menus, pills, fonts, graphs, gauge, diagnostics, alerts).
2. Native click, long hold, drag, swipe and list scroll; physical touch updates EXE; simultaneous local-touch priority.
3. Rotation changes screen geometry and triggers keyframe resync; no missing BACK hitboxes or stale pixels.
4. PIN lock prevents capture and remote control; USB reset/disconnect clears stale screen; reconnect recovers.
5. Automated transmitter suite continues with exact, uncorrupted receiver LOG evidence, no falsely reported signal.
6. BLE/WiFi scans, GPS fix/sats/UTC, SD cards, WATCH/HUNT, SquachMesh messages, OTA, WiFi USE, active scanning and rotation redraw checks.
7. Thirty-minute high-rate USB and RF soak; watch radio detection latency, memory, frame time, UART buffer pressure and display flicker with MIRROR ON/OFF.
8. Full CI host/emulator/firmware board matrix and hard >=8192-byte real OTA image reserve, no bypass.

DRAFT only. Do not merge, tag, publish production release, or delete branches without owner approval.
