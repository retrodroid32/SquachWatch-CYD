# Upstream 45-commit audit — 2026-10-06

Scope: the 45 upstream commits after `4f813d52` in the current 160-commit behind set.
This is a selective reconciliation ledger, not a merge plan.

Rules:
- preserve fork BLE OTA;
- do not port upstream release-history commits;
- no production version/tag/release changes;
- hardware/publication changes stay deferred until their boards are accepted;
- prefer focused ports over blind upstream merges.

| # | Commit | Summary | Disposition |
|---:|---|---|---|
| 1 | `4183f7ec` | v1.29.0 release notes/gallery | **EXCLUDE — release history** |
| 2 | `35a1e050` | Wardrive queues allocated only by begin() | **CANDIDATE ON #69 STACK** — saves ~3 KB on boards that never wardrive; current master does not yet contain wardrive.cpp |
| 3 | `4c274960` | TH3 0N3 outfit / DIGITAL-rain unlock | **NEW-WARDROBE DEFERRED** |
| 4 | `6f2e35a4` | v1.30.0 release notes/gallery | **EXCLUDE — release history** |
| 5 | `87f60781` | Merge C5 5 GHz work | **C5 HARDWARE DEFERRED** |
| 6 | `81f3be67` | 3.5in capacitive cyd35c-fast | **HARDWARE / #75 FOLLOW-UP REVIEW** |
| 7 | `50de8e60` | Publish 3.5in capacitive as BETA | **PUBLICATION DEFERRED** |
| 8 | `5fdedf72` | Squad serious-catch heads-up banner | **FEATURE REVIEW** |
| 9 | `237faf08` | M5Stack StickS3 bring-up | **NEW-BOARD HARDWARE DEFERRED** |
| 10 | `09a0813a` | Cardputer ADV bring-up | **NEW-BOARD HARDWARE DEFERRED** |
| 11 | `64878a6e` | Cardputer EXT SCREEN bench driver | **CARDPUTER CHAIN DEFERRED** |
| 12 | `dd7ca368` | Cardputer EXT SCREEN setting | **CARDPUTER CHAIN DEFERRED** |
| 13 | `3b977384` | EXT SCREEN still backdrop | **CARDPUTER CHAIN DEFERRED** |
| 14 | `802a8482` | Small-screen LOG/WATCH/HUNT/MORE INFO fitting | **SMALL-SCREEN BOARD CHAIN DEFERRED** |
| 15 | `16bc4b39` | Small-screen update/watch/hunt/diagnostics fitting | **SMALL-SCREEN BOARD CHAIN DEFERRED** |
| 16 | `4fb8985a` | StickS3 speaker amp off at boot | **STICKS3 HARDWARE DEFERRED** |
| 17 | `b4e9db60` | StickS3 backlight PWM | **STICKS3 HARDWARE DEFERRED** |
| 18 | `316027a2` | Small-screen stats/dex/desk/outfit fitting | **SMALL-SCREEN BOARD CHAIN DEFERRED** |
| 19 | `7060c007` | Cardputer lab publication | **PUBLICATION DEFERRED** |
| 20 | `34e5e52d` | next.html 3-step flasher redesign | **WEB-FLASHER UX REVIEW** |
| 21 | `2f9a6a35` | next.html tile naming | **WEB-FLASHER UX CHAIN** |
| 22 | `74a09966` | next.html board-code help placement | **WEB-FLASHER UX CHAIN** |
| 23 | `26c2e837` | next.html white-screen note placement | **WEB-FLASHER UX CHAIN** |
| 24 | `f49ecbef` | StickS3/Cardputer public BETA publication | **PUBLICATION DEFERRED** |
| 25 | `4b9c29d6` | C1iPPY pet | **PET/FEATURE DEFERRED** |
| 26 | `7a8a7f9e` | Throw Squachy/C1iPPY + XP-style NEARBY tiles | **UI/PET FEATURE REVIEW** |
| 27 | `0ce517c8` | v1.31.0 notes + pet/demo changes | **EXCLUDE release history; feature pieces handled separately** |
| 28 | `f5883353` | DETECTIONS XP/CLASSIC setting | **UI FEATURE REVIEW** |
| 29 | `17bbf151` | XP counter tiles at top | **UI FEATURE CHAIN** |
| 30 | `08e609f7` | v1.31 clip + pet/counter layering | **PARTIAL FEATURE FOLLOW-UP; release media excluded** |
| 31 | `dd58eaf3` | HUNTING pill opens HUNT MODE | **SUPERSEDED BY FORK** — current master already has separate WATCH and HUNT pills opening their own rosters from #63-era work |
| 32 | `90b78102` | flash_known second 3.5in resistive MAC | **LOCAL-HARDWARE ONLY** — port only if that exact unit belongs in fork helper |
| 33 | `b2b8f434` | Remove black line under pointed speech bubble | **SAFE VISUAL CANDIDATE** — current master still erases the bubble rectangle every frame |
| 34 | `443f5f3c` | ESP32-32E board-code aliases + browser emulator version | **WEB-FLASHER SUPPORT CANDIDATE** — separate alias/version pieces before porting |
| 35 | `395672ac` | Default usual 2.8in ST7789 flasher to 80 MHz | **POLICY/HARDWARE REVIEW** — changes default flashing behavior |
| 36 | `10a7a329` | T0ASTY toaster pet | **PET/FEATURE DEFERRED** |
| 37 | `6b7a7637` | More pet thrown lines + master unlock | **PET CHAIN DEFERRED** |
| 38 | `ba3f5bc6` | Pet landing reactions + throw streaks | **PET CHAIN DEFERRED** |
| 39 | `c64d3cb4` | XP counter readability tweak | **XP UI CHAIN** |
| 40 | `9a7f2686` | XP counter typography tweak | **XP UI CHAIN** |
| 41 | `e78f148b` | XP counters fit 135px boards | **XP/SMALL-SCREEN CHAIN** |
| 42 | `b9b71190` | XP counters use third row on 135px boards | **XP/SMALL-SCREEN CHAIN** |
| 43 | `5fb1dd3d` | Rename toaster to T0@$TY | **PET CHAIN DEFERRED** |
| 44 | `e025243b` | Toaster unlock hint | **PET CHAIN DEFERRED** |
| 45 | `6ef18479` | v1.32.0 release notes/clip | **EXCLUDE — release history** |

## Immediate focused candidates

1. `b2b8f434` — pointed speech-bubble erase fix.
2. `35a1e050` — wardrive RAM allocation optimization, but apply only to the open wardrive stack.
3. `443f5f3c` — split-review the web-flasher ESP32-32E aliases and browser-emulator version stamp.
4. `5fdedf72` — optional squad heads-up feature, if desired.

Everything else is primarily a board-specific chain, publication change, large UI/pet/wardrobe feature chain, or upstream release history.
