#!/usr/bin/env python3
"""Upstream v1.41 Arlo OUI regression guard; hardware acceptance separate."""
from pathlib import Path
import re
src = (Path(__file__).resolve().parents[1] / "src/signatures.cpp").read_text()
for oui in ("FC, 0x9C, 0x98", "48, 0x62, 0x64", "A4, 0x11, 0x62"):
    prefix = "0x" + oui
    pat = re.escape(prefix) + r'\}\s*,\s*"Arlo"\s*,\s*DetectionType::CAMERA\s*,\s*Confidence::HIGH_CONF'
    assert re.search(pat, src), "missing high-confidence Arlo OUI " + prefix
assert not re.search(r'\{\{0xBC,\s*0xDD,\s*0xC2\}\s*,\s*"Arlo"', src), "Espressif block mislabelled as Arlo"
print("PASS upstream v1.41 Arlo OUI signatures")
