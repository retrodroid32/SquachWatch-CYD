#!/usr/bin/env python3
"""Narrow upstream v1.50 signature backport regression."""
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / "src/signatures.cpp").read_text()
required = {
    '0xF0, 0xC8, 0x8B': ('"Wyze"', 'DetectionType::CAMERA', 'Confidence::HIGH_CONF'),
    '0x80, 0x48, 0x2C': ('"Wyze"', 'DetectionType::CAMERA', 'Confidence::HIGH_CONF'),
    '0xF8, 0x51, 0x28': ('"SimpliSafe"', 'DetectionType::CAMERA', 'Confidence::MED_CONF'),
    '0x98, 0xD3, 0x31': ('"Skim-HC05"', 'DetectionType::SKIMMER', 'Confidence::LOW_CONF'),
}
for prefix, fragments in required.items():
    matching = [line for line in s.splitlines() if '{{' + prefix + '}' in line]
    assert len(matching) == 1, (prefix, matching)
    assert all(fragment in matching[0] for fragment in fragments), matching[0]
assert '{{0x98, 0xD3, 0x00}' not in s, 'incorrect legacy HC-05 OUI returned'
print("PASS: v1.50 Wyze/SimpliSafe and corrected HC-05 signatures")
