# LAB verification: PR #129 + #130 + #147 (classic CYD regression)

This **unmerged, untagged LAB integration** is branched from draft #147, with the
current #129 browser finder and #130 flash helper staged on top. It does **not**
publish CYD32C or CYD35C firmware to the public flasher and does not update the
existing v1.22.0 production release.

## Downloads and interfaces (after LAB Actions succeeds and Pages deploys)

- LAB firmware picker: https://retrodroid32.github.io/SquachWatch-CYD/?lab=1
- **PR #129 isolated** browser preview: https://retrodroid32.github.io/SquachWatch-CYD/flasher-pr129.html
- GitHub Actions LAB run will contain a downloadable
  `pr129-130-147-cyd-verification` artifact with APP-ONLY firmware for
  `cyd` (2.8 ST7789) and `cyd32-st7798` (3.2 ST7798).
- The ordinary production flasher index and its root `boards.json` are mirrored
  from the already-published release; the PR #129 page is **separate/unlisted**.

## PR #129: finder test (no flashing required)

Open the isolated preview page on a PC. Enter each printed code and check:

| Code | Expected |
|---|---|
| `ESP32-2432S028R` / `JC2432W328C` | 2.8 ST7789 default; white-screen fallback guidance |
| `E32R28T` | older 2.8 ILI9341; no 80 MHz |
| `ESP32-2432S032R` / `E32R32P` | 3.2 ST7798 / ST7789-compatible |
| `ESP32-32E` | ambiguous size warning; no automatic firmware choice |
| `E32R35T` | 3.5-inch resistive warning; **not** a 2.8 firmware |
| `ESP32-2432S032C` | recognized as *unreleased/unverified*; **no alternate firmware selected** |
| nonsense code | unknown-board warning |

If the experimental 80 MHz box is checked on a different profile,
enter a board code; auto-selection must **clear** the box.

**Do not install** an ILI9341 image on an ST7789 display to test the finder.
Selecting a displayed option is sufficient for this PR.

## PR #130: flash-helper safety (PC-only)

The helper is `tools/flash_known.py` in this LAB branch.
From the repository root in PowerShell:

```powershell
py -m unittest discover -s tools -p test_flash_known.py -v
```

Expected: eight tests pass, without communicating with any board.

`py tools/flash_known.py COM8 --check` is safe only if you know COM8 is
the intended ESP32 port. It does not flash. Unlisted MACs are deliberately refused.
Two known CYD32C MAC IDs print their identification with `--check` but
cannot be flashed without explicit `--allow-experimental`. **Do not use**
that override for an unverified CYD32C.

## PR #147: classic 2.8 + 3.2 regression and CYD35 limitation

Flash each classic board using the matching LAB entry in `?lab=1`.
**Do not erase all flash**; use the web-flasher manifest, which writes
bootloader/partition/app regions separately and preserves your NVS settings.

Test each board for a complete display and working touch, **portrait and
landscape**, a centered toast over the main screen, toast placement on
Log/scan screens, diagnostics navigation, and Wi-Fi/SD/SquachMesh basic stability.
Check any available time-zone selection card only if it naturally appears;
do not clear NVS or erase saved pairings solely to create first-run state.

`#147` fixes the **3.5-inch CYD35C half-height framebuffer** overlay order.
The classic CYDs can only validate nonregression. Full acceptance still needs
a real CYD35C displaying toasts and time-zone cards across both bands, with
fallback and rotations. CYD35C is **compile-only** in this LAB.

## Build integrity

The LAB release workflow runs the three targeted verification suites plus
compiles regular released profiles and the unlisted `cyd35c` environment.
The Pages deployment retains production assets unchanged, adding LAB binaries
and an unlisted PR129 browser preview. LAB builds are not production tags,
and no open feature PR is merged by this workflow.
