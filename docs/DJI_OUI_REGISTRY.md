# DJI MAC/OUI registry — manufacturer evidence, not Remote ID

Validated against the **SZ DJI TECHNOLOGY CO.,LTD** public IEEE MA-L
registry snapshot (cross-checked 2026-11-10). These 10 prefixes were
**already present** in `src/signatures.cpp` prior to this documentation
and test audit; do not duplicate them.

Sources:
- https://maclookup.app/vendors/sz-dji-technology-co-ltd
- https://mac.jasontally.com/vendor/sz-dji-technology-co-ltd
- https://udger.com/resources/mac-address-vendor-detail?name=sz_dji_technology_co-ltd

| MA-L prefix | MACLookup registration date |
| --- | --- |
| `04:A8:5A` | 2025-01-09 |
| `0C:9A:E6` | 2025-08-14 |
| `34:D2:62` | 2019-08-13 |
| `48:1C:B9` | 2022-05-07 |
| `4C:43:F6` | 2025-12-01 |
| `58:B8:58` | 2024-07-26 |
| `60:60:1F` | 2013-03-11 |
| `88:29:85` | 2025-10-29 |
| `8C:58:23` | 2025-05-27 |
| `E4:7A:2C` | 2023-10-19 |

These are **10 MA-L /24 prefixes**, each with 2^24 possible suffixes.
The dates above are the dates reported by MACLookup; other mirrors may
give slightly different first-observed dates. Allocation capacity is not
a count of manufactured or active DJI devices.

## Receiver interpretation

SquachWatch's existing OUI lookup returns vendor **DJI** and a
`DetectionType::DRONE` family classification with `HIGH_CONF` for all
10 exact prefixes. The HIGH grade concerns **confidence in the registered
manufacturer OUI**, **not** proof of an aircraft being present.
DJI also sells controllers, cameras, handheld gimbals and other networked
products; MAC addresses can be spoofed, and local/randomized MACs may not
carry their manufacturer's OUI.

A manufacturer prefix does **not** prove a particular DJI aircraft model,
current flight, Remote ID, OcuSync/DroneID protocol, or a compliance status.
To report confirmed broadcast Remote ID, require an actually decoded valid
ASTM/OpenDroneID message and keep that evidence path separate from the
manufacturer table.

## Regression contract

`test/signatures_test.cpp` enumerates all 10 prefixes (and checks both
ends of their MA-L range): exact 24-bit match, label `DJI`, family
`DRONE`, and current OUI confidence. It also rejects an unrelated,
locally-administered lookalike. See SquachEmit's **offline-only**
`tests/dji_oui_registry.json` and generated Wi-Fi fixtures for independent
cross-project decoder checks. Neither project should transmit real vendor
MAC prefixes solely to test OUI classification.
