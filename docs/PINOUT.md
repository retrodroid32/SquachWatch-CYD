# Pinout — ESP32-2432S028R ("CYD")

Pin assignments used by SquachWatch-CYD. Verified against the most
common Sunton "USB-C" revision of the board. Other CYD revisions
may differ — check your board's silkscreen and the
[witnessmenow/ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
repo for variants.

## TFT (ILI9341 / ILI9342C) — VSPI bus

| Signal | GPIO |
|---|---|
| MOSI   | 27   |
| SCLK   | 14   |
| CS     | 15   |
| DC     |  2   |
| RST    |  4   |
| BL (backlight, PWM) | 21 |

## Touch (XPT2046) — HSPI bus

| Signal | GPIO |
|---|---|
| CS     | 33   |
| IRQ    | 36   |
| MOSI   | 32   |
| MISO   | 39   |
| SCLK   | 25   |

## SD card — VSPI bus (shared with TFT)

| Signal | GPIO |
|---|---|
| CS     |  5   |
| MOSI   | 23   |
| MISO   | 19   |
| SCLK   | 18   |

The SD card shares `MOSI` and `SCLK` with the TFT. The firmware
selects between them via their respective `CS` lines. The SD card
is optional — if no card is inserted at boot, the firmware just
skips SD logging and everything else works.

## Unused GPIOs (free for future use)

GPIO 0, 1, 3, 12, 13, 16, 17, 22, 26, 34, 35, 37, 38 are exposed
on the CYD's GPIO header but not used by SquachWatch-CYD v1.0.

## Sources

- [witnessmenow/ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) (MIT)
- [Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display](https://github.com/Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display)
- Sunton schematic for the ESP32-2432S028R (publicly available in
  the witnessmenow wiki).


## Optional GPS / GNSS firmware

GPS-capable CYD builds use a dedicated UART2 pair that does not overlap the
display, touch, SD or status-light assignments used by this fork:

| Signal | CYD GPIO |
|---|---:|
| GPS module TX -> ESP32 RX | 35 |
| GPS module RX <- ESP32 TX | 26 |

GPIO35 is input-only on the ESP32, which is intentional here: it receives the
GNSS module's serial output. The GPS firmware defaults to 115200 baud.

GPS is a compile-time/web-flasher option, not a required peripheral. The normal
40 MHz and 80 MHz firmware remain available without GPS, and the GPS checkbox
can be combined with either display-clock choice. GPS builds append
`gps_epoch,lat,lon,alt_m,sats,hdop,gps_age_ms` to SD detection rows and leave
the fixed-size BlackBox format unchanged.
