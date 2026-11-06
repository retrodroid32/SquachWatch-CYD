#!/usr/bin/env python3
"""Static guard for LOG footer redraw on full-frame and multi-band CYDs.

This is a source-integration guard, not proof of flicker-free physical TFT
behavior. Check the 3.2 ST7798 after flashing the LAB build.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
log = (root / "src/ui_log.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
theme = (root / "src/theme.cpp").read_text(encoding="utf-8")
start = log.index("static void drawLogFooter(")
end = log.index("void uiLogInit(", start)
footer = log[start:end]
tick = log[log.index("void uiLogTick("):]
checks = {
    "footer paints an opaque full-width plate from list bottom to panel bottom":
       "const int top = bar.y - 4;" in footer
       and "t.fillRect(0, top, t.width(), t.height() - top, Theme::BG);" in footer,
    "footer painting is clipped to the active sprite band":
       '#include "draw_band.h"' in log
       and "if (!DrawBand::has(top, t.height())) return;" in footer,
    "LOG controls always use predictable GLCD text/font settings":
       "t.setTextFont(1);" in footer
       and "t.setTextWrap(false);" in footer
       and "t.setTextSize(1);" in footer,
    "all empty, EVENTS and DEVICES paths use same footer":
       tick.count("drawLogFooter(t, bar);") == 3
       and tick.count("Theme::drawButtonBar(t, ButtonId::LOG") == 0,
    "footer remains under modal overlays":
       "else if (confirmPending) drawConfirmPanel" in tick
       and tick.rindex("drawLogFooter(t, bar);") <
           tick.rindex("else if (confirmPending) drawConfirmPanel"),
    "background still bounded above footer":
       "const int bodyBottom = bar.y - 4;" in tick,
    "footer is offscreen-buffered as before":
       "drawTwoBand([&](TFT_eSPI& t, bool)" in main
       and "uiLogTick(t, now, engine" in main,
    "Log soft button layout and semantics unchanged":
       '"[ CLR ]"' in theme and "ButtonBarMode::LOG" in footer
}
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
assert all(checks.values()), "LOG footer repair incomplete"
print(f"LOG footer: {len(checks)} checks PASS")
