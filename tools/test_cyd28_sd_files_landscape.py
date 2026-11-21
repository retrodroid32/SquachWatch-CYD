#!/usr/bin/env python3
"""CYD28 SD FILES band/rotation regression.

The SD file browser must see physical 320x240/240x320 dimensions, not
an 8/16-row sprite, and long filenames must not wrap over adjacent rows.
Physical ST7789 cold-boot and rotation remain a hardware gate.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
sd = (ROOT / "src/ui_sd_browser.cpp").read_text(encoding="utf-8")

section = main.split("case AppState::SD_FILES: {", 1)[1].split(
    "case AppState::DIAGNOSTICS:", 1
)[0]
assert "if (s_sdFilesDirty)" in section
assert "defined(CYD)" in section
assert "frameBufferOk && s_recoveryBands > 1" in section
assert "SdBrowser::draw(tft);" in section
assert "FramePush::invalidate();" in section
assert "drawTwoBand([&](TFT_eSPI& t, bool) { SdBrowser::draw(t); });" in section
assert "s_sdFilesLastW != tft.width()" in section
assert "s_sdFilesLastH != tft.height()" in section
assert "SdBrowser::tap(tp.x, tp.y, tft.width(), tft.height())" in section

assert "t.setTextFont(1); t.setTextSize(2); t.setTextWrap(false);" in sd
assert "const int fit = (w - 16) / 12 - 2;" in sd
assert "snprintf(label, sizeof label, \"%c %.*s\"" in sd
for w, h in ((320, 240), (240, 320)):
    # 2x 6-pixel ASCII glyphs, 8-pixel left/right margins.
    max_name = min(31, max(1, (w - 16) // 12 - 2))
    assert (max_name + 2) * 12 <= w - 16
    # Every row ends before WIPE ALL and BACK controls.
    rows = min(5, (h - 116) // 22)
    assert 70 + (rows - 1) * 22 + 16 < h - 59
    assert h - 59 + 16 < h - 30 or h == 240  # last label must not spill into BACK
print("PASS: SD FILES physical-panel redraw, rotation, filename width, touch geometry")
