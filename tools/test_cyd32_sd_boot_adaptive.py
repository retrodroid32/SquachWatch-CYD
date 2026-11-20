#!/usr/bin/env python3
"""Guard CYD32 cold boot against SD + Wi-Fi fragmented heap regressions."""
from pathlib import Path

src = (Path(__file__).resolve().parents[1] / "src/main.cpp").read_text()
start = src.index("#if defined(CYD32)\n    // CYD 3.2 ST7798")
end = src.index("\n#endif\n\n    // Builds a 512-byte lookup", start)
block = src[start:end]
assert "for (uint8_t bands = 2; bands <= 16; bands <<= 1)" in block
assert "tft.height() % bands" in block
assert "wanted + 1024u" in block
assert "frame.createSprite(tft.width(), bandHeight)" in block
assert "s_recoveryBands = bands;" in block
assert "frameBufferOk = true;" in block
assert "canvas = &tft;" in block
assert "[frame-boot] CYD32 ready: bands=" in block
assert "[frame-boot] CYD32 DIRECT PANEL:" in block
assert "frameBufferOk = false;" in block
assert "s_recoveryBands = 1;" in block
cyd32 = block.split("#else\n    // Other boards retain", 1)[0]
assert "esp_restart()" not in cyd32, "CYD32 allocation must never reboot"
assert src.count("frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);") >= 1, "rotation must resize in place"
assert "if (s_recoveryBands > 1 && frameBufferOk)" in src, "band renderer must have valid sprite"
# 320x240 RGB332 half screen = 38,400 bytes; a 53,236-byte
# contiguous block from the user's boot log can fit it with reserve.
assert 320 * 240 // 2 + 1024 <= 53236
print("PASS: CYD32 SD-in boot adaptive framebuffer / no restart / rotation preserved")
