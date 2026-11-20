#!/usr/bin/env python3
"""Source-level guard for narrowly selected upstream v1.45 Phase 1 ports.

The real PIN behavior is exercised by test/security_test.cpp using persisted
Preferences across Security::begin() reboots. This guard catches accidental
regressions in the Wi-Fi promiscuous callback, which requires ESP32 hardware.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sec = (root / "src/security.cpp").read_text()
det = (root / "src/detection.cpp").read_text()
test = (root / "test/security_test.cpp").read_text()

pin = sec[sec.index("bool setPin("):sec.index("void disable()", sec.index("bool setPin("))]
assert "if (!s_enabled) setLockAtBoot(true);" in pin, "new first PIN must lock at boot"
assert pin.index("if (!s_enabled) setLockAtBoot(true);") < pin.index("s_enabled = true;")
assert "Security::begin();" in test
assert 'first PIN survives reset as locked' in test
assert 'replacement PIN preserves unlocked boot' in test

assert "type == 0 && (subtype == 12 || subtype == 10)" in det
assert "(_deauthLastFireMs == 0 ||" in det
assert "_deauthLastFireMs = now ? now : 1u;" in det
assert "Settings::typeEnabled(DetectionType::DEAUTH)" in det
assert "d.evidence = EvidenceKind::WIFI_DEAUTH;" in det
print("PASS: upstream v1.45 first-PIN boot lock + deauth/disassoc safeguards")
