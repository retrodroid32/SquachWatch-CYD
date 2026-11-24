#!/usr/bin/env python3
"""Prevent accidental replacement of FNK0104N's SquachWatch UI with a mock."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
pio = (ROOT / "platformio.ini").read_text()
source = (ROOT / "src/freenove35_main.cpp").read_text()
detector = (ROOT / "src/detection.cpp").read_text()
docs = (ROOT / "docs/FREENOVE35_N_LAB.md").read_text()

def require(ok, msg):
    if not ok:
        raise AssertionError(msg)

profile = pio.split("[env:freenove35-st77922]", 1)[1]
require("-<main.cpp>" in profile and "+<*>" in profile,
        "FNK0104N must link the fork's real application modules")
require("-DSQUACH_MESH=1" in profile and "${env.nimble_flags_mesh}" in profile,
        "FNK0104N must match other S3 fork targets for mascot visit and mesh APIs")
required = {
    "uiBootTick(": "original boot splash",
    "uiClearTick(": "original SquachWatch idle/Squachy/animation screen",
    "uiSettingsTick(": "original settings layout",
    "uiLogTick(": "original detection log",
    "uiRawScanTick(": "original BLE/Wi-Fi raw scan",
    "engine.init(": "shared BLE/Wi-Fi detection engine",
    "engine.loop(": "continuous shared radio processing",
    "Squachy::trigger(": "original mascot behavior",
    "Settings::cycleBackground(": "background selection",
    "Settings::cyclePrevBackground(": "backward background selection",
    "St77922::queuePixels(": "exact upstream ST77922 QSPI transfer",
    "present()": "LCD framebuffer push",
    "pollTouch()": "capacitive touch input",
}
for token, label in required.items():
    require(token in source, "FNK0104N missing: " + label)
require("#if !defined(FREENOVE35_N)" in detector and "_sd.begin()" in detector,
        "Classic CYD SPI SD probing must be guarded on FNK0104N")
upstream_qspi = (ROOT / "src/st77922_qspi.cpp").read_text()
upstream_touch = (ROOT / "src/st77922_touch.cpp").read_text()
require("-DFREENOVE_S3_35=1" in profile and "St77922::begin()" in source,
        "Freenove must use the actual SquachWatch upstream hardware driver")
require("RAM_WRITE, nullptr, 0" in upstream_qspi and "RAM_CONTINUE << 8" in upstream_qspi,
        "Keep upstream's RAMWR command followed by RAMWRC QSPI pixel streaming")
require("St77922::reinit()" in source and "St77922Touch::begin" in source,
        "Reinitialize the panel after integrated capacitive-touch reset")
require("REG_POINT0" in upstream_touch, "Keep upstream ST77922 touch reporting")
require("frameErrors" in source and "LCD frames=" in source,
        "Provide serial diagnostics for frozen/scrambled LCD reports")
require("complete functional parity" in docs.lower(),
        "Do not claim that R2 means all existing fork features have shipped")
print("Freenove native SquachWatch UI/source guard PASS")
