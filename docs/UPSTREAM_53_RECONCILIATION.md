# Upstream 53-commit reconciliation

Source comparison: `retrodroid32/SquachWatch-CYD:master` against `skizzophrenic/SquachWatch-CYD:master` as reviewed on 2026-09-29.

This checklist tracks **functional reconciliation**, not commit-for-commit ancestry. Open PRs are intentionally kept separate for review and hardware testing. Until they are merged, GitHub will continue to report the fork as behind upstream. Even after selected PRs merge, intentionally excluded upstream commits (notably upstream release/version history and the removal of CYD BLE OTA) mean the histories will remain different.

| # | Upstream SHA | Change | Disposition | Fork PR | Notes |
|---:|---|---|---|---|---|
| 1 | `ddf56c91` | CrowPanel Advance 7.0 base support | **PORTED** | #40 | Selected functional port; no upstream merge commit copied verbatim. |
| 2 | `c31e6c85` | CrowPanel docs/per-commit build | **PORTED** | #40 | Docs/build portions represented in reconciliation. |
| 3 | `974e868f` | CrowPanel buzzer for new-device alerts | **PORTED** | #40 | CrowPanel buzzer support included. |
| 4 | `67060363` | Freenove ESP32-S3 2.8 support | **PORTED** | #40 | Freenove S3 hardware groundwork included. |
| 5 | `7c6c4b94` | Freenove S3 SDMMC/status light/random seed | **PORTED** | #40 | Functional board support represented. |
| 6 | `53d561de` | Freenove S3 battery/system page and display limit | **PORTED** | #40 | Board/runtime portions represented. |
| 7 | `4c3a3603` | Freenove S3 docs/pinout/build | **PORTED** | #40 | Documentation/build portions represented. |
| 8 | `5f3e643d` | T-Watch WATCH SETTINGS and buzz levels | **PORTED** | #40 | Watch settings work represented. |
| 9 | `567d13fe` | v1.21.0 release notes/clip | **EXCLUDED** | — | Upstream release/version history is intentionally not imported. |
| 10 | `9523f140` | v1.21.0 release-note follow-up | **EXCLUDED** | — | Upstream release/version history is intentionally not imported. |
| 11 | `8a0c88de` | Spam flood alert + BLACKBOX CLR visibility | **PORTED** | #40 | Functional behavior represented. |
| 12 | `7cc1dd0f` | Remove BLE updates from CYDs | **EXCLUDED** | — | Fork intentionally retains BLE OTA. |
| 13 | `20a109ec` | Merge commit for CYD BLE-OTA removal | **EXCLUDED** | — | Same intentional divergence: BLE OTA remains. |
| 14 | `77de79f2` | Wardrobe redraw: TINFOIL/bros/CAPTAIN | **PORTED** | #40 | Cosmetic/wardrobe work represented. |
| 15 | `0c441f02` | Wardrobe redraw: remaining outfits | **PORTED** | #40 | Cosmetic/wardrobe work represented. |
| 16 | `f7b72602` | Restore original UNICORN/SNOW PARKA/SHARK | **PORTED** | #40 | Wardrobe reconciliation represented. |
| 17 | `bc9f3d17` | TANOOKI ears adjustment | **PORTED** | #40 | Wardrobe reconciliation represented. |
| 18 | `09030fe5` | TANOOKI suit adjustment | **PORTED** | #40 | Wardrobe reconciliation represented. |
| 19 | `3256ac52` | Simulator: no-BT update screen | **PORTED** | #40 | Simulator/update-screen portion represented while BLE OTA is retained on fork hardware. |
| 20 | `8ae07fee` | Wardrobe merge commit | **REPRESENTED** | #40 | Underlying changes ported; merge commit identity not copied. |
| 21 | `9e41660f` | v1.22.0 release notes/clip | **EXCLUDED** | — | Upstream release/version history is intentionally not imported. |
| 22 | `62b5d259` | Watch alert design studies | **PORTED** | #40 | Watch-alert work represented. |
| 23 | `8e431e0c` | LOCKED ON alert variants | **PORTED** | #40 | Watch-alert work represented. |
| 24 | `acf7d80a` | LOCKED ON Operator chosen | **PORTED** | #40 | Watch-alert work represented. |
| 25 | `b3785d40` | Watch alert waits for tap/Squachy explanation | **PORTED** | #40 | Watch-alert behavior represented. |
| 26 | `72d0db7d` | WATCHTEST fallback sources | **PORTED** | #40 | Watch test/runtime portions represented. |
| 27 | `8a6cd0e5` | Watch-alert headset artwork | **PORTED** | #40 | Watch-alert cosmetic work represented. |
| 28 | `6b0dc0fd` | Watch-alert merge commit | **REPRESENTED** | #40 | Underlying changes ported; merge commit identity not copied. |
| 29 | `1d6c88d9` | v1.23.0 release notes/clip | **EXCLUDED** | — | Upstream release/version history is intentionally not imported. |
| 30 | `cfa67eb2` | Merge PR #11 CrowPanel | **REPRESENTED** | #40 | Underlying CrowPanel changes ported. |
| 31 | `82e1f103` | Merge PR #16 Freenove S3 | **REPRESENTED** | #40 | Underlying Freenove S3 changes ported. |
| 32 | `53fd0854` | 18 more time zones | **PORTED** | #40 | Time-zone additions represented. |
| 33 | `8d842038` | LOG ignored marker / IGNORE↔UN-IGNORE | **PORTED** | #40 | Ignore-state UX represented. |
| 34 | `5c188ee6` | Remote ID over Bluetooth and Wi-Fi | **PORTED** | #40 | Remote ID additions represented. |
| 35 | `1a779796` | Watch alert tap/remove logging | **PORTED** | #40 | Watch-alert logging represented. |
| 36 | `79d755aa` | Flock-You signatures | **PORTED** | #40 | Flock additions represented. |
| 37 | `e3e0ffd0` | v1.24.0 notes/clip + S3 boards on flasher | **PARTIAL** | #40 | Functional S3/flasher exposure represented; upstream release metadata excluded. |
| 38 | `4c9d2082` | Fix BLE advertised-name lifetime | **PORTED** | #41 | Fork-specific safe payload parsing covers the lifetime bug. |
| 39 | `13aa8368` | Canonical BLE address order + legacy migration | **PORTED** | #41 + #44 | #41 normalizes display/storage; #44 preserves legacy policies/Regulars/BlackBox rows. |
| 40 | `cdff6c10` | Recognize T-Watch S3 Plus in flash_known | **PORTED** | #51 | Added to the watch GPS follow-up. |
| 41 | `278e85dd` | T-Watch S3 Plus GPS power/UART probe | **PORTED** | #51 | GNSS runtime ported using shared parser from #45. |
| 42 | `fa9d7341` | Settings headings are labels, not folds | **PORTED** | #49 | Dedicated UI follow-up. |
| 43 | `ab0b8562` | GPS STATUS best/first-fix history | **PORTED** | #51 | Status/history represented. |
| 44 | `13b46b30` | GPS counter on T-Watch main screen | **PORTED** | #51 | Badge included and layered under speech bubbles. |
| 45 | `851d2703` | GNSS/Wi-Fi security/WiGLE groundwork | **PORTED** | #45 | Pure-code groundwork + host tests. |
| 46 | `69e72f24` | T-Watch wardrive capture/storage/USB WiGLE | **PORTED** | #52 | Runtime capture/storage/export follow-up. |
| 47 | `ca03be14` | WARDRIVE Watch Settings row + browser WiGLE download | **PORTED** | #52 | Settings row and browser download included. |
| 48 | `8ec59b50` | ESP32-2432S032C capacitive build | **PORTED** | #46 | Dedicated hardware profile; stacked on #40 GT911 groundwork. |
| 49 | `af12c340` | ADC console hardware-only guard | **PORTED** | #50 | ADC command includes emulator-safe guard. |
| 50 | `7fade178` | Grouped flasher/code finder/CYD32C publication | **PORTED** | #47 | Stacked on #46. |
| 51 | `005e4711` | Seven status-light brightness levels | **PORTED** | #48 | Includes saved-setting migration. |
| 52 | `4cdfa1c4` | v1.25.0 release notes/clip | **EXCLUDED** | — | Upstream release/version history is intentionally not imported. |
| 53 | `ab176b38` | GPS badge below speech bubbles | **PORTED** | #51 | Badge is drawn by uiClear beneath moving speech bubbles. |

## Intentional divergences

- **CYD BLE OTA remains supported.** Upstream commits that removed it are intentionally excluded.
- **Upstream release tags, VERSION progression, release-note files and clips are not imported as fork release history.** Functional code associated with a release is reconciled separately where useful.
- **Merge-commit identity is not preserved.** When the underlying feature is ported into a fork PR, the checklist marks the upstream merge commit as represented rather than duplicated.

## Open dependency chains

- #41 → #44
- #45 → #51 → #52
- #40 → #46 → #47
- #39, #42, #43, #48, #49 and #50 remain separate review/test tracks.

No item in this document grants merge approval. All merges remain explicit.
