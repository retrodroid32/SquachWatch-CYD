#!/usr/bin/env python3
"""CYD28 R6 native full-stripe push + no premature physical screen wipe."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
guard = "#if defined(CYD) && !defined(CYD32) && !defined(CYD35)"

def section(begin, end):
    return main.split(begin, 1)[1].split(end, 1)[0]

push = section("static inline void pushFrame(int x, int y) {", "// Draw a whole screen")
assert guard in push
assert "} else if (s_recoveryBands > 1) {" in push
native = push.split("} else if (s_recoveryBands > 1) {", 1)[1].split("#endif", 1)[0]
assert "frame.pushSprite(x, y);" in native
assert "FramePush::invalidate();" in native
assert "FramePush::push(" not in native
assert "#if defined(CYD32)" in push
assert "FramePush::push(tft, frame.buf(), frame.bufW(), frame.bufH(), x, y)" in push

settings = section("static void enterSettings() {", "static void enterDesk()")
assert guard in settings
assert "if (frameBufferOk && s_recoveryBands > 1)" in settings
assert "uiSettingsInit(frame);" in settings
assert "uiSettingsInit(*canvas);" in settings
assert settings.index("uiSettingsInit(frame);") < settings.index("uiSettingsInit(*canvas);")

boot = section("void setup() {", "// Backlight:")
assert "[cyd28-r6] boot reset=%u" in boot
assert "esp_reset_reason()" in boot
assert "heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)" in boot

draw = section("static __attribute__((noinline, noclone)) void drawTwoBandCore(", "template <typename F>")
assert "frame.fillSprite(Theme::BG);" in draw
assert "pushFrame(0, y);" in draw
assert "frame.setViewport(0, -y, tft.width(), tft.height(), true);" in draw

print("PASS: CYD28 R6 native stripe transfer, safe SETTINGS entry and reboot diagnostics")
