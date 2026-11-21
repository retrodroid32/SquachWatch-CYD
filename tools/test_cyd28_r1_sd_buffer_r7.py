#!/usr/bin/env python3
"""R7: R1 render pipeline, with SD FILES geometry corrected in-buffer only."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
sd = (root / "src/ui_sd_browser.cpp").read_text(encoding="utf-8")
hdr = (root / "include/ui_sd_browser.h").read_text(encoding="utf-8")
push = (root / "src/frame_push.cpp").read_text(encoding="utf-8")
sdlog = (root / "src/sd_log.cpp").read_text(encoding="utf-8")
det = (root / "src/detection.cpp").read_text(encoding="utf-8")

# The R1 low-heap allocation and SD deferral remain intact.
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in main
assert "const uint32_t wanted = (uint32_t)tft.width() * (uint32_t)bandHeight;" in main
assert "SD DEFERRED - LOW RAM" in sdlog
assert "if (sdHeap < 45000u || sdLargest < 28000u)" in sdlog
assert "_sd.begin();" in det

# R1 main screen, stripes and hash transfer were *not* replaced by
# direct-panel writes, sprite scrubbing or the R5/R6 native transfer.
clearscreen = main.split("case AppState::CLEAR: {", 1)[1].split(
    "case AppState::LOG: {", 1)[0]
assert "drawTwoBand([&](TFT_eSPI& t, bool advance) {" in clearscreen
assert "uiClearTick(t, now, engine, advance" in clearscreen
assert "FramePush::push(tft, frame.buf(), frame.bufW(), frame.bufH(), x, y)" in main
assert "frame.fillSprite(Theme::BG);" not in main.split(
    "static __attribute__((noinline, noclone)) void drawTwoBandCore(", 1)[1].split(
    "template <typename F>", 1)[0]
assert "s_cyd28FullFrame" not in push
assert "[push-r6]" not in main

# R2's direct TFT SD path is disabled for classic CYD 2.8.
browser = main.split("case AppState::SD_FILES: {", 1)[1].split(
    "case AppState::DIAGNOSTICS: {", 1)[0]
assert "SdBrowser::draw(tft);" in browser  # existing CYD32 only
assert "defined(CYD) && !defined(CYD35) && !defined(AWOK)" in browser
buffered = browser.split("#elif defined(CYD)", 1)[1].split("#else", 1)[0]
assert "drawTwoBand([&](TFT_eSPI& t, bool)" in buffered
assert "SdBrowser::draw(t, tft.width(), tft.height());" in buffered
assert "SdBrowser::draw(tft);" not in buffered
assert "FramePush::invalidate();" not in buffered

# All file browser buttons, text and hit boxes use physical rotated size,
# and wrapping cannot cause cross-stripe text artefacts.
assert "void draw(TFT_eSPI& t, int physicalW = 0, int physicalH = 0);" in hdr
assert "const int w = physicalW > 0 ? physicalW : t.width();" in sd
assert "const int h = physicalH > 0 ? physicalH : t.height();" in sd
assert "t.fillRect(0, 0, w, h, Theme::BG);" in sd
assert 'snprintf(label, sizeof label, "%c %.*s"' in sd
assert "t.setTextWrap(false);" in sd
assert "Theme::drawButton(t, 0, h - 30, w, 30" in sd

# The old R1 direct canvas pointer must not blank the LCD when leaving
# SD FILES and the small buffered screen resumes drawing.
settings = main.split("static void enterSettings() {", 1)[1].split(
    "static void enterDesk()", 1)[0]
assert "uiSettingsInit(frame);" in settings
assert "uiSettingsInit(*canvas);" in settings
print("PASS: R7 R1 framebuffer preserved; SD browser physical geometry remains buffered")
