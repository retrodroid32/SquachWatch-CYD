#!/usr/bin/env python3
"""Guard physical-panel repaint order and Settings/Power Saver Back semantics.

A rotated 3.2 ST7798 CYD was observed with stale bottom rows hiding BACK.
Host checks do not replace portrait/landscape hardware acceptance.
"""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
main = (r / "src/main.cpp").read_text()
settings = (r / "src/ui_settings.cpp").read_text()
head = (r / "include/ui_settings.h").read_text()
theme = (r / "src/theme.cpp").read_text()
start = main.index("// 3.2\" CYD portrait after rotation can retain")
end = main.index("if ((state == AppState::WATCH_LIST || state == AppState::HUNT_LIST)", start)
block = main[start:end]
checks = {
  "post-frame push overlay (not overwritten by buffered frame)":
     main.index("pushFrame(0, 0);", main.index("// Skipped on cyd35")) < start,
  "only 3.2 board and portrait":
     "#if defined(CYD32)" in main[start-100:start]
     and "tft.height() > tft.width()" in block,
  "both Settings and Power Saver covered":
     "AppState::SETTINGS" in block and "AppState::POWER_SAVER" in block,
  "Settings respects BACK/UP/desk OK":
     "uiSettingsDrawPinnedBackOverlay(tft)" in block
     and "drawPinnedBack(panel, panel.width(), panel.height())" in settings
     and "s_page == SettingsPage::DESK" in settings
     and 'SettingsPage::MAIN) ? "[ BACK ]" : "[ UP ]"' in settings
     and "uiSettingsDrawPinnedBackOverlay" in head,
  "Power Saver strip uses shared layout and touch target":
     'Theme::drawPinnedBack(tft, "[ BACK ]")' in block
     and "pinnedBackHit(gestureStartX, gestureStartY, tft.width(), tft.height())" in main,
  "overlay row-hash invalidated when leaving":
     "s_directPinnedBackWasDrawn" in block and "FramePush::invalidate()" in block,
  "shared strip height is also used by touch testing":
     "pinnedBackH(screenW)" in theme and "pinnedBackH(w)" in theme,
}
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "CYD32 Back strip fix incomplete"
print(f"CYD32 rotated Back: {len(checks)} checks PASS")
