#!/usr/bin/env python3
"""Static ordering/geometry guard for CYD35's two-band CLEAR renderer.

This does NOT substitute for actual CYD35C pixel/touch hardware testing. It
catches the specific regression where a toast or zone card was drawn only
*after* both framebuffer halves had already been pushed to the TFT.
"""
from pathlib import Path

main = (Path(__file__).resolve().parents[1] / "src" / "main.cpp").read_text()
start = main.index("            if (drawClear) {")
end = main.index("            } else {\n                renderedThisLoop = false;", start)
clear = main[start:end]

first = clear.index("DrawBand::set(0, halfH);")
push_top = clear.index("pushFrame(0, 0);", first)
second = clear.index("DrawBand::set(halfH, tft.height());", push_top)
push_bottom = clear.index("pushFrame(0, halfH);", second)

for label, paint in [
    ("upper", clear[first:push_top]),
    ("lower", clear[second:push_bottom]),
]:
    assert "uiZoneCardDraw(frame, now)" in paint, label + ": zone card after push"
    assert "Theme::drawToast(frame, now)" in paint, label + ": toast after push"

assert "!frameBufferOk" in clear[push_bottom:], (
    "CYD35 buffered CLEAR path must not draw overlays after both pushes"
)
for height in (320, 480):
    split = height // 2
    for label, y0, y1 in [
        ("zone", (height - 96) // 2 - 8, (height - 96) // 2 - 8 + 96),
        ("toast", (height - 34) // 2, (height - 34) // 2 + 34),
    ]:
        assert y1 > 0 and y0 < split, (height, label, "missing top")
        assert y1 > split and y0 < height, (height, label, "missing bottom")

print("CYD35 CLEAR overlay order/geometry: PASS (compile + real hardware still required)")
