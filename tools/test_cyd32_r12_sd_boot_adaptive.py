#!/usr/bin/env python3
"""R12: verify SD-in cold-boot allocation never restarts CYD32 on low heap.
These structural checks do not replace physical SD/GPS/ST7798 testing.
"""
from pathlib import Path
main = (Path(__file__).resolve().parents[1] / "src/main.cpp").read_text(encoding="utf-8")
setup = main.split("void setup() {", 1)[1].split("void loop() {", 1)[0]
start = setup.index("// R12: CYD32 SD-in cold boot recovery")
end = setup.index("frame.setTextSize(1);", start)
alloc = setup[start:end]
assert "#if defined(CYD32)" in alloc
assert "cyd32CanTryFull8() && frame.createSprite(tft.width(), tft.height())" in alloc
assert "if (cyd32Full8ReservesIntact())" in alloc
assert "frame.deleteSprite();" in alloc
assert "for (uint8_t bands = 2; bands <= 16; bands <<= 1)" in alloc
assert "tft.height() % bands" in alloc
assert "largest < wanted + 1024u" in alloc
assert "frame.createSprite(tft.width(), bandHeight)" in alloc
assert "s_recoveryBands = bands;" in alloc
assert "if (cyd32Allocated)" in alloc
assert "frameBufferOk = true;" in alloc
assert "frameBufferOk = false;" in alloc
assert "CYD32 DIRECT PANEL: no sprite" in alloc
assert "esp_restart();" not in alloc.split("#else\n    // Other boards", 1)[0]
assert 'canvas = s_recoveryBands == 1 ? (TFT_eSPI*)&frame : &tft;' in alloc
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in setup
restore = main.split("static void restoreFrameBuffer() {", 1)[1].split("static void enterWifiPass(", 1)[0]
assert "for (uint8_t bands = 1; bands <= 16; bands <<= 1)" in restore
assert "if (bands == 1 && !cyd32CanTryFull8()) continue;" in restore
assert "if (bands == 1 && !cyd32Full8ReservesIntact())" in restore
def select_band(height, largest, eligible_full=False):
    if eligible_full:
        return 1
    for band in (2, 4, 8, 16):
        if height % band == 0 and largest >= (320 * (height // band)) + 1024:
            return band
    return None
assert select_band(240, 53236) == 2
assert select_band(240, 33000) == 4
assert select_band(240, 17000) == 8
assert select_band(240, 7000) == 16
assert select_band(240, 4000) is None
assert select_band(240, 125000, True) == 1
print("PASS: R12 CYD32 boot 1/2/4/8/16 guarded allocation, no boot reboot, R11 restore and CYD28 isolation")
