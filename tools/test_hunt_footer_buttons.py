#!/usr/bin/env python3
"""HUNT three-button footer regression: shared layout and exclusive hit targets."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
ui=(r/"src/ui_hunt.cpp").read_text()
main=(r/"src/main.cpp").read_text()
theme=(r/"src/theme.cpp").read_text()
font=(r/"include/button_font_metrics.h").read_text()
assert "enum HuntFooterSlot : uint8_t { HUNT_BACK = 0, HUNT_GRAPH = 1, HUNT_STOP = 2 }" in ui
assert "Theme::computeButtonBar(screenW, screenH)" in ui
assert "x = g.x[slot];" in ui and "w = g.w[slot];" in ui and "y = g.y;" in ui and "h = g.h;" in ui
for slot, label in (("HUNT_BACK","BACK"),("HUNT_STOP","STOP")):
    assert "return hitHuntFooter("+slot+", x, y, screenW, screenH);" in ui
    assert "huntFooterRect("+slot+", w, h, bx, by, bw, bh);" in ui
    assert 'Theme::drawButton(t, bx, by, bw, bh, "'+label+'", false);' in ui
assert "return hitHuntFooter(HUNT_GRAPH, x, y, screenW, screenH);" in ui
assert "huntFooterRect(HUNT_GRAPH, w, h, bx, by, bw, bh);" in ui
assert 'Theme::drawButton(t, bx, by, bw, bh, s_graphMode ? "GAUGE" : "GRAPH", false)' in ui
assert "t.drawRect(8, 21, 64, 24" not in ui
assert "x >= 8 && x <= 72 && y >= 21" not in ui
assert "touchJustDown && (now - lastTouch) > TOUCH_DEBOUNCE_MS" in main
assert "uiHuntHitGraph(tp.x, tp.y, tft.width(), tft.height())" in main
assert "uiHuntToggleGraph();" in main
assert "const int margin = 8, gap = 8;" in theme
assert "int bw = (screenW - 2 * margin - 2 * gap) / 3;" in theme
assert "g.y = screenH - g.h - 6;" in theme
assert "static const uint8_t HEIGHT[5] = {8, 11, 14, 18, 22};" in font
assert "static const uint8_t ADVANCE[5] = {6, 7, 8, 9, 10};" in font
for screenW,screenH in [(240,320),(320,240),(240,320),(320,240),(480,320),(320,480)]:
  for step,(h,adv) in enumerate(zip((8,11,14,18,22),(6,7,8,9,10))):
    bh=max(20,h+10)
    y=screenH-bh-6
    bw=(screenW-2*8-2*8)//3
    xs=[8,8+bw+8,8+2*bw+16]
    rects=[(x,y,bw,bh) for x in xs]
    assert y>45 and y+bh<=screenH
    assert xs[0]+bw<xs[1] and xs[1]+bw<xs[2]
    assert xs[2]+bw<=screenW-8
    assert all((len(label)*adv-1)<=bw-6 for label in ("BACK","GRAPH","GAUGE","STOP"))
    for i,(x,ry,w,rh) in enumerate(rects):
      mx,my=x+w//2,ry+rh//2
      hits=[j for j,(a,b,c,d) in enumerate(rects)
            if a<=mx<a+c and b<=my<b+d]
      assert hits==[i],(screenW,step,i,hits)
print("PASS HUNT footer: shared 3-slot geometry, font 1-5 fit, no overlap at both orientations")
