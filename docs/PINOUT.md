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

## External GNSS input (CYD 2.8" and 3.2")

The current firmware enables real UBX and NMEA GPS reception on the tested
classic ESP32 CYDs. Connect **GPS module TX → GPIO35**, **GND → GND**,
and power from a supply rated for the module. The GPS module must transmit
at **115200 baud**. UART2 is RX-only: leave the GPS module's RX pin
unconnected. GPIO35 is input-only. Do not wire GPS TX to UART0
(GPIO3/RXD, shared with the USB console on some 3.2" boards).

## Other header GPIOs

GPIO 0, 1, 3, 12, 13, 16, 17, 22, 26, 34, 37, 38 may be exposed
on different board revisions. Confirm pin availability against the exact
hardware before using them. GPIO35 is reserved for GPS RX on supported
SquachWatch-CYD firmware.

## Sources

- [witnessmenow/ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) (MIT)
- [Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display](https://github.com/Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display)
- Sunton schematic for the ESP32-2432S028R (publicly available in
  the witnessmenow wiki).
