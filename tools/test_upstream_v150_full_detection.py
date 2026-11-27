#!/usr/bin/env python3
"""v1.50 detection backport guard. Source checks; hardware verification required."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
sig = (root / "src/signatures.cpp").read_text()
hdr = (root / "include/signatures.h").read_text()
det = (root / "src/detection.cpp").read_text()
state = (root / "include/state.h").read_text()
settings = (root / "src/settings.cpp").read_text()
info = (root / "src/device_info.cpp").read_text()
sim = (root / "sim/sim_detections.h").read_text()
main = (root / "src/main.cpp").read_text()
expected = (
    "BlackVue", "70mai", "Thinkware", "Lytx", "Samsara", "Viofo",
    "Nextbase", "Rove", "Redtiger", "HDWifiCam", "LookCam", "TinyCam",
    "YCC365", "Sricam", "Yoosee", "Zosi", "Rokid", "Vuzix",
    "Cradlepoint", "AirLink", "Sonos", "Google speaker",
    "Wyze", "SimpliSafe", "Skim-HC05",
)
for name in expected:
    assert name.lower() in sig.lower(), "Missing signature: " + name
for phrase in ("isP2pCamSsid(", "isV380Ssid(", "ssidPatternName(",
               "_VANTRUE_", 'return "P2P-Cam"', 'return "V380"'):
    assert phrase in sig, "Missing hotspot matcher " + phrase
for phrase in ("isHomePodSiri(", "airTagFrame(", "airTagStatusByte(",
               "isFmdnPayload(", "fmdnFrameType("):
    assert phrase in sig and phrase in hdr, "Missing frame decoder " + phrase
assert "DetectionType::LISTENER" in det
assert "LISTENER = 18" in state and "COUNT   = 19" in state
assert "(1u << (uint8_t)DetectionType::LISTENER)" in settings
assert "isHomePodSiri(b, mfg.len)" in det
assert "isFmdnPayload(payload.data()" in det
assert "isAxonBwcPayload(payload.data()" in det
assert "bool isAxonBwcPayload(" in sig and "bool isFlockFsName(" in sig
assert "BodyWorn" in sig and "BodyWorn" in info and "BodyWorn" in sim
assert "DetectionType::LISTENER" in (root / "src/ui_clear.cpp").read_text()
assert "DetectionType::LISTENER, Rarity::COMMON" in (root / "src/dex.cpp").read_text()
assert "0x16" in det and "uuid == 0xFEAA || uuid == 0xFFFA" in det
assert "OWNER_WITH" in det and "OWNER_AWAY" in det
assert "det.owner" in det
assert "static bool isAirTagPairing(" in sig
assert "isAirTagPairing(payload + i" in sig
assert "payload[i + 3] == 0x02" in sig
assert '"P2P-Cam|HDWifiCam|LookCam|TinyCam|V380|YCC365|Sricam|Yoosee|Zosi"' in info
for family in ("Rokid", "Vuzix", "BlackVue", "Lytx", "Sonos", "HomePod"):
    assert family in info and family in sim, "Missing device page or simulator " + family
# v1.50 broad hardware signatures must not re-add known false positives.
oui_table = sig.split("const OuiEntry kOuiTable[] = {", 1)[1].split("\n};", 1)[0]
assert '{{0x00, 0x03, 0x7F}' not in oui_table, "Generic QCA falsely classified as Flock"
uuid_table = sig.split("const UuidEntry kUuidTable[] = {", 1)[1].split("\n};", 1)[0]
assert '{0xFEAA,' not in uuid_table, "Generic Eddystone UUID falsely classified"
assert "R12: CYD32 SD-in cold boot recovery" in main
assert "CYD32 DIRECT PANEL" in main
print("PASS: v1.50 cameras, routers, smart speakers, enhanced tag payloads and R12 recovery")

# UUID-list and short service-data frames must not classify as Remote ID.
assert '{0xFFFA,' not in uuid_table
assert det.count("if (uuid == 0xFEAA || uuid == 0xFFFA) continue;") == 2
assert "RemoteId::isBluetoothLegacy(payload.data()" in det
assert '{{0xEC, 0x5B, 0xCD}, 0xE, "Autel"' in sig
print("PASS: bare Remote ID UUID excluded; valid transport and Autel MA-M preserved")
