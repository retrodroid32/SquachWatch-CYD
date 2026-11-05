#!/usr/bin/env python3
"""Regression checks for 3.2 CYD / 2.8 display-path parity and raw scanner."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
scan = (root / "src/ui_rawscan.cpp").read_text(encoding="utf-8")
checks = {
 "full-frame sprite preferred with two independent headroom checks":
    "largestBefore >= fullFrameBytes + 32768UL" in main and
    "totalBefore >= fullFrameBytes + 65536UL" in main and
    "s_recoveryBands = fullFrameSafe ? 1 : 2;" in main,
 "allocation failure falls back without boot-loop":
    "if (!spriteCreated && s_recoveryBands == 1)" in main and
    "spriteCreated = frame.createSprite(tft.width(), tft.height() / 2);" in main,
 "single-frame path points canvas at sprite":
    "canvas = s_recoveryBands == 1 ? (TFT_eSPI*)&frame : &tft;" in main,
 "banded init avoids physical LCD clear":
    "static TFT_eSPI& initFrameCanvas()" in main and
    "if (frameBufferOk && s_recoveryBands > 1) return (TFT_eSPI&)frame;" in main,
 "raw BLE->WiFi screen init is buffered":
    "uiRawScanInit(initFrameCanvas(), isBle);" in main and
    "uiRawScanInit(*canvas, isBle);" not in main,
 "critical Settings/Power/Log entries buffered":
    all(("ui"+p+"Init(initFrameCanvas());") in main for p in
        ("Settings","Power","Log","Diagnostics","Security")),
 "both scanning and completed scan draw same buttons":
    scan.count("drawBottomBar(t, w, h, isBle);") >= 3 and
    'isBle ? "[ WIFI ]" : "[ BLE ]"' in scan,
 "scan text drawn with canonical font rather than sticky state":
    "t.setTextFont(1);" in scan.split("static void drawBottomBar",1)[1].split("// Confirm panel",1)[0]
    and "t.setTextWrap(false);" in scan.split("static void drawBottomBar",1)[1].split("// Confirm panel",1)[0],
 "scan button paint matches hit-testing":
    "y >= by && y < by + bh" in scan and
    "y >= ay && y < ay + ah" in scan,
 "2.8 default framebuffer untouched":
    "s_recoveryBands = fullFrameSafe ? 1 : 2;" in main
    and "#if defined(CYD32) && defined(GPS_SUPPORT)" in main
}
for label,ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "CYD32 fullframe/scanner guard failed"
print(f"CYD32 screen/scan regression: {len(checks)} checks PASS")
