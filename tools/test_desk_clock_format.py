#!/usr/bin/env python3
"""Static integration contract for v1.40-compatible 12/24-hour desk-clock UI.

This is intentionally not presented as a hardware/display acceptance test.
The actual application and simulator must still compile, and the setting must
be verified on physical 2.8- and 3.2-inch CYDs.
"""
from pathlib import Path

root = Path(__file__).resolve().parent.parent

def source(path):
    return (root / path).read_text(encoding="utf-8")

header = source("include/settings.h")
settings = source("src/settings.cpp")
rows = source("include/ui_settings.h")
ui = source("src/ui_settings.cpp")
main = source("src/main.cpp")
desk = source("src/ui_desk.cpp")
clock = source("src/clock.cpp")

checks = {
    "settings API": "bool        clock24Hour();" in header and "void        toggleClock24Hour();" in header,
    "existing settings default preserved": 's_prefs.getBool("clk24", false)' in settings,
    "persistent preference": 's_prefs.putBool("clk24", s_clock24Hour)' in settings,
    "new row registered": "CLOCK_FORMAT," in rows and "SettingsRow::CLOCK_FORMAT" in ui,
    "desk page contains format row": "SettingsRow::CLOCK_FORMAT, SettingsRow::TIME_ZONE" in ui,
    "desk label reflects setting": '"24 HOUR" : "12 HOUR"' in ui,
    "row tap toggles clock": "case SettingsRow::CLOCK_FORMAT: Settings::toggleClock24Hour();" in main,
    "actual clock formatting parameter": "const bool twelveHour = !Settings::clock24Hour();" in desk and
        "Clock::formatTime(tm, sizeof tm, twelveHour, &pm);" in desk,
    "clock parser supports both formats": 'twelveHour ? "%d:%02d" : "%02d:%02d"' in clock,
    "12-hour geometry vs 24-hour": "const int AMPM_W  = twelveHour ? 14 : 0;" in desk,
    "am-pm only in 12-hour": "if (twelveHour) {\n        const uint16_t ac" in desk,
}
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
assert all(checks.values()), "desk clock 12/24 contract failed"
print(f"Desk clock 12/24 integration: {len(checks)} checks PASS")
