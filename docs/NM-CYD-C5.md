# RockBase NM-CYD-C5 (ESP32-C5) — experimental

A 2.8" CYD built on an **ESP32-C5** instead of an Xtensa ESP32. Same glass as
the classic board (ST7789 240×320, resistive XPT2046, microSD), same footprint
and mounting holes, different silicon underneath:

| | ESP32-2432S028R (classic CYD) | NM-CYD-C5 |
|---|---|---|
| Core | Xtensa LX6, **dual**-core 240 MHz | RISC-V, **single**-core 240 MHz |
| WiFi | 2.4 GHz, 802.11 b/g/n | **2.4 + 5 GHz**, 802.11 a/n/ac/**ax** |
| Bluetooth | BLE 4.2 + **Classic** | BLE 5.3, **no Classic** |
| 802.15.4 | none | **Zigbee 3.0 / Thread** |
| Flash | 4 MB | **16 MB** |
| PSRAM | none | **8 MB** |
| Backlight | GPIO21 | **GPIO25** |
| Status light | 3 PWM pins (4/16/17) | **one WS2812 on GPIO27** |
| Touch bus | its own SPI | **shares the display's SPI** |

Vendor board support, including the schematic, is at
[RockBase-iot/NM-CYD-C5](https://github.com/RockBase-iot/NM-CYD-C5).

## Build and install

```bash
pio run -e nm-cyd-c5 -t upload
```

This environment carries **its own `platform` line**. The ESP32-C5 did not
exist when Arduino core 2.0.14 was cut, so it builds on Arduino 3.3.12 /
IDF 5.5.5 while every Xtensa board stays on the pinned espressif32@6.5.0.
Nothing about any other board changes; `pio run` across all 22 environments is
the check for that.

The board has **two USB-C ports**: the ESP32-C5's native USB, and a CH340
USB-to-UART. Either will flash it, but they are **not** interchangeable for the
serial console, and getting this wrong costs an hour.

This build sets `ARDUINO_USB_CDC_ON_BOOT=0`, so `Serial` is UART0 on GPIO
11/12, which is the **CH340** port. That is the port most people plug into (it
is the one that enumerates as a COM port with the CH340 driver) and the one the
web flasher talks to. Plugged into the *native* port instead, the board runs
perfectly and prints nothing at all, which is indistinguishable from a board
that failed to boot.

If you want the console on the native port, flip that flag in
`boards/nm-cyd-c5.json` to 1 and rebuild.

### Do not flash the merged image at 0x0

`pio run -e nm-cyd-c5 -t upload` is safe and is what you should use. If you
flash by hand with esptool, flash the **four parts**, not
`firmware.factory.bin`:

```
esptool --port COM5 --baud 460800 --chip esp32c5 write-flash \
  0x2000 bootloader.bin  0x8000 partitions.bin \
  0xe000 boot_app0.bin   0x10000 firmware.bin
```

`firmware.factory.bin` is those same four padded into one contiguous image
from 0x0, so it also covers **0x9000–0xe000, which is NVS**. Flashing it wipes
settings, the touch calibration, the PIN and its duress twin, mesh pairings and
Squachy's stats. The symptom is the board running the five-target calibration
again as though it were brand new, which reads as "the calibration did not
save" rather than as "the flash erased it". The four-part write leaves the gap
alone and settings survive.

## Pinout

From RockBase's `Demos/Platformio/nm-cyd-c5/`. Display, touch and SD all share
one SPI bus.

| Device | SCK | MISO | MOSI | CS | DC | RST | BL |
|---|---|---|---|---|---|---|---|
| Display (ST7789) | 6 | 2 | 7 | 23 | 24 | chip RST | 25 |
| Touch (XPT2046) | 6 | 2 | 7 | 1 | — | — | — |
| SD card | 6 | 2 | 7 | 10 | — | — | — |

There is **no touch IRQ line**. The classic CYD wires one to GPIO36; this board
does not have one at all, which is why the environment takes `-DRLPHANTOM_R=1`
for the raw shared-bus touch path rather than the classic CYD's.

RGB LED: one WS2812 on GPIO27 (GRB). I²C is on 8/9. The GPS header is a
low-power UART on 4/5.

> RockBase ship two `pins_arduino.h` variants that **disagree** about I²C:
> `Demos/Platformio/nm-cyd-c5/pins_arduino.h` says SDA 4 / SCL 5, while
> `pinouts/nm-cyd-c5.h` and `connections.md` say SDA 9 / SCL 8. This firmware
> uses no I²C on this board, so it does not matter here — but check the
> schematic before adding a sensor.

## What this board cannot do, and what it does better

**Cannot: Bluetooth Classic.** The C-series has no Classic radio. Today that
costs nothing, because `SKIMMER` detection is a BLE advertised-name match on
every board and BT Classic inquiry is explicitly not enabled in v1.0 (see
[DETECTIONS.md](DETECTIONS.md) — it conflicts with NimBLE on a single radio).
If the v1.1 task ever lands, **this board will not be able to run it.** That is
silicon, not software.

**Cannot: an on-device crash backtrace.** RISC-V has no windowed register ABI,
so a backtrace cannot be walked on the device; the IDF stores a raw stack dump
for a host to decode instead. The crash screen shows task, PC, cause and fault
address here, and no frames. Nothing is broken when that section is empty.

**Better, and currently unused: 5 GHz and 802.15.4.** `DetectionEngine` hops
2.4 GHz channels only, which is all any Xtensa board can do. This chip can also
see 5 GHz networks and hear Zigbee/Thread traffic. Several things in
[DETECTIONS.md](DETECTIONS.md) live on 5 GHz that no CYD can currently see.
That is a real opportunity and deliberately **not** part of this port: the port
is for parity first.

## Validation

Nothing below is confirmed yet. A box is only ticked from the board itself.

### Built, not yet run on hardware
- [x] `pio run -e nm-cyd-c5` links a valid ESP32-C5 image
- [x] All 22 environments still build (no Xtensa board regressed)
- [x] `make -C test` — the host suite passes
- [x] `make -C test crypto` — the **real** `meshcrypto.cpp` against the host's
      mbedtls 3 reproduces the golden vectors, and fails under three mutations

### Confirmed on the board
Flashed and captured over the CH340 port on 2026-09-28.

- [x] **esptool identifies it**: `ESP32-C5 (revision v1.0)`, 16MB flash,
      "Wi-Fi 6 (dual-band), BT 5 (LE), IEEE802.15.4, Single Core + LP Core"
- [x] **Boots**, and prints the SquachWatch banner on the CH340 port
- [x] **PSRAM works**: `PSRAM found: yes (8388608 bytes)`. Worth stating
      explicitly because the bootloader prints
      `E MSPI Timing: Failed to allocate dummy cacheline for PSRAM memory barrier!`
      on every single boot. That is
      [arduino-esp32 #12587](https://github.com/espressif/arduino-esp32/issues/12587),
      open since 2026-05-12, present on stock sketches, and nothing to do with
      this firmware. All 8 MB is there afterwards. The print above is how this
      board answers that for itself rather than trusting the issue thread; no
      CYD build asks for PSRAM anyway.
- [x] **No stray GPIO errors.** The first flash produced three per boot --
      `Invalid IO 32 selected`, `perimanGetPinBus(): Invalid pin: 32`,
      `IO 25 is not set as GPIO` -- from the shared pre-init block driving 27
      and 32 HIGH. Fixed with an `NM_CYD_C5` branch that drives only `TFT_BL`,
      and confirmed gone by re-capture.
- [x] **Reaches touch calibration**: `Touch: no five-target calibration yet`,
      which means the display stack initialised and the board is asking for
      input.
- [x] **No ADC errors.** `randomSeed(analogRead(34))` fails here -- GPIO34 does
      not exist on a C5 -- so the seed was 0 and the digital rain opened on the
      same frame every boot. `NM_CYD_C5` now joins the boards that seed from
      `esp_random()`, which is the fix already in the tree for the S3s.
- [x] **The WiFi sniffer is NOT deaf.** This was the one to worry about (see
      the checklist item below, and PR #10): `wifi 581` frames and climbing,
      alongside `ble 69/s` and 1,908 adverts, in a single 75-second capture.
      The `delay(10)` in `detection.cpp` holds on this chip and core.
- [x] **Mesh crypto self-test passes on the device**: `[meshtalk] crypto
      self-test PASS`. That is the mbedtls 2 -> 3 migration confirmed by the
      board itself, against frames built by an independent implementation.
- [x] **Detections fire.** `det 5` in one capture, from ambient traffic alone.
- [x] **Performance**: 67-72 fps, `loop 69/s`, heap flat around 80 KB free
      across the capture, largest block 8.2 MB.
- [x] **SD card absence is handled**: `[sd] no card, or it did not answer:
      nothing will be logged`. A card still needs testing.

### Still to confirm on the board
Ordered so that a failure early explains the failures after it.

- [ ] **Display**: picture, right way up, correct colours, no inversion
- [ ] **Display**: picture, right way up, correct colours, no inversion
- [ ] **Backlight**: the brightness slider actually dims (proves GPIO25)
- [ ] **Touch**: registers, and lands where you press after calibration
- [ ] **Status light**: the boot sweep runs (proves the WS2812 on 27, and that
      it is not silently doing nothing)
- [ ] **WiFi AP count matches a phone's.** The sniffer is confirmed alive
      above; this is the stricter version -- that it sees as many APs as a
      phone standing in the same place, not merely more than zero. Not
      optional and not obvious: on the ESP32-S3
      a race between `esp_wifi_deinit()` and the sniffer's `esp_wifi_init()`
      brought the driver up **with the receiver off** — scans returned zero
      networks and the sniffer saw no frames while BLE worked perfectly and
      the board's own AP was visible from a phone. The `delay(10)` that fixes
      it is already in `detection.cpp`, but it was tuned on an S3 on core 2.
      This is a new chip on a new core. **Check the AP count against a phone
      standing in the same place.**
- [ ] **BLE scan sees adverts** (any phone with Bluetooth on will do)
- [ ] **Mesh crypto self-test passes**: the console prints
      `[meshtalk] crypto self-test PASS`. This is the on-device proof of the
      mbedtls 2→3 migration.
- [ ] **A real detection fires.** Per [BUILD.md](BUILD.md): any BLE device
      named `HC-05`/`HC-06` trips `SKIMMER`; walking past a Ring/Wyze/Nest/Eufy
      camera trips `CAMERA`; an AirTag trips `AIRTAG`.
- [ ] **SD card**: a FAT32 card gets `squachwatch-<day>.log` written to it
      (proves SD on the shared display bus)
- [ ] **Settings persist** across a power cycle (proves the 16 MB partition
      table kept NVS where the bootloader expects it)
- [ ] **SquachMesh**: two boards see each other. Needs a second device.
- [ ] **OTA**: an update applies and the board still boots (proves both 6 MB
      app slots and the ported ECDSA signature check)
- [ ] **Soak**: left running for an hour without a reset or a heap collapse

## Performance, and why the SPI frequency does nothing

The port renders correctly but more slowly than a classic CYD: a full 320x240
frame takes about 88 ms, roughly 0.37 ms per row. Two things are worth knowing
before anyone tries to tune it.

**`SPI_FREQUENCY` is not connected to anything on the path this firmware
uses.** Built at 20, 26.6, 40 and 80 MHz, the board came up with
`SPI_CLOCK_REG = 0x00002001` every time and a 240-row frame took 88.8, 86.6,
83.2 and 88.3 ms -- the same frame, four times, within noise. RockBase's C5
processor port mentions `SPI_FREQUENCY` exactly once, inside `initDMA()`'s
`spi_device_interface_config_t`, and `frame_push.cpp` never calls `initDMA` --
it drives the peripheral registers itself. So the bus keeps whatever divisor it
came up on. Per the C5 TRM ch.33.7,
`f = f_clk_spi_mst / ((SPI_CLKCNT_N+1)(SPI_CLKDIV_PRE+1))`, and `0x00002001` is
`CLKCNT_N = 2`, `CLKDIV_PRE = 0`: a fixed divide-by-three.

**The SPI_UPDATE latch is not optional.** The C5's GPSPI2 stages its
configuration registers and needs `SPI_UPDATE` raised, and seen to clear,
before `SPI_USR` starts a transfer. Without it the panel still initialises,
answers its ID registers, reports display-on and 16 bits per pixel, and the
firmware reports 70 fps -- while the glass shows dashed, torn runs with most
rows missing. `frame_push.cpp` does this once per span, which is where the TRM
puts it: the burst length is configuration and must be latched, the
`SPI_W0..W15` data buffer is not and does not.

## Deliberately not claimed
- Display overclock. There is no `-fast` variant for this board on purpose:
  an out-of-spec panel clock is a poor thing to discover on hardware nobody
  has soaked. The vendor's own setup asks for 20 MHz; this build uses it.
