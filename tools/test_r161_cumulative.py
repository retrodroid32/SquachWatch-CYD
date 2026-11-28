#!/usr/bin/env python3
"""Cumulative R16.1 feature guards: prevent accidental source rollback."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
markers={
    "src/ui_rawscan.cpp": [
        "\"SCAN.LOG\"",
        "\"RESULTS.LOG\"",
        "\"BLE LIVE\"",
        "\"WIFI LISTENING\"",
        "drawFeed(",
        "drawList(",
        "drawCompactSignal(",
        "scramble(label",
        "NAME_AT",
        "feed::holding",
        "wifiCascadeMs(",
        "eng.rawWifiChannel",
        "Privacy::mac",
        "Privacy::name",
        "uiRawScanHitConfirm",
        "uiRawScanRowAt",
        "drawTargetFlags"
    ],
    "src/main.cpp": [
        "InfoCard::feed",
        "InfoCard::tapCloses",
        "enterAlert(copy,false,true)",
        "enterRawScanCached(s_alertScanBle)",
        "s_alertFromScan && state == AppState::ALERT",
        "RawScanConfirmTap::WATCH",
        "RawScanConfirmTap::HUNT",
        "RawScanConfirmTap::IGNORE",
        "InfoPage::EVIDENCE",
        "InfoPage::EXPLAIN",
        "engine.stopRawScan()"
    ],
    "src/info_card.cpp": [
        "\"SURE\"",
        "\"LIKELY\"",
        "\"MAYBE\"",
        "\"WHY: %s\"",
        "\"TIP\"",
        "\"SEEN\"",
        "\"PLACES\"",
        "\"FIRST\"",
        "TrailHistory::cache()",
        "tapCloses",
        "hidden"
    ],
    "src/ui_alert.cpp": [
        "OWNER_WITH",
        "OWNER_AWAY",
        "OWNER ?",
        "+%u WAITING",
        "Settings::privacyMode()",
        "context+used",
        "uiAlertSetSpam"
    ],
    "include/info_card.h": [
        "FREENOVE35_N",
        "SQW_INFO_CARD"
    ],
    "src/theme.cpp": [
        "InfoCard::active(now)",
        "InfoCard::draw"
    ]
}
for path, required in markers.items():
    data=(root/path).read_text(encoding="utf-8")
    for marker in required:
        assert marker in data, f"{path}: missing cumulative feature {marker}"
assert "chNow" not in (root/"src/ui_rawscan.cpp").read_text(), "No fabricated channel progress"
print("PASS: cumulative scanner, tracking, information and alert-card source guards")
