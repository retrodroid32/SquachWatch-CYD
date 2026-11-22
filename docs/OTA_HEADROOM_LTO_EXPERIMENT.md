# OTA flash headroom: isolated LTO benchmark (2026-11-13)

Status: DRAFT / UNMERGED / NOT HARDWARE VALIDATED.

## Problem

The classic CYD OTA application partition is 1,982,464 bytes, and the mandatory
minimum *final firmware.bin* reserve is 8,192 bytes. The GUI/detection branches
are only tens to hundreds of bytes short on ILI9341, leaving no practical growth
room even when a build barely passes. Never relax the reserve, switch to a
single-app partition, silently remove functions, or use PlatformIO's ELF-based
size estimate in place of the final binary validation.

Recent reference CI results (these are **other branches**, not master numbers):

| Build | ST7789 | ILI9341 | ILI9341-fast | ST7798 |
|---|---:|---:|---:|---:|
| PR #224 HUNT R12 free bytes | 8,368 | 8,176 | 8,112 | 13,552 |
| PR #226 category pills free bytes | 8,304 | 8,128 | 8,048 | 13,664 |

Reference PR runs: 43371664608 and 43371475354.

## Experiment and expected evidence

Apply `-flto` to firmware compile/link flags via `extra_script.py` on a branch
from master. The existing compiler target, pinned PlatformIO version, dependency
versions, all code paths, and OTA partition table stay untouched. This is a
**measurement**, not a claim that LTO is safe on the physical ESP32 devices.

The Build job must:
1. Complete every profile in `tools/board_matrix.py pio-args ci`.
2. Pass host tests and emulator tests.
3. Report the sizes from `tools/check_app_image_sizes.py` for every release
   profile, including both ILI9341 clock profiles.
4. Record the *absolute final-image byte count and margin* against an otherwise
   identical non-LTO build. Prefer several KiB of recovered room, not a 16-byte
   band-aid. Keep the hard >=8192-byte gate.
5. Not dispatch a LAB flasher or production firmware just because this
   experimental branch builds.

## Hardware gate before adoption

If CI demonstrates meaningful improvement, test LAB builds **only with explicit
approval and after Build is green**. Cover 2.8-inch ST7789 and 3.2-inch ST7798,
portrait/landscape, touch mapping, rapid UI redraw, active BLE/Wi-Fi discovery,
manufacturer fields, WATCH/HUNT, new and repeated sightings, GPS UBX 115200,
SD remove/reinsert/log access, Wi-Fi USE, OTA, privacy/PIN, and bidirectional
SquachMesh. For classic ILI9341 variants, CI size is checked even though
physical validation may need additional hardware.

LTO can alter compiler inlining and expose undefined behavior, including
potential ISR/RTOS timing differences. If CI fails or regressions appear,
revert the compile/link flags on this experiment branch. Avoid touching the
two in-progress UI PRs until the test is understood.

No merge, production tag/release, or deletion of existing development branches
without the owner's express permission.
