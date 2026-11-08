#!/usr/bin/env python3
"""Verify upstream v1.41 confirmation privacy routing; physical tests still required."""
from pathlib import Path
src = (Path(__file__).resolve().parents[1] / "src/main.cpp").read_text()
assert "static bool s_confirmLabelIsName = false;" in src
assert "return s_confirmLabelIsName ? privLabel(s_confirmLabel) : s_confirmLabel;" in src
assert src.count("s_confirmPending, confirmLabelOnScreen(),") == 2
assert "s_confirmLabelIsName = d->name[0] != 0;" in src
assert "s_confirmLabelIsName = true; // BLE self-name / placeholder" in src
assert "s_confirmLabelIsName = true; // Wi-Fi SSID / hidden placeholder" in src
assert "s_confirmPending, privLabel(s_confirmLabel)," not in src
print("PASS privacy confirmation routing (LOG+RAWSCAN)")
