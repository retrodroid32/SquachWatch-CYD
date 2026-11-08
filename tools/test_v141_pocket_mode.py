#!/usr/bin/env python3
"""Classic CYD BOOT pocket-mode structural regression checks."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"src/main.cpp").read_text()
assert "static void classicPocketButtonTick(uint32_t now)" in s
assert "pinMode(0, INPUT_PULLUP);" in s
assert "if (!down) s_pocketArmed = true;" in s
assert "held >= 40 && held < 1200" in s
assert "if (!s_pocketDark) lastTouch = now;" in s
assert "if (s_pocketDark) wantDim = !alertWantsWake;" in s
assert "if (s_pocketDark && s_screenDimmed) duty = 0;" in s
assert s.count("#if defined(CYD) && !defined(TWATCH_S3)") >= 5
print("PASS classic CYD BOOT pocket mode invariants")
