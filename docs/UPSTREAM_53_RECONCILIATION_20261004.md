# Upstream 53-commit reconciliation — refreshed 2026-10-04

This replaces the stale September tracker in PR #53. It tracks **functional
reconciliation**, not commit ancestry. The fork intentionally retains CYD BLE
OTA and does not import upstream release/version history.

Statuses below describe where each of the original 53 upstream-only commits is
represented now. Open replacement PRs still require their own review/testing
and **none of this grants merge approval**.

| # | Upstream | Change | Current disposition |
|---:|---|---|---|
| 1 | `ddf56c91` | CrowPanel Advance 7.0 base | **OPEN #79** — clean compile-only RGB/display/touch base |
| 2 | `c31e6c85` | CrowPanel docs/per-commit build | **PARTIAL / DEFERRED** — build profile is in #79; old standalone docs/bench packaging are deferred until hardware validation |
| 3 | `974e868f` | CrowPanel buzzer | **INTENTIONALLY DEFERRED** — the old #40 path never completed a real alert chirp trigger; not blindly ported |
| 4 | `67060363` | Freenove ESP32-S3 2.8 support | **OPEN #78** — clean compile-only board profile |
| 5 | `7c6c4b94` | Freenove S3 SDMMC/status light/random seed | **OPEN #78 / FORK EQUIVALENT** — WS2812 + safe RNG included; SD logger intentionally remains off until an SDMMC path is validated |
| 6 | `53d561de` | Freenove S3 battery/system page/display limit | **OPEN #78** — GPIO9 voltage estimate + 40 MHz display ceiling |
| 7 | `4c3a3603` | Freenove S3 docs/pinout/build | **PARTIAL / DEFERRED** — build/pin behavior is encoded in #78; standalone docs await hardware validation |
| 8 | `5f3e643d` | T-Watch WATCH SETTINGS + buzz levels | **REPRESENTED ON CURRENT MASTER** by the newer watch implementation |
| 9 | `567d13fe` | v1.21.0 release notes | **EXCLUDED** — upstream release history |
| 10 | `9523f140` | v1.21.0 notes follow-up | **EXCLUDED** — upstream release history |
| 11 | `8a0c88de` | Spam flood + BLACKBOX CLR visibility | **REPRESENTED ON CURRENT MASTER** |
| 12 | `7cc1dd0f` | Remove BLE updates from CYDs | **EXCLUDED** — fork intentionally retains BLE OTA |
| 13 | `20a109ec` | Merge form of BLE-OTA removal | **EXCLUDED** — same intentional divergence |
| 14 | `77de79f2` | Wardrobe redraw: TINFOIL/bros/CAPTAIN | **OPEN #83** |
| 15 | `0c441f02` | Wardrobe redraw follow-up | **OPEN #83** |
| 16 | `f7b72602` | Restore selected outfits | **OPEN #83** |
| 17 | `bc9f3d17` | TANOOKI adjustment | **OPEN #83** |
| 18 | `09030fe5` | TANOOKI suit adjustment | **OPEN #83** |
| 19 | `3256ac52` | Simulator no-BT update screen | **EXCLUDED** — modeled upstream BLE-OTA removal, which this fork does not adopt |
| 20 | `8ae07fee` | Wardrobe merge commit | **REPRESENTED** by #83 underlying work |
| 21 | `9e41660f` | v1.22.0 release notes | **EXCLUDED** — upstream release history |
| 22 | `62b5d259` | Watch alert design studies | **SUPERSEDED / FORK EQUIVALENT** — current multi-target WATCH UI is newer |
| 23 | `8e431e0c` | LOCKED ON alert variants | **SUPERSEDED / FORK EQUIVALENT** — current WATCH alert UI is newer |
| 24 | `acf7d80a` | Operator alert selected | **SUPERSEDED / FORK EQUIVALENT** — current WATCH alert UI is newer |
| 25 | `b3785d40` | Watch alert waits for acknowledgement | **OPEN #82** — adapted to current multi-target semantics |
| 26 | `72d0db7d` | WATCHTEST fallback/debug source | **INTENTIONALLY EXCLUDED** — obsolete single-target debug machinery |
| 27 | `8a6cd0e5` | Watch-alert headset artwork | **SUPERSEDED / FORK EQUIVALENT** in current WATCH presentation |
| 28 | `6b0dc0fd` | Watch-alert merge commit | **REPRESENTED** by current master + #82 |
| 29 | `1d6c88d9` | v1.23.0 release notes | **EXCLUDED** — upstream release history |
| 30 | `cfa67eb2` | Merge CrowPanel PR | **REPRESENTED** by #79 + #84 |
| 31 | `82e1f103` | Merge Freenove S3 PR | **REPRESENTED** by #78 |
| 32 | `53fd0854` | 18 more time zones | **OPEN #80** |
| 33 | `8d842038` | Visible ignore/un-ignore state | **REPRESENTED ON CURRENT MASTER** |
| 34 | `5c188ee6` | Remote ID additions | **REPRESENTED ON CURRENT MASTER** |
| 35 | `1a779796` | Watch-alert tap/remove behavior | **FORK EQUIVALENT #82** — preserves current multi-target remove/ack rules |
| 36 | `79d755aa` | Flock-You signatures | **CURRENT MASTER + OPEN #81** — core signatures already present; exact bare Flock SSID/helper hardening in #81 |
| 37 | `e3e0ffd0` | v1.24.0 notes + S3 flasher exposure | **PARTIAL** — release history excluded; supported-board exposure handled per board, with unvalidated S3 boards kept compile/LAB-only |
| 38 | `4c9d2082` | BLE advertised-name lifetime | **MERGED #41** |
| 39 | `13aa8368` | Canonical BLE address + legacy migration | **MERGED #41 + #44** |
| 40 | `cdff6c10` | T-Watch S3 Plus mapping | **OPEN #71** |
| 41 | `278e85dd` | T-Watch S3 Plus GPS power/UART | **OPEN #71**, using shared GNSS groundwork |
| 42 | `fa9d7341` | Settings headings are labels | **MERGED #64** |
| 43 | `ab0b8562` | T-Watch GPS status/history | **OPEN #71** |
| 44 | `13b46b30` | T-Watch main-screen GPS counter | **OPEN #71** |
| 45 | `851d2703` | GNSS/Wi-Fi security/WiGLE groundwork | **OPEN #69** — refreshed replacement for old #45 |
| 46 | `69e72f24` | T-Watch wardrive runtime | **OPEN #72**, stacked on #71/#69 |
| 47 | `ca03be14` | Wardrive settings + WiGLE export | **OPEN #72** |
| 48 | `8ec59b50` | ESP32-2432S032C capacitive support | **OPEN #73** — refreshed self-contained replacement |
| 49 | `af12c340` | ADC console hardware guard | **MERGED #65** — GPS-safe GPIO35 behavior |
| 50 | `7fade178` | Flasher grouping/code finder/CYD32C publication | **OPEN #74**, stacked on #73 |
| 51 | `005e4711` | Seven status-light brightness levels | **MERGED #67** |
| 52 | `4cdfa1c4` | v1.25.0 release notes | **EXCLUDED** — upstream release history |
| 53 | `ab176b38` | T-Watch GPS badge layering | **OPEN #71** |

