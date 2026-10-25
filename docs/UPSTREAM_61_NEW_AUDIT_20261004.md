# Upstream delta audit — 61 new commits since the original 53

Snapshot date: **2026-10-04**.

The live comparison shows the fork has **441 fork-only commits** and upstream has
**114 upstream-only commits**. The original reconciliation snapshot accounted for
53 of those upstream-only commits, leaving **61 newly arrived upstream commits**
listed here.

This is an **audit**, not a merge plan. Hardware work is held behind explicit
review and hardware validation. Upstream release-history commits remain excluded.
CYD BLE OTA remains a deliberate fork feature.

| # | Upstream | Change | Initial class | Notes |
|---:|---|---|---|---|
| 1 | `a8e42362` | T-Watch S3 LoRa receive-only listener | **HARDWARE REVIEW** | New SX1262 runtime; requires watch hardware validation. |
| 2 | `ff405910` | T-Watch LoRa TX bench + flash_known env | **LORA-CHAIN DEFERRED** | Bench TX and its flash-known variant depend on the unported T-Watch LoRa runtime; review only with that chain. |
| 3 | `e727bb1b` | T-Watch LoRa Meshtastic/MeshCore/BOTH chats | **HARDWARE REVIEW** | Feature/runtime/UI; depends on LoRa base. |
| 4 | `445aa881` | LoRa BOTH test validation | **LORA-CHAIN DEFERRED** | Runtime/CAD tuning and validation belong to the T-Watch LoRa BOTH chain, not a standalone test port. |
| 5 | `368f9c97` | T-Watch LoRa new-message pill + buzz | **HARDWARE REVIEW** | UI/haptics layered on LoRa chain. |
| 6 | `7d924805` | RockBase NM-CYD-C5 initial port | **HARDWARE REVIEW** | New ESP32-C5/RISC-V board family. |
| 7 | `bff36e36` | NM-CYD-C5 hardware bring-up/pins | **HARDWARE REVIEW** | Board-specific measured pin work. |
| 8 | `2107b0fd` | NM-CYD-C5 radios + safe PRNG seed | **HARDWARE REVIEW** | Board-specific runtime fixes. |
| 9 | `6eb2173e` | NM-CYD-C5 SPI_UPDATE latch + SD | **HARDWARE REVIEW** | Board-specific bus/storage behavior. |
| 10 | `fdb82a00` | NM-CYD-C5 80 MHz panel result | **HARDWARE REVIEW** | Measured display-clock update. |
| 11 | `b3d466d1` | CI builds NM-CYD-C5 | **SUPPORT REVIEW** | Useful only with C5 board port. |
| 12 | `052b2784` | NM-CYD-C5 SD reporting board gate | **HARDWARE REVIEW** | C5 isolation fix. |
| 13 | `ec7c7ec6` | NM-CYD-C5 shared-code no-ops | **HARDWARE REVIEW** | Cross-board safety within C5 port. |
| 14 | `8d775031` | Drop dead BT Classic include | **OPEN #87** | One-line cleanup on current master; no runtime behavior change. |
| 15 | `84a1aa09` | Remove T-Watch RADIO RESET/STEADY/CLOCK CHECK rows | **FORK-DIAGNOSTIC DEFERRED** | Upstream removed these after its deaf-radio root cause was fixed, but the fork still carries the diagnostic/self-heal chain. Do not remove until T-Watch hardware verifies those controls are obsolete here too. |
| 16 | `5ce8aae2` | TAGS/RINGS log-only + GPS rest + 400 mA charge | **PARTIAL: OPEN #96 / HARDWARE DEFERRED** | Software-only TAGS + RINGS log-only policy extracted as #96. GPS rest/power cycling and S3 Plus 400 mA charging remain hardware-deferred. |
| 17 | `e607cf75` | CYD CHARGE MODE + runtime battery log | **HARDWARE REVIEW** | New cross-board power mode. |
| 18 | `f1e07994` | Time-zone bidirectional steps + CHARGE MODE entry/wake | **PARTIAL: OPEN #93 / CHARGE DEFERRED** | Bidirectional zone row extracted onto #80; charge-mode behavior remains in the hardware/power chain. |
| 19 | `c7b2a96b` | CHARGE MODE tap reveals screen | **HARDWARE REVIEW** | Depends on charge-mode base. |
| 20 | `c26cb033` | CHARGE MODE light sleep | **SUPERSEDED UPSTREAM** | Do not port independently: `8ca8f434` removes this light-sleep path after watchdog resets on hardware. |
| 21 | `e5041a30` | Yield before CHARGE MODE light sleep | **SUPERSEDED UPSTREAM** | Follow-up to the light-sleep experiment; the entire light-sleep path is later removed by `8ca8f434`. |
| 22 | `8ca8f434` | PRIVACY MODE + remove charge light sleep | **PARTIAL: OPEN #95 / CHARGE DEFERRED** | Screen-only privacy behavior extracted as #95. Its CHARGE change is final-state evidence that the earlier light-sleep experiment must not be ported. |
| 23 | `cafb0997` | Console PINS/I2C bench commands | **HARDWARE-GUARD DEFERRED** | Upstream pin list assumes classic CYD; this fork needs per-board pin/I2C maps before exposing the commands globally. |
| 24 | `885e8db1` | CYD BOOT button screen toggle / long-press charge | **DEPENDENCY/HARDWARE DEFERRED** | Long-press behavior depends on the unported CHARGE MODE chain; GPIO0/button semantics also require validation across supported CYD variants before any extraction. |
| 25 | `ff2a54e8` | 3.5-inch screens no longer draw one half twice | **REPRESENTED BY #75** | Refreshed S035C path already clears each half-buffer and handles the Wi-Fi keyboard safely. |
| 26 | `d58e4c2d` | v1.26.0 release notes/clip | **EXCLUDE** | Upstream release history. |
| 27 | `d232161b` | Lab builds include 3.5-inch | **REPRESENTED BY #75** | #75 marks `cyd35c` as LAB-only in `web-flasher/boards.json`; the current matrix-driven LAB workflow copies its firmware/manifest and injects lab-only picker entries without upstream\’s hard-coded board list. |
| 28 | `adb836e8` | flash_known second ESP32-2432S032C | **OPEN #90** | Tooling-only CYD32C known-device mapping, stacked on #73; includes both validated units. |
| 29 | `137dd106` | YZZERD wizard outfit / XYZZY unlock | **NEW-WARDROBE DEFERRED** | Separate from #83: adds a new outfit plus TERMINAL/XYZZY unlock behavior across theme, console, mesh metadata and tests. Review as part of the later wardrobe feature chain. |
| 30 | `9c23bf13` | Legend top hat becomes aura | **AURA-CHAIN DEFERRED** | Replaces the existing Legend top-hat model with new aura state/settings/rendering; not represented by #83. |
| 31 | `15756a87` | Test flasher wears Legend look | **AURA-CHAIN DEFERRED** | Test-build preview behavior only makes sense after the Legend aura model is ported. |
| 32 | `d75a7daf` | Master unlock lights aura | **AURA-CHAIN DEFERRED** | Follow-up to the unported aura state model; no standalone value. |
| 33 | `3d2ab4f0` | v1.27.0 release notes/clip | **EXCLUDE** | Upstream release history. |
| 34 | `398a511d` | OVER 9000 outfit / scouter unlock | **NEW-WARDROBE DEFERRED** | Separate new outfit/unlock and mesh metadata chain; not part of #83 wardrobe polish. |
| 35 | `93f50d16` | Emulator solo screen + pixel costume bench | **OPEN #94** | Simulator-only costume inspection/pixel-cost bench; no firmware/runtime behavior change. |
| 36 | `279dafc7` | Skip drawing what a costume hides | **DEPENDENCY-DEFERRED** | Depends on later YZZERD / OVER 9000 outfit IDs not present on current master or #83; review with that outfit chain. |
| 37 | `e54d7e77` | OVER 9000 hair fill optimization | **NEW-WARDROBE DEFERRED** | Optimization only for the unported OVER 9000 outfit. |
| 38 | `3682867d` | YZZERD outline cleanup | **NEW-WARDROBE DEFERRED** | Rendering cleanup only for the unported YZZERD outfit. |
| 39 | `ad284e13` | Console OUTFIT n / AURA bench controls | **DEPENDENCY-DEFERRED** | Depends on the later AURA/outfit model not present in current master; review with that wardrobe chain. #94 already supplies host-side costume inspection/cost tooling without adding console/runtime state. |
| 40 | `bdae5786` | Merge master into C5 port | **MERGE-ONLY** | No standalone port needed. |
| 41 | `53e945a6` | flash_known RockBase C5 + 8-byte MAC | **HARDWARE REVIEW** | C5 support chain. |
| 42 | `969590ed` | flash_known C5 pioarduino core path | **HARDWARE REVIEW** | C5 toolchain support. |
| 43 | `8543ce69` | Report C5 framebuffer placement | **HARDWARE/DIAGNOSTIC REVIEW** | C5-specific memory diagnostics. |
| 44 | `cf4bc6b0` | Aquarium integer math optimization | **OPEN #89** | One-file rendering/performance refresh; all upstream hunks applied cleanly. |
| 45 | `b37dbc6b` | SQW_SQUACHY_LAPS profiler | **HARDWARE/OUTFIT DEFERRED** | On-device scratch-lap profiling was used for C5/aura/costume timing. Keep out of current firmware until that hardware/outfit chain is evaluated; #94 provides a host-side costume pixel-cost bench meanwhile. |
| 46 | `c703c179` | v1.28.0 release + C5 in Release | **EXCLUDE/PARTIAL** | Release history excluded; C5 publication considered only after board validation. |
| 47 | `38601d91` | C5 on web flasher as BETA | **PUBLICATION REVIEW** | Blocked on C5 hardware acceptance. |
| 48 | `ec5ac48f` | C5 DMA frame step 1 | **HARDWARE REVIEW** | C5 display transport chain. |
| 49 | `1d0a0526` | C5 async push task step 2 | **HARDWARE REVIEW** | C5 display transport chain. |
| 50 | `2e030460` | C5 four-pixel conversion loop | **HARDWARE REVIEW** | C5 display transport optimization. |
| 51 | `6d9c6f13` | C5 flip-flop diagnostic gate | **BENCH/HARDWARE REVIEW** | Diagnostic follow-up. |
| 52 | `459e0db2` | C5 pushFrame comment update | **C5-CHAIN DEFERRED** | Comment-only follow-up to C5 async/double-buffer diagnostics; no standalone value before that code exists. |
| 53 | `48f7e308` | Legend aura propagates to visitors | **AURA/PROTOCOL DEFERRED** | Depends on the aura model and consumes a mesh advert bit. Keep separate until protocol compatibility is explicitly reviewed. |
| 54 | `22d2363c` | Lab flasher restores C5 bootloader/boot_app0 | **PUBLICATION REVIEW** | C5 lab publication support. |
| 55 | `c64cd536` | C5 in ?lab=1 picker | **PUBLICATION REVIEW** | C5 lab publication support. |
| 56 | `edf8d46c` | C5 dual-buffer first frame + diagnostics touch bus | **HARDWARE REVIEW** | C5 display/touch correctness. |
| 57 | `0fa447a8` | SHAMBLER zombie outfit / FIRE owl unlock | **NEW-WARDROBE DEFERRED** | Separate new outfit/unlock/theme asset chain; not represented by #83. |
| 58 | `b8d78282` | NEARBY excludes IGNORE list | **OPEN #88** | Focused policy-consistency refresh with emulator regression switch. |
| 59 | `03e60328` | Owl stops after SHAMBLER; snoozed leave NEARBY | **PARTIAL: OPEN #92 / OUTFIT-DEFERRED** | Snoozed-device NEARBY policy extracted onto #88; SHAMBLER/owl half waits for the outfit chain. |
| 60 | `4f813d52` | SQUAD SEND button on in-range page | **OPEN #91** | Focused UI/action refresh reusing the existing compose path; emulator coverage included. |
| 61 | `4183f7ec` | v1.29.0 release notes/clip/gallery | **EXCLUDE** | Upstream release history. |

## First-pass grouping

- **T-Watch LoRa:** 1-5. Treat as one hardware feature chain.
- **NM-CYD-C5:** 6-13 and 40-56. Treat as one new-board program; no blind cherry-picks.
- **Power / charge / privacy:** 15-24. Audit final behavior as a chain because later commits revise earlier light-sleep choices.
- **3.5-inch / board publication:** 25, 27, 28.
- **Wardrobe / aura / unlocks:** 29-39, 53, 57, and part of 59.
- **Small functional candidates:** 14, 23, 25, 28, 36, 44, 58-60.
- **Release history:** 26, 33, 46, 61 — intentionally excluded as fork release history.

## Recommended next pass

Review the small functional candidates first against current master, then review
the final state of each hardware chain rather than porting intermediate commits.
For the C5 and CHARGE/PRIVACY chains especially, later commits supersede earlier
implementation choices.

No item in this audit authorizes a merge, release, tag, VERSION change, or
production publication.
