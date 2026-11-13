#!/usr/bin/env python3
"""Layout, touch, classification, and category-action regression guards."""
from pathlib import Path

r = Path(__file__).resolve().parents[1]
clear = (r / "src/ui_clear.cpp").read_text()
main = (r / "src/main.cpp").read_text()
log = (r / "src/ui_log.cpp").read_text()
headers = (r / "include/category_roster.h").read_text()

assert "uiClearCategoryHit(int x, int y, int screenW, int screenH)" in clear
assert "categoryPillRect(layout, row, col, px, py, pw, ph);" in clear
assert "categoryPillRect(l, row, col, px, py, pw, ph);" in clear
assert "screenH > 0 ? screenH : t.height()" in clear
assert "s_clearLayout.bar.h == Theme::computeButtonBar(w, h).h" in clear
assert 't.drawRoundRect(px, py, pw, ph, 3, outline);' in clear
assert 't.print(num);' in clear
assert "enterCategoryLog(tappedCategory);" in main
assert "armLogConfirmAt(gestureStartX, gestureStartY);" in main
assert "if (uiLogCategoryHitBack(gestureStartX, gestureStartY," in main
assert "uiLogTick(t, now, engine, 0," in main
assert "tft.width(), tft.height());" in main
assert "CategoryRoster::matches(s_category, d->type)" in log
assert "CategoryRoster::fresh(*d, now) == (pass == 0)" in log
assert "const bool fresh = CategoryRoster::fresh(*d, now);" in log
assert "inline bool fresh(const Detection& d, uint32_t now)" in headers
assert "uiLogRowCount(eng, now)" in log
assert "Theme::drawActiveBackground(t, now, bgTop, bodyBottom, eng, advance)" in log
assert "case Settings::Background::FIRE" not in log
assert "const uint32_t age = kept ? 0 : (uint32_t)(now - d->lastSeen) / 1000u;" in log
assert 'else t.printf("%ddBm", d->rssi);' in log
assert 'if (s_category != DetectionType::UNKNOWN && !fresh)' in log
assert '(d.channel ? 6500 : 4000)' in headers
assert 't.printf("LAST %lu%c", (unsigned long)(seconds ? age : age / 60u),' in log
assert "s_category != DetectionType::UNKNOWN" in log
assert "if (s_category != DetectionType::UNKNOWN) return categoryReindex(eng);" in log
assert 'Theme::drawButton(t, bar.x[1], bar.y, bar.w[1], bar.h, "BACK", false)' in log
assert "actual == DetectionType::RING" in headers
assert "actual == DetectionType::EVILTWIN" in headers

# Panel-sized rectangles must stay inside their physical display, with no
# overlap between hit targets. This also tests the half-height 3.2 sprite
# failure: all rectangles are calculated using *physical*, never band height.
for w, h in ((240, 320), (320, 240), (320, 480), (480, 320)):
    for beacon in (False, True):
        n = 12 if beacon else 11
        rows = 2 if w > h else 3
        base, remainder = divmod(n, rows)
        bar_h = 24
        bar_y = h - bar_h - 6
        top = bar_y - rows * 22 - 5
        rects = []
        at = 0
        for row in range(rows):
            cols = base + (row < remainder)
            gap, margin = 3, 5
            cell = (w - 2 * margin - (cols - 1) * gap) // cols
            used = cols * cell + (cols - 1) * gap
            for col in range(cols):
                x = (w - used) // 2 + col * (cell + gap)
                y = top + row * 22
                rw, rh = cell, 20
                rects.append((x, y, rw, rh))
                assert 0 <= x < x + rw <= w
                assert 30 < y < y + rh < bar_y
                assert rw >= 45, (w, h, cols, rw)
                for px, py, pw, ph in rects[:-1]:
                    assert not (x < px + pw and x + rw > px and
                                y < py + ph and y + rh > py)
            at += cols
        assert at == n
        assert top > h // 3

print("PASS: 2.8/3.2 category pills, rotation, touch, stale RSSI, grouped identity")
