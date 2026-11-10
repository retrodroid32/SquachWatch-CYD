#!/usr/bin/env python3
"""v1.41 passive Wi-Fi survey guards (hardware/packet test still required)."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
d = (root/"src/detection.cpp").read_text()
o = (root/"src/ota_wifi.cpp").read_text()
for needle in (
    "WiFi.scanNetworks(false, true, true, 150)",
    "WiFi.scanNetworks(true /* async */, true /* hidden */, true /* passive */, 150)",
    "WiFi.scanNetworks(true, true, true, 180)",
):
    assert needle in d, needle
for needle in (
    "WiFi.scanNetworks(true /* async */, false /* no hidden */, true /* passive */, 150)",
    "WiFi.scanNetworks(false, false, true, 180)",
    "WiFi.scanNetworks(false, false, true, 130)",
):
    assert needle in o, needle
print("PASS passive Wi-Fi survey callsites")
