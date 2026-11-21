#!/usr/bin/env python3
"""CYD 2.8 R8: boot alone uses R1 memory-recovery sprite bands."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
boot = (root / "src/ui_boot.cpp").read_text(encoding="utf-8")
header = (root / "include/ui_boot.h").read_text(encoding="utf-8")
sd = (root / "src/ui_sd_browser.cpp").read_text(encoding="utf-8")
push = (root / "src/frame_push.cpp").read_text(encoding="utf-8")

guard = "defined(CYD) && !defined(CYD35) && !defined(AWOK)"
assert guard in main
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in main
assert "void uiBootTick(TFT_eSPI& t, uint32_t now, int physicalW = 0," in header

entry = main.split("static void enterBoot() {", 1)[1].split("// True while CLEAR", 1)[0]
assert "if (frameBufferOk && s_recoveryBands > 1) uiBootInit(frame);" in entry
assert "else                                      uiBootInit(*canvas);" in entry

render = main.split("case AppState::BOOT: {", 1)[1].split("case AppState::BINGO:", 1)[0]
assert "#elif defined(CYD) && !defined(CYD35)" in render
cyd28 = render.split("#elif defined(CYD) && !defined(CYD35)", 1)[1].split("#else\n            uiBootTick(*canvas", 1)[0]
assert "drawTwoBand([&](TFT_eSPI& t, bool)" in cyd28
assert "uiBootTick(t, now, tft.width(), tft.height(), true);" in cyd28
assert "drawCrashCard(t, tft.width(), tft.height());" in cyd28
assert "uiBootTick(*canvas, now, 0, 0, true);" in cyd28
assert cyd28.index("drawTwoBand(") < cyd28.index("uiBootTick(*canvas, now, 0, 0, true);")

# BOOT is the only change in rendering. MAIN still retains its stable
# full-frame buffered R1 path; no R5, R6 changes to the SPI cache or strip.
clear = main.split("case AppState::CLEAR: {", 1)[1].split("case AppState::LOG: {", 1)[0]
assert "drawTwoBand([&](TFT_eSPI& t, bool advance)" in clear
assert "uiClearTick(t, now, engine, advance" in clear
assert "FramePush::push(tft, frame.buf(), frame.bufW(), frame.bufH(), x, y)" in main
assert "s_cyd28FullFrame" not in push
assert "[push-r6]" not in main
assert "SdBrowser::draw(t, tft.width(), tft.height());" in main
assert "const int h = physicalH > 0 ? physicalH : t.height();" in sd

# A short sprite must not lay out the title, mascot, scanline and crash
# card using its 15-40px buffer height. Prevent the CRT flash bursts
# on CYD28 only; other boards keep their original splash behavior.
assert "const int h = physicalH > 0 ? physicalH : t.height();" in boot
assert "if (!suppressGlitch)" in boot
assert "const bool glitch = !suppressGlitch && Theme::glitchActive();" in boot
assert "s_chromaAt != now" in boot
assert "drawCrashCard(TFT_eSPI& t, int physicalW = 0, int physicalH = 0)" in main
assert 'Theme::drawSynthwave(t, now, 0, h, 5.0f / 8.0f);' in boot
print("PASS: R8 CYD28 buffered boot splash, real geometry, no scripted flashes; R7 main/SD preserved")
