# Upstream 53-Commit Reconciliation — 2026-09-29

This checklist reconciles the 53 commits currently present on `skizzophrenic/SquachWatch-CYD:master` but not on this fork's `master` at merge base `ecaff618e67b9c67121174937d0145c7567e1585`.

Fork production remains **v1.20.1**. This document does not authorize a merge, release, tag, or VERSION change. The fork intentionally retains CYD BLE OTA support.

## Classification

1. **Represented** — functionally present in an open reconciliation/follow-up PR.
2. **Fork-specific equivalent** — same useful behavior is represented with a fork-specific implementation or split across PRs.
3. **Useful, still needs a PR** — no representation yet.
4. **Intentionally excluded** — conflicts with a deliberate fork decision or is not applicable.
5. **Release-history / merge-only / superseded** — no code port is needed.

| # | Upstream | Upstream work | Class | Fork disposition |
|---:|---|---|:---:|---|
| 1 | `ddf56c91` | CrowPanel 7 hardware | 1 | PR #40 |
| 2 | `c31e6c85` | CrowPanel docs/build | 1 | PR #40 |
| 3 | `974e868f` | CrowPanel buzzer | 1 | PR #40 |
| 4 | `67060363` | Freenove ESP32-S3 support | 1 | PR #40 |
| 5 | `7c6c4b94` | Freenove S3 SDMMC/status light/random seed | 1 | PR #40 |
| 6 | `53d561de` | Freenove S3 battery/display ceiling | 1 | PR #40 |
| 7 | `4c3a3603` | Freenove S3 docs | 1 | PR #40 |
| 8 | `5f3e643d` | T-Watch settings + BUZZ levels | 1 | PR #40 |
| 9 | `567d13fe` | v1.21.0 release notes | 5 | Upstream release history only |
| 10 | `9523f140` | v1.21.0 notes adjustment | 5 | Upstream release history only |
| 11 | `8a0c88de` | Spam flood + BLACKBOX CLR | 1 | PR #40 |
| 12 | `7cc1dd0f` | Remove BLE OTA from CYDs | 4 | Intentionally excluded; this fork retains BLE OTA |
| 13 | `20a109ec` | Merge form of CYD BLE OTA removal | 4 | Intentionally excluded; this fork retains BLE OTA |
| 14 | `77de79f2` | Wardrobe redraw | 1 | PR #40 |
| 15 | `0c441f02` | Wardrobe redraw follow-up | 1 | PR #40 |
| 16 | `f7b72602` | Restore selected outfits | 1 | PR #40 |
| 17 | `bc9f3d17` | Tanooki adjustment | 1 | PR #40 |
| 18 | `09030fe5` | Tanooki reference redraw | 1 | PR #40 |
| 19 | `3256ac52` | Simulator no-BT update screen | 4 | Intentionally excluded; it exists to model upstream's CYD BLE-OTA removal, which this fork does not adopt |
| 20 | `8ae07fee` | Wardrobe merge commit | 5 | Merge-only; underlying work represented by PR #40 |
| 21 | `9e41660f` | v1.22.0 release notes | 5 | Upstream release history only |
| 22 | `62b5d259` | Watch alert studies | 1 | PR #40 |
| 23 | `8e431e0c` | Watch alert styles | 1 | PR #40 |
| 24 | `acf7d80a` | Operator alert selected | 1 | PR #40 |
| 25 | `b3785d40` | Squachy alert behavior | 1 | PR #40 |
| 26 | `72d0db7d` | WATCHTEST fallback | 1 | PR #40 |
| 27 | `8a6cd0e5` | Headset graphics follow Squachy's head | 1 | PR #40 |
| 28 | `6b0dc0fd` | Watch-alert merge commit | 5 | Merge-only; underlying work represented by PR #40 |
| 29 | `1d6c88d9` | v1.23.0 release notes | 5 | Upstream release history only |
| 30 | `cfa67eb2` | Merge PR #11 CrowPanel | 5 | Merge-only; underlying work represented by PR #40 |
| 31 | `82e1f103` | Merge PR #16 Freenove S3 | 5 | Merge-only; underlying work represented by PR #40 |
| 32 | `53fd0854` | Time zones | 1 | PR #40 |
| 33 | `8d842038` | Visible ignore/unignore | 1 | PR #40 |
| 34 | `5c188ee6` | Remote ID additions | 1 | PR #40 |
| 35 | `1a779796` | Watch-alert tap logging | 1 | PR #40 |
| 36 | `79d755aa` | Flock signatures | 1 | PR #40 |
| 37 | `e3e0ffd0` | v1.24.0 release notes | 5 | Upstream release history only |
| 38 | `4c9d2082` | BLE advertised-name lifetime | 1 | PR #41 |
| 39 | `13aa8368` | Printed-order BLE MAC + legacy migration | 2 | Split deliberately across PR #41 (canonical order) and PR #44 (legacy persisted-data migration) |
| 40 | `cdff6c10` | T-Watch S3 Plus flash_known mapping | 1 | PR #56 |
| 41 | `278e85dd` | T-Watch S3 Plus GPS ON/OFF + hardware | 2 | PR #56, using PR #45's shared host-tested GNSS parser instead of a second parser |
| 42 | `fa9d7341` | Settings headings are labels, not folds | 1 | PR #54 |
| 43 | `ab0b8562` | T-Watch GPS STATUS/history | 2 | PR #56, integrated with the shared GNSS state |
| 44 | `13b46b30` | T-Watch main-screen GPS counter | 2 | PR #56, integrated with the shared GNSS state |
| 45 | `851d2703` | GNSS/WiGLE groundwork | 1 | PR #45 |
| 46 | `69e72f24` | Full T-Watch wardrive runtime | 1 | PR #57, stacked on #56/#45 |
| 47 | `ca03be14` | WATCH SETTINGS wardrive row + Web Serial WiGLE download | 1 | PR #57 |
| 48 | `8ec59b50` | ESP32-2432S032C support | 1 | PR #46 |
| 49 | `af12c340` | ADC console command hardware guard | 2 | PR #55 provides the useful ADC command as a standalone GPS-safe equivalent and skips GPIO35 when it is GNSS RX |
| 50 | `7fade178` | Flasher grouping/code finder/CYD32C publication | 1 | PR #47 |
| 51 | `005e4711` | Seven-level status-light brightness | 1 | PR #48 |
| 52 | `4cdfa1c4` | v1.25.0 release notes | 5 | Upstream release history only |
| 53 | `ab176b38` | T-Watch GPS bar layering | 1 | PR #56; badge is drawn below Squachy/speech bubbles |

## Result

- **Category 1 — represented:** 35 commits
- **Category 2 — fork-specific equivalent:** 5 commits
- **Category 3 — useful but still missing:** 0 commits
- **Category 4 — intentionally excluded:** 3 commits
- **Category 5 — release-history / merge-only / superseded:** 10 commits
- **Total:** 53 commits

## Open PR dependency map

```text
#41 BLE name/MAC normalization
└─ #44 legacy BLE address migration

#40 selected upstream reconciliation
└─ #46 CYD32C capacitive 3.2-inch
   └─ #47 flasher board finder / CYD32C publication

#45 GNSS/WiGLE groundwork
└─ #56 T-Watch S3 Plus GNSS runtime
   └─ #57 T-Watch wardrive runtime + WiGLE export

Independent:
#39 optional CYD GPS variants
#42 shared UI safety/fitting
#43 host dependency + mesh crypto verification
#48 seven-level status-light brightness
#54 settings headings are labels
#55 GPS-safe ADC console diagnostics
```

No PR in this checklist is authorized for merge merely by appearing here.
