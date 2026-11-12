#!/usr/bin/env python3
"""All six settings must change actual firmware drawing, including nonstandard keys."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def read(p): return (root/p).read_text(encoding="utf-8")
theme=read("src/theme.cpp")
ui=read("src/ui_settings.cpp")
desk=read("src/ui_desk.cpp")
diag=read("src/ui_diagnostics.cpp")
for key in ("src/ui_alert.cpp","src/ui_phone.cpp","src/ui_wifipass.cpp",
            "src/ui_sysprops.cpp","src/ui_clear.cpp"):
    src=read(key)
    assert "drawButtonLabel(t," in src, key
for symbol in ("void selectCategoryFont(", "void xpTileFontOn(", "xpTileTextH()", "fontDelta(Settings::FontKind::TEXT)"):
    assert symbol in theme, symbol
for token in ("Theme::xpTileFontOn(t)", "Theme::xpTileTextH()", "Theme::xpTileAscent()"):
    assert token in desk, token
assert "Theme::fontDelta(Settings::FontKind::TEXT)" in diag
assert "(6 * size)" in diag
assert "if (pending) return" not in ui and "PREVIEW ONLY" not in ui
assert "kind == Settings::FontKind::XP_TILES) return;" not in ui
assert "selectCategoryFont(t, Settings::FontKind::TEXT" in theme
assert 'fontDelta(Settings::FontKind::BUTTONS)' in ui
assert 'ButtonFont::barHeight(Settings::fontStep(Settings::FontKind::BUTTONS))' in theme
assert 'delta == -1' in theme and 'delta == 1' in theme
assert "tools/check_app_image_sizes.py" in read(".github/workflows/build.yml")
assert "0x1F0000" in read("partitions_ota.csv") and "0x1E4000" in read("partitions_ota.csv")
print("PASS: all six font categories live; button keys, alerts, XP desk, body diagnostics")
