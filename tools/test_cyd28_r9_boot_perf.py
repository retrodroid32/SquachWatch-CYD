#!/usr/bin/env python3
"""R9 CYD 2.8 boot sprite performance regression guards.

Code-path verification, not a claim of physical FPS. The hardware
serial [boot-r9] line provides actual FPS and worst-frame measurements.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
boot = (root / "src/ui_boot.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
push = (root / "src/frame_push.cpp").read_text(encoding="utf-8")
r7_sd = (root / "src/ui_sd_browser.cpp").read_text(encoding="utf-8")

intro = main.split("case AppState::BOOT: {", 1)[1].split("case AppState::BINGO:", 1)[0]
clear = main.split("case AppState::CLEAR: {", 1)[1].split("case AppState::LOG: {", 1)[0]

# R8 buffer, SD and MAIN recovery remain intact.
assert "drawTwoBand([&](TFT_eSPI& t, bool)" in intro
assert "uiBootTick(t, now, tft.width(), tft.height(), true);" in intro
assert "if (frameBufferOk && s_recoveryBands > 1) uiBootInit(frame);" in main
assert "drawTwoBand([&](TFT_eSPI& t, bool advance)" in clear
assert "uiClearTick(t, now, engine, advance" in clear
assert "FramePush::push(tft, frame.buf(), frame.bufW(), frame.bufH(), x, y)" in main
assert "s_cyd28FullFrame" not in push
assert "SdBrowser::draw(t, tft.width(), tft.height());" in main
assert "const int h = physicalH > 0 ? physicalH : t.height();" in r7_sd

# R9 culls title, gradient, subtitle, mascot, status and scanline by
# real panel Y, not 15/30px stripe height; the TFT_eSPI driver and memory
# allocation path do not change.
assert '#include "draw_band.h"' in boot
assert "if (DrawBand::has(0, 52))" in boot
assert "Theme::drawBangersOutline(t, tx, 8, title, Theme::BLACK," in boot
assert "Theme::BangersSize::LG, 3);" in boot
assert "if (DrawBand::has(46, 47))" in boot
assert "if (DrawBand::has(sy - 4, sy + 27))" in boot
assert "DrawBand::has(h / 5, h)" in boot
assert "if (DrawBand::has(h - 19, h)) t.print(init);" in boot
assert "if (DrawBand::has(phase, phase + 1))" in boot
assert "Theme::drawSynthwave(t, now, 0, h, 5.0f / 8.0f);" in boot
assert "Squachy::drawWaving(" in boot

# All other board variants keep the original headline outline; the
# fast single-pass path is boot-only and only used by R8's CYD28 option.
assert "if (suppressGlitch) {" in boot
assert "Theme::drawBangersText(t, tx + dx, 8 + dy, title" in boot
assert "const bool glitch = !suppressGlitch && Theme::glitchActive();" in boot

# No instrumentation in MAIN; boot-only aggregates once per second.
assert "const uint32_t bootRenderStartUs = micros();" in intro
assert "if (dt >= 1000u)" in intro
assert "[boot-r9] bands=%u fps=%lu render_avg_us=%lu render_max_us=%lu heap=%lu" in intro
assert "[boot-r9]" not in clear

# Cost model: old outline: each physical stripe scanned each glyph
# more than 24 times. R9 scans once only on stripes covering title.
for bands in (8, 16):
    band_height = 240 // bands
    visible_title_stripes = sum((b * band_height < 52 and
                                  (b + 1) * band_height > 0)
                                 for b in range(bands))
    assert visible_title_stripes <= 4
    old_outline_passes = bands * 24
    optimized_outline_passes = visible_title_stripes
    assert optimized_outline_passes < old_outline_passes // 8

print("PASS: R9 single-pass title and per-stripe boot culling; R7 MAIN/SD unchanged")
