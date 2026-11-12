#!/usr/bin/env python3
"""Five distinct button steps on both portrait/landscape CYD geometries."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
h=(root/"include/button_font_metrics.h").read_text()
src=(root/"src/theme.cpp").read_text()
ui=(root/"src/ui_settings.cpp").read_text()
def values(name):
    m=re.search(r"\b"+name+r"\[5\]\s*=\s*\{([^}]+)\}",h)
    assert m, name
    return [int(n.strip()) for n in m.group(1).split(",")]
heights=values("HEIGHT")
advances=values("ADVANCE")
assert len(set(heights))==5 and heights==sorted(heights), heights
assert len(set(advances))==5 and advances==sorted(advances), advances
for screen_w in (240,320,480):
    button_w=(screen_w-16-16)//3
    for step in range(5):
        compact=(len("[ SCAN ]")*advances[step]-1 > button_w-6)
        bar_height=max(20,heights[step]+10)
        for title in ("SCAN","LOG","DESK","BLE","WIFI","BACK","CLR"):
            label=title if compact else "[ "+title+" ]"
            pixel_w=len(label)*advances[step]-1
            assert pixel_w<=button_w-6,(screen_w,step,label,pixel_w,button_w)
            assert heights[step]<=bar_height-4
assert "drawButtonLabel(t,x,y,w,h,label,fg,fill)" in src
assert "drawButtonLabel(t,x+2+ox,y+2+ox,w-4-ox,h-4-ox" in src
assert "g.h = ButtonFont::barHeight(Settings::fontStep(Settings::FontKind::BUTTONS))" in src
assert "const bool compact=ButtonFont::width" in src
assert 'Theme::drawButton(t,w-88,y+5,76,29,"TEST",false)' in ui
print("PASS: five distinct button heights and responsive SCAN/LOG/DESK in 240/320/480px")
