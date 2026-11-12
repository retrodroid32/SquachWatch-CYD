# SquachWatch-CYD Fonts: what each setting controls

The Fonts screen is under **Settings → Appearance → Fonts**. Each row saves its
size and face independently in NVS. Changes appear on relevant real screens
after returning to those screens; saved values survive a reboot.

| Setting | Live target | Important limitation |
| --- | --- | --- |
| MENUS | Settings rows, section headings and navigation labels | Longer entries fall back to a smaller measured size |
| TEXT | Measured information panels and diagnostics rows | Not yet all static body text throughout the firmware |
| BUTTONS | Standard vaporwave and Win95 push buttons, keyboard keys, alert buttons, WATCH/HUNT pills and navigation controls | Very narrow or fixed-height buttons may clamp to fit safely |
| BUBBLES | Squachy's speech bubbles | Long text still wraps and may use a safe fallback |
| XP TILES | Windows-style SquachMesh message card in Desk mode | Does not style all tiles; requires a received message to see it |
| PETS | Short speech lines for pet/creature features | Long lines can fall back to the safe font |

Five numeric size settings are stored **0–4**, displayed as **1/5–5/5**:
**Tiny**, **Smaller**, **Normal**, **Bigger**, **Huge**.
The common BUTTONS raster uses actual heights of **8, 11, 14, 18, and 22
pixels**, with five corresponding horizontal advances; narrow containers may
use the largest size that fully fits.

Faces: **DEFAULT** uses the category's design styling; **PIXEL** is a
blocky bitmap-style rendering; **SMOOTH** selects the smoother supported
TFT face/edge styling. Some categories have incomplete face-specific
rendering, so the appearance can be similar even if the setting is saved.

## Verification
On CYD 2.8 ST7789 and CYD 3.2 ST7798, validate all five BUTTONS steps
in both orientations on: SCAN/LOG/DESK, BLE/WIFI/BACK, the Settings
BACK/UP strip, Wi-Fi password and QWERTY keyboards, alert WATCH/HUNT/
SNOOZE, WATCH/HUNT main pills, and System Properties tabs/buttons.
Confirm no text overlap or touch-target drift, then reboot and recheck.
Run OTA partition headroom, CI, Wi-Fi USE, SD, GPS UBX 115200, privacy,
WATCH/HUNT and SquachMesh smoke tests before any merge.