## CrowPanel split

The old #40 CrowPanel work is intentionally split instead of revived wholesale:

- **#79** — display, RGB blit, GT911 touch, backlight, partitions and safe compile-only board base.
- **#84** — CrowPanel-only flash-write pacing and OTA power/load tuning.
- Buzzer/probe/bench experiments remain out until they have a complete user-facing behavior and hardware validation.

## Current replacement chains

```text
#69 GNSS/WiGLE groundwork
└─ #71 T-Watch S3 Plus GNSS
   └─ #72 T-Watch wardrive runtime/export

#73 CYD32C capacitive base
├─ #74 board finder / CYD32C publication
├─ #75 S035C capacitive
├─ #76 LCDWiki ES3C28P
└─ #79 CrowPanel 7 base
   └─ #84 CrowPanel stability tuning

Independent refreshed tracks:
#77 optional CYD GPS
#78 Freenove S3
#80 expanded time zones
#81 Flock signature hardening
#82 WATCH alert acknowledgement
#83 wardrobe artwork polish
```

## Intentional divergences

- CYD **BLE OTA stays supported**.
- Upstream release tags, VERSION progression, release-note files and clips are not imported as fork release history.
- Merge-commit identity is not reproduced when the underlying functionality is represented elsewhere.
- Debug/bench-only code and incomplete hardware features are not treated as production requirements merely because they existed upstream.

This document is a reconciliation ledger only. It does not authorize any merge,
tag, VERSION bump, release, or production publication.
