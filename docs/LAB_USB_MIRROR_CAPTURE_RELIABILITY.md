# LAB — SquachWatch USB MIRROR frame reliability and screen provenance

## Scope
This SquachWatch-only draft is stacked on PR #234 (which is stacked on PR #232). The original PR #225 and PR #233 are preserved, unchanged, and unmerged. It fixes real RGB332 **pixel capture** independent of the partial native UI v2 model. The SquachEmit EXE is reference-only and was not modified.

## What was fixed
1. Frame requests now know whether the current physical drawing source is an authentic **full-screen 8-bit sprite (source 1)**, **multi-pass recovery-band sprite (source 2)**, no available sprite (source 0), or a deliberately **direct-panel renderer (source 3)**. The CYD32 ST7798 SD_FILES browser draws directly to TFT, so it now returns `MIRROR ERROR DIRECT_PANEL_UNSUPPORTED` and NEVER transmits stale sprite bytes from another page. When framebuffer is missing, `MIRROR FRAME` returns `MIRROR ERROR NO_AUTHENTIC_BUFFER` immediately.
2. The retained full-frame snapshot is eligible only if actual pushFrame() previously presented **the same screen, rotation and transition**. A pending screenshot on a static page can use that legitimate retained display, but cannot use a stale frame after navigation.
3. A frame in progress is aborted with an explicit `MIRROR ERROR SCREEN_CHANGED` on display-source/screen/rotation transition; baseline hash is invalidated, and remote hold is cancelled. The host must discard the incomplete frame and request a fresh keyframe.
4. Changed-row transmission remains bounded and nonblocking; allow at most **four RGB332 rows / pushed frame pass**, with maximum 32 candidate rows examined. Classic CYD UART TX buffer increased to **2048 B**, while each full record is queued only if it fits without waiting. This reduces low-refresh ST7798 band-capture latency but requires physical radio performance validation. No second LCD buffer is allocated.
5. Recovery-band capture can request a new actual draw on an otherwise static CLEAR page to prevent a screenshot from waiting forever for the next cosmetic redraw.
6. Existing 6-second no-progress timeout, explicit PIN denial, `MIRROR PAUSE / RESUME` LOG priority, physical touch precedence, and old `MIRROR READY / BEGIN / ROW / END / DELTA` framing remain. No WiFi/BLE listener is added.

## Wire protocol
Send LF-terminated commands to **SquachWatch's configured USB baud**, typically 2,000,000 for lab builds, NOT automatically the 115200 of SquachEmit. Use independent COM ports.

### PC commands
```text
MIRROR HELLO
MIRROR ON
MIRROR STATUS
MIRROR FRAME
MIRROR FRAME <last-accepted-seq>
MIRROR TOUCH <x> <y> DOWN|MOVE|UP
MIRROR PAUSE
MIRROR RESUME
MIRROR OFF
```

### Device responses
```text
MIRROR CAPS 2 RGB332 FULL DELTA ROWS TOUCH PAUSE NO_FAKE_USB
MIRROR READY RGB332 320x240/240x320 TOUCH DELTA V2
MIRROR STATUS 2 <phase> <source> <bands> <appstate> <rotation> <seq> <nextrow> <height> <sentrows> <sentbytes> <age_ms>
MIRROR QUEUED <source> <bands> <appstate>
MIRROR BEGIN <seq> <width> <height> RGB332
MIRROR DELTA <seq> <base-seq> <width> <height> RGB332
MIRROR ROW <seq> <y> <base64 width authentic RGB332 bytes>
MIRROR END <seq>
MIRROR ERROR <reason>
MIRROR PAUSED
MIRROR RESUMED
MIRROR OFF OK
MIRROR TOUCH OK
```
`phase`: OFF, READY, PENDING, STREAMING, PAUSED. `source`: 0 = unavailable, 1 = full buffered, 2 = buffered recovery bands, 3 = direct TFT unsupported. `nextrow` is zero-based progress within the current image; `sentrows` and `sentbytes` are counters for this frame; `age_ms` is elapsed since acceptance while pending/streaming.

`MIRROR QUEUED` is informational and does not mean the EXE has a screenshot. Only a complete BEGIN + every ROW + matching END is a valid full image; DELTA is allowed ONLY when the PC accepted the base sequence. No invented pixels.

Machine-readable reasons include `NO_AUTHENTIC_BUFFER`, `DIRECT_PANEL_UNSUPPORTED`, `SCREEN_CHANGED`, `ROTATION_RESTART`, `NO_FRAME_REFRESH_OR_TX`, `BAD_SPRITE_GEOMETRY`, `ENCODE_OVERFLOW`, `EVIDENCE_PAUSE`, `LOCKED`, `PAUSED`, `BUSY`. Locked devices refuse commands, including TOUCH. A 6-second stalled row transfer returns NO_FRAME_REFRESH_OR_TX. The Windows app must clear partial captures on error and show unavailable rather than falsely claiming LIVE.

## Required Windows EXE handoff (NO edits here)
- Keep SquachEmit native UI v1 unaffected. SquachWatch emits authentic pixel MIRROR v2 and an **independent partial** native UI v2.
- Check the actual firmware SHA and UART baud on the connected CYD. Working LOG alone does not demonstrate MIRROR firmware was flashed. After connecting an unlocked SquachWatch send `MIRROR HELLO`, `MIRROR ON`, `MIRROR STATUS`, then `MIRROR FRAME`. Do not send OFF before END/error/timeout.
- Show status and decode only valid completed frames. `MIRROR QUEUED`/STATUS are control lines, NOT RGB rows or receiver LOG evidence. The Windows app must not pass them into the detection CSV parser.
- Use a frame timeout based on baud and actual progress (at 115200, a 320x240 full Base64 keyframe exceeds 100 KB and can exceed the old 12-second timeout). Allow at least 45 seconds at 115200 and reject stalled frames after firmware ERROR; 2Mbps recommended for responsiveness.
- Existing receiver LOG priority is preserved: PAUSE mirroring before LOG and RESUME plus full FRAME after testing; don't treat SquachEmit EXPECTED labels as receiver ACTUAL.
- For CYD32 SD_FILES (direct-panel) and other unavailable sources, show an explicit unsupported message. Native UI model is **partial**, cannot fabricate all 42 screens.

## Physical merge gate (not proven by CI)
Both CYD 2.8 ST7789 and CYD 3.2 ST7798:
1. Verify SHA/version and MIRROR HELLO/CAPS at actual UART baud; ON/READY; STATUS source 1 (full) or 2 (band); FRAME QUEUED/BEGIN/each ROW/END; rendered Windows screenshot compared to actual LCD.
2. Test CLEAR, LOG, SETTINGS, DIAGNOSTICS, WATCH/HUNT, alerts, rotated portrait+landscape and recovery-band rendering. CYD32 SD_FILES must return DIRECT_PANEL_UNSUPPORTED, NEVER a false screenshot.
3. Switch screen mid-keyframe: host should discard partial with SCREEN_CHANGED. Test static screens, multiple frames/deltas, disconnected USB, FIFO backpressure and no-progress error.
4. Remote click, press, swipe, scroll, physical touch priority, orientation coordinate mapping, PIN lock/session cancellation, privacy redaction.
5. Repeat automated SquachEmit transmission tests with receiver LOG evidence; no missed detections, invented ACTUAL, BLE/WiFi stall, GPS FIX/UTC loss, SquachMesh disruption, SD errors or LCD flicker. Run >=30 minute USB/RF soak.
6. Exact-commit CI across firmware matrix and >=8192 bytes final OTA headroom, then LAB-only release. No merge or production release without owner approval.
