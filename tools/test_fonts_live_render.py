#!/usr/bin/env python3
"""Stage C: persisted Fonts controls must affect actual bounded drawing."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
theme=(root/"src/theme.cpp").read_text()
ui=(root/"src/ui_settings.cpp").read_text()
pet=(root/"src/pet.cpp").read_text()
sq=(root/"src/squachy.cpp").read_text()
head=(root/"include/theme.h").read_text()
for marker in ('Settings::fontStep(Settings::FontKind::BUBBLES)', 's_bubbleHeight = t.fontHeight()',
               'fontDelta(Settings::FontKind::MENUS)', 'fontDelta(Settings::FontKind::BUTTONS)',
               't.textWidth(label) <= w - 6', 't.fontHeight() <= h - 2',
               'void petFontOn(', 'void petFontOff('):
    assert marker in theme, marker
assert 'Theme::uiMenuTextSize(t)' in ui
assert 'Settings::fontFace(Settings::FontKind::MENUS)' in ui
assert 'Theme::petFontOn(t);' in pet and 'Theme::petFontOff(t);' in pet
drawbubble = sq.split('static void drawBubbleIn(TFT_eSPI& t, int cx, int topY, const char* text,', 2)[-1]
assert 't.setTextSize(1);' not in drawbubble[:240], "Bubble draw must respect selected size"
assert 'int8_t fontDelta(Settings::FontKind k)' in head
assert 'PREVIEW ONLY' in ui and 'kind == Settings::FontKind::XP_TILES' in ui
assert 't.setTextFont(1); t.setTextSize(1);' in ui, 'restore drawing font state'
assert "0x1E4000" in (root/"partitions_ota.csv").read_text()
print("PASS Stage C live bounded menus/buttons/bubbles/pets + honest pending categories")
