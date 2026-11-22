#!/usr/bin/env python3
"""R10: SquachEmit R12-R14.7 parity with conservative CYD32 GPS memory policy.

Source-level regression, not physical ST7798 proof.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
theme = (root / "src/theme.cpp").read_text(encoding="utf-8")
boot = (root / "src/ui_boot.cpp").read_text(encoding="utf-8")
band = (root / "src/draw_band.cpp").read_text(encoding="utf-8")
push = (root / "src/frame_push.cpp").read_text(encoding="utf-8")
clear_ui = (root / "src/ui_clear.cpp").read_text(encoding="utf-8")

def between(content, start, end):
    return content.split(start, 1)[1].split(end, 1)[0]

# CYD32 retains HALF8 as a guaranteed fallback; R11 may choose FULL8
# only after explicit free/largest contiguous heap safety checks.
setup = between(main, "void setup() {", "void loop() {") if "void setup() {" in main else main
assert "if (!cyd32Allocated)" in setup
assert "for (uint8_t bands = 2; bands <= 16; bands <<= 1)" in setup
assert "if (cyd32CanTryFull8() &&" in setup
assert "if (cyd32Full8ReservesIntact())" in setup
assert "frame.deleteSprite();" in setup
assert "#if defined(CYD32) && defined(GPS_SUPPORT) && defined(GPS_RX_PIN)" in main

# ST7798 must stay on safe native TFT_eSPI push, with cached rows invalidated.
transfer = between(main, "static inline void pushFrame(int x, int y) {", "// Draw a whole screen")
assert "#if defined(CYD32)" in transfer
assert "frame.pushSprite(x, y);" in transfer
assert "FramePush::invalidate();" in transfer
assert "[push] CYD32 NATIVE TFT_eSPI pushSprite" in transfer
assert "FramePush::push(tft, frame.buf()" in transfer  # other boards only

# One logical now, DrawBand row clipping and first-band-only advancement.
core = between(main, "static __attribute__((noinline, noclone)) void drawTwoBandCore(", "template <typename F>")
assert "DrawBand::setEnabled(true);" in core
assert "DrawBand::set(y, y + bandH);" in core
assert "frame.setViewport(0, -y, tft.width(), tft.height(), true);" in core
assert "invoke(context, (TFT_eSPI&)frame, b == 0);" in core
assert "frame.resetViewport();" in core
assert "DrawBand::setEnabled(false);" in core
clear_case = between(main, "case AppState::CLEAR: {", "case AppState::LOG: {")
assert "drawTwoBand([&](TFT_eSPI& t, bool advance)" in clear_case
assert "uiClearTick(t, now, engine, advance" in clear_case
assert "Theme::drawActiveBackground(t, now, 0, h, eng, advance);" in clear_ui

# Original SquachWatch wrapper controls s_animK and owns background releases.
background = between(theme, "void drawActiveBackground(", "// Where the gold toaster")
assert "if (advance)" in background and "s_animK = 0.0f;" in background
assert "case Settings::Background::SPECTRUM:   drawGibson(t, now" in background
assert "case Settings::Background::SNOWFALL:   drawSnowfall(t, now" in background
assert "case Settings::Background::SYNTHWAVE:  drawSynthwave(t, now" in background
assert "drawDigitalRain(t, now, yStart, yEnd, advance)" in background

# Gibson's previous dt=16 for a repeated timestamp caused upper/lower
# camera drift; R10 makes the second pass dt=0.
# Gibson is located after the wrapper in the large source module; its
# implementation runs to the end of the file in this fork.
gib = theme.split("void drawGibson(", 1)[1]
assert "now == lastMs ? 0u" in gib
assert "if (dt) flyZ = fmodf" in gib
assert "uint32_t dt = (lastMs && now > lastMs) ? (now - lastMs) : 16u;" not in gib

rain = between(theme, "void drawDigitalRain(", "void drawActiveBackground(")
assert "(uint32_t)(now - s_redGlyphSaid) < 500u;" in rain
assert "(uint32_t)(millis() - s_redGlyphSaid) < 500u;" not in rain

# CYD32 splash must not layout against a half-height sprite.
boot_case = between(main, "case AppState::BOOT: {", "case AppState::BINGO:")
cyd32 = between(boot_case, "#elif defined(CYD32)", "#elif defined(CYD)")
assert "drawTwoBand([&](TFT_eSPI& t, bool)" in cyd32
assert "uiBootTick(t, now, tft.width(), tft.height(), true);" in cyd32
assert "drawCrashCard(t, tft.width(), tft.height());" in cyd32
assert "uiBootTick(*canvas, now, 0, 0, true);" in cyd32
entry = between(main, "static void enterBoot() {", "// True while CLEAR")
assert "#if defined(CYD32)" in entry
assert "uiBootInit(frame);" in entry
assert "const int h = physicalH > 0 ? physicalH : t.height();" in boot
assert "Theme::drawSynthwave(t, now, 0, h" in boot
assert "Squachy::drawWaving(" in boot
assert "if (DrawBand::has(0, 52))" in boot   # R9 title optimization preserved

# Measured frame FPS based on finished buffers, with separate drawing and
# SPI push time and largest contiguous heap after screen render.
assert "[cyd32-frame] screen=%s fps=%lu bands=%u" in core
assert "mode=NATIVE" in core
assert "heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)" in core
assert "if (totalPushUs > pushWorstUs)" in core
assert "if (totalDrawUs > drawWorstUs)" in core

# R7-R9 classic 2.8's recovery path is preserved (including SD).
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in main
assert "SdBrowser::draw(t, tft.width(), tft.height());" in main
assert "[boot-r9]" in main
assert "#if defined(CYD35) || (defined(ARDUINO_ARCH_ESP32)" in band or "g_on =" in band
assert "static uint16_t s_lut[256];" in push

# Model check: same logical timestamp in both bands => 1 camera step.
def gibson_dt(previous, now):
    d = 16 if not previous else (0 if now == previous else (now - previous) & 0xFFFFFFFF)
    return min(d, 100)
assert gibson_dt(1234, 1250) == 16
assert gibson_dt(1250, 1250) == 0
assert gibson_dt(1250, 1351) == 100
print("PASS: R10 ST7798 boot geometry/Gibson sync/Digital Rain timing/native SPI/heap; R7-R9 CYD28 untouched")
