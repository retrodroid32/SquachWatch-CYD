#!/usr/bin/env python3
"""Source-integration checks: preserve each retained row's first sighting.

Live BLE/Wi-Fi/deauth engine code is hardware-specific and cannot be linked
against host stubs. Host behavioral tests cover the visit-clock helper; this
checks every actual reactivation call site and spam window remains per visit.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/detection.cpp").read_text(encoding="utf-8")
engine = (root / "include/detection.h").read_text(encoding="utf-8")
helper = (root / "include/detection_visit.h").read_text(encoding="utf-8")
workflow = (root / ".github/workflows/build.yml").read_text(encoding="utf-8")
makefile = (root / "test/Makefile").read_text(encoding="utf-8")

deauth = source.split("const int16_t foundSlot = findLogSlot(e.mac, DetectionType::DEAUTH);", 1)[1].split("pushLog(d);", 1)[0]
ble = source.split("void DetectionEngine::applyBle(Detection d) {", 1)[1].split("// Nothing calls this today", 1)[0]
wifi = source.split("void DetectionEngine::processWiFiQ(", 1)[1].split("void DetectionEngine::pushLog(", 1)[0]

assert "uint32_t   _visitStartMs[LOG_CAP] = {0};" in engine
assert "_visitStartMs[slot] = d.firstSeen;" in source
assert '"detection_visit.h"' in source
assert "DetectionVisit::restart(_visitStartMs[(uint8_t)foundSlot], now);" in deauth
assert "row.firstSeen = now;" not in deauth
assert "DetectionVisit::restart(_visitStartMs[slot], nowMs);" in ble
assert "_log[slot].firstSeen = nowMs;" not in ble
assert "DetectionVisit::restart(_visitStartMs[slot], nowMs);" in wifi
assert "_log[slot].firstSeen = nowMs;" not in wifi
assert source.count("DetectionVisit::restart(") == 3
assert "DetectionVisit::elapsedMs(_visitStartMs[slot], gone.lastSeen)" in source
assert "gone.lastSeen - gone.firstSeen" not in source
assert "inline uint32_t elapsedMs(" in helper
assert "detection_visit_test" in makefile
assert "python3 tools/test_first_seen_stability.py" in workflow
print("PASS: original firstSeen retained, per-return visits reset, spam duration isolated")
