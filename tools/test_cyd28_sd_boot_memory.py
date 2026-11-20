#!/usr/bin/env python3
"""Source-level regression guards for classic CYD 2.8 SD-in startup recovery.

Hardware allocation/crypto tests must still be run on 2.8 ST7789.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text()
det = (root / "src/detection.cpp").read_text()
sd = (root / "src/sd_log.cpp").read_text()

start = main.index("#if defined(CYD) && !defined(CYD32) && !defined(CYD35) && !defined(AWOK) && !defined(RLPHANTOM) && !defined(RLPHANTOM_R)\n    // CYD 2.8")
end = main.index("#else\n    // Other boards retain", start)
cyd = main[start:end]
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in cyd
assert "tft.height() % bands" in cyd
assert "frame.createSprite(tft.width(), bandHeight)" in cyd
assert "canvas = &tft;" in cyd
assert "[frame-boot] CYD28 ready: bands=" in cyd
assert "[frame-boot] CYD28 DIRECT PANEL:" in cyd
assert "esp_restart()" not in cyd
assert "s_recoveryBands = bands;" in cyd
assert "frameBufferOk = false;" in cyd
assert "if (s_recoveryBands > 1 && frameBufferOk)" in main
assert "frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);" in main
assert "SD DEFERRED - LOW RAM" in sd
assert "sdHeap < 45000u || sdLargest < 28000u" in sd
assert sd.index("SD DEFERRED - LOW RAM") < sd.index("if (!SD.begin(")
assert "other boards preserve original startup behavior" in det
assert "engine.sd().startMonitor();" in main
assert main.index("MeshTalk::begin();") < main.index("engine.sd().startMonitor();")
assert "[sd] CYD28 monitor deferred:" in main
assert "ESP.getFreeHeap() >= 24000u" in main
print("PASS: classic CYD 2.8 SD-in boot memory reserve, frame fallback, mesh-before-monitor")
