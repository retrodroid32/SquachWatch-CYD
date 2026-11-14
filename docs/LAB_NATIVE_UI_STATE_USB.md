# LAB: SquachWatch native UI-state USB transport

This draft adds the **transport foundation** for a SquachEmit-style, PC-rendered
SquachWatch screen. It does **not** yet implement a pixel-perfect native
SquachWatch renderer in the Windows EXE. SquachWatch has 42 application states,
with their own menus, text, animations and transient overlays. A Windows
renderer must explicitly cover those states and their stateful contents before
it can replace the authentic RGB332 mirror from PR #225.

## Commands (USB serial, line-oriented)

Only accepted when the on-device PIN is unlocked. No network service is
created. The feature is disabled by default.

- `UI ON` — enables telemetry and responds `UI READY 1 SQUACHWATCH TOUCH STATE`.
- `UI GET` — requests the next state message.
- `UI TOUCH <x> <y> DOWN|MOVE|UP` — injects an on-screen gesture via the
  existing physical touchscreen/hit-testing path. Coordinates use current
  displayed orientation and are validated against the displayed geometry.
- `UI OFF` — ends the UI session; replies `UI OFF OK`.
- `UI STATE 1 <screen_id> <width> <height> <rotation> <uptime_ms>` — opt-in,
  at most 5 Hz; `screen_id` is the numeric `AppState` in `include/state.h`.

**Security:** any PIN lock revokes the UI session and pixel mirror session.
State transport excludes PINs, device identifiers, raw detections, and
other user data. No USB control is accepted when locked.

**Concurrency:** Unlike `MIRROR FRAME`, UI state messages are small and do
not stream LCD pixels; at most one per 200 ms, and skipped when the serial
transmit buffer cannot accept a short message. This is intended to share the
serial port with receiver `LOG` evidence without turning logging off.

**Important:** The existing `MIRROR ON/OFF/FRAME/TOUCH` protocol is retained
as a separate, opt-in authentic LCD-pixel fallback. On an unsupported,
low-memory or direct-panel rendering path, do not invent missing pixels.

## Windows EXE integration

This firmware branch alone cannot make the Windows EXE draw SquachWatch.
The **SquachEmit** Workbench's SquachWatch role currently understands
`MIRROR ...` pixel messages, whereas its SquachEmit role uses `UI ...` state
messages. The desktop must gain an independent SquachWatch native renderer
that consumes the `UI READY` and `UI STATE` records, drives `UI TOUCH`,
and represents every supported screen and changing content. Only then
should it switch the receiver from `MIRROR FRAME` to `UI ON`.

A numeric screen ID is **insufficient** to reproduce most SquachWatch
screens, including detections, log rows, settings choices, graphs and
alerts. Future follow-up state messages will need explicit view models
and receiver-driven data, with a locked/privacy-aware contract. Do not
advertise this draft as equivalent to the existing SquachEmit native UI.

## Verification

- Build both CYD 2.8 ST7789 and CYD 3.2 ST7798 firmware.
- Unlock and confirm UI ON/READY, five updates a second, UI GET, UI OFF.
- Rotate screens and confirm current width, height, rotation, and screen ID.
- Click, hold, drag, release; confirm same physical hit-tests.
- Enable PIN lock during an active session; verify all subsequent commands
  are refused until physical unlock.
- Check serial LOG concurrently with telemetry and RF scanning.
- Validate no frame drops, memory regressions, or GPS/SquachMesh interference.
- Verify the authentic pixel mirror still functions on supported buffers.
- Finally test every PC-rendered screen against both physical CYD displays.

Do not merge or publish production firmware without owner approval and
physical validation.
