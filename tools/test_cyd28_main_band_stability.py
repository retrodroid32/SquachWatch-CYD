#!/usr/bin/env python3
"""CYD 2.8 main-screen stripe reuse + physical refresh cadence guards."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
main = (root / "src/main.cpp").read_text(encoding="utf-8")
push = (root / "src/frame_push.cpp").read_text(encoding="utf-8")
guard = "#if defined(CYD) && !defined(CYD32) && !defined(CYD35)"
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in main
draw = main.split("static __attribute__((noinline, noclone)) void drawTwoBandCore(", 1)[1].split(
    "template <typename F>", 1)[0]
assert "frame.resetViewport();\n            frame.fillSprite(Theme::BG);" in draw
assert draw.index("frame.resetViewport();\n            frame.fillSprite") < draw.index("frame.setViewport(0, -y,")
assert draw.index("frame.setViewport(0, -y,") < draw.index("invoke(context,")
assert "pushFrame(0, y);" in draw
assert "DrawBand::set(y, y + bandH);" in draw
entry = main.split("static void enterClear() {", 1)[1].split("static uint32_t outfitUnlockStart", 1)[0]
assert "if (!(frameBufferOk && s_recoveryBands > 1)) uiClearInit(*canvas);" in entry
assert "FramePush::invalidate();" in entry
assert guard in push
begin = push.split("void newFrame() {\n    s_lastRows = 0;", 1)[1].split("#endif", 1)[0]
assert "++s_frameNo;" in begin
assert "s_cyd28FullFrame = !s_valid || (s_frameNo % 64u) == 0u;" in begin
assert "s_cyd28FullFrame = true;" in push.split("void invalidate() {", 1)[1].split("}", 1)[0]
first = push.split("const bool full = s_cyd28FullFrame", 1)[1].split("#else", 1)[0]
assert "s_frameNo++" not in first
for bands in (8, 16):
    for screen_frame in range(1, 129):
        observed = [(screen_frame % 64 == 0) for _ in range(bands)]
        assert len(set(observed)) == 1
        assert observed[0] == (screen_frame in (64, 128))
print("PASS: CYD28 main refresh cadence, sprite scrub, transition invalidation")
