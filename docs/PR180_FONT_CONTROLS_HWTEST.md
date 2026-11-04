# Hardware validation: draft PR #180 (font controls)

Source: [PR #180](https://github.com/retrodroid32/SquachWatch-CYD/pull/180)
Base: production `master` v1.22.0, *unmerged*. This is a staged v1.40 feature port, **not** a public release.

## Required boards

- Classic 2.8-inch ESP32-2432S028R ST7789
- Classic 3.2-inch ESP32-2432S032R ST7798

Test actual portrait and landscape on **both** boards. CI simulator and source checks do not substitute for this.

## Settings navigation

1. Open `SETTINGS -> APPEARANCE -> FONTS`. Verify the six rows: MENUS, TEXT, BUTTONS, BUBBLES, PETS, XP TILES.
2. Verify pinned UP from FONTS returns to APPEARANCE, rather than MAIN; another UP returns to MAIN.
3. In both rotations, drag-scroll rows and tap +, -, or the line: the touched row alone changes.
4. Test label taps for the four supported PIXEL/SMOOTH face choices (menus, buttons, bubbles, pets).
5. Return to each row to confirm the preview shows its chosen size and face. No overlap with the bottom navigation strip.
6. Reboot **without erase-all** and confirm chosen values persist; reset them to defaults after testing.

## Layout and runtime

- **MENUS:** raise/lower one step; test Settings, long Wi-Fi SSIDs, system screens and saved-device lists. No label/value collision, off-screen line, or click-target drift.
- **TEXT:** on classic panels the size is intentionally fixed at the native readable size. The larger-screen up/down path is deferred to its board-specific implementation. Verify the page explains FIXED rather than changing silently.
- **BUTTONS:** alter size and face; verify small BACK/QWERTY buttons still fit their boxes. Test normal and Win95 button surfaces.
- **BUBBLES:** enable SMOOTH, PIXEL, SMALLER and BIGGER. Trigger normal and long Squachy dialogue and received SquachMesh chat; verify wrap widths, cursor baselines, tail, and clearing without tears or flicker.
- **PETS:** activate Shaggy/Yeti, change PETS size and face; verify captions stay in-screen, do not overlap head/status icons, and do not leave stale rectangles.
- **XP TILES:** listed but FIXED until upstream XP-style counter UI is ported in its own draft; do not assert tile-font parity yet.

For every category, verify no involuntary reboot, Wi-Fi scanning failure, SD remount regression or GPS UART stall. Use serial diagnostics/frame metrics if the display jitters.

## Baseline and acceptance

Existing installation must retain original visual sizes (all six stored steps default to **2**; all faces default to **0**). Confirm existing Wi-Fi credentials, pairings, SD files and GPS configuration remain. Use a **LAB firmware build from this exact draft commit** only after its firmware/host/emulator CI checks pass; never test with a production v1.22.0 image and conclude PR #180 worked. Do not merge or tag before explicit approval.

## LAB firmware artifact and PowerShell flashing

After **all three** Build jobs (firmware/tests/emulator) pass, open the PR's
GitHub Actions **Build** run and download the `lab-pr-180-classic-cyd` ZIP
from **Artifacts**. It contains separate `2.8-ST7789/` and `3.2-ST7798/`
firmware, bootloader and partition images, plus shared `otadata.bin`
and `BUILD-INFO.txt` with the actual checked-out commit. This is a LAB
artifact, **not** a production release or a signed BLE OTA package.

On Windows, extract it to `C:\SquachWatch-LAB\PR180` (change the path
or COM port for your PC). Flash **only the matching board directory**:

**2.8-inch ST7789:**

```powershell
$lab = 'C:\SquachWatch-LAB\PR180'
py -m esptool --chip esp32 --port COM8 --baud 460800 write-flash `
  0x1000 "$lab\2.8-ST7789\bootloader.bin" `
  0x8000 "$lab\2.8-ST7789\partitions.bin" `
  0xe000 "$lab\otadata.bin" `
  0x10000 "$lab\2.8-ST7789\firmware.bin"
```

**3.2-inch ST7798:**

```powershell
$lab = 'C:\SquachWatch-LAB\PR180'
py -m esptool --chip esp32 --port COM8 --baud 460800 write-flash `
  0x1000 "$lab\3.2-ST7798\bootloader.bin" `
  0x8000 "$lab\3.2-ST7798\partitions.bin" `
  0xe000 "$lab\otadata.bin" `
  0x10000 "$lab\3.2-ST7798\firmware.bin"
```

Writing `otadata.bin` selects the freshly flashed primary app slot even
after an earlier BLE OTA upgrade. None of these writes targets the NVS
settings partition at `0x9000`, so **do not use erase-all**. Existing
settings and pairings must remain intact. Do not cross-flash 2.8/3.2
images. These images are for USB validation only; a signed BLE OTA
workflow remains a separate test.

To capture diagnostics after flashing:

```powershell
py -m serial.tools.miniterm COM8 2000000
```

Press **Ctrl+]** to exit miniterm. Record the `BUILD-INFO.txt` commit
SHA and actual hardware PASS/FAIL results separately from CI.
