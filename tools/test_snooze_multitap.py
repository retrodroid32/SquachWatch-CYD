#!/usr/bin/env python3
"""Wiring/persistence guards for isolated v1.40 tap-to-step SNOOZE backport.

Hardware taps on physical 2.8 and 3.2 CYDs remain mandatory.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")
main = read("src/main.cpp")
ui = read("src/ui_alert.cpp")
hdr = read("include/ui_alert.h")
ign = read("src/ignore_list.cpp")
ignh = read("include/ignore_list.h")
checks = {
 "four-hour duration is distinct from persisted ALWAYS":
    "HOUR_4" in ignh and "4UL * 60UL * 60UL * 1000UL" in ign,
 "duration cycle order":
    all(main.index("SnoozeDuration::" + x) < main.index("SnoozeDuration::" + y)
        for x,y in [("MIN_10","MIN_30"),("MIN_30","HOUR_1"),("HOUR_1","HOUR_4")]),
 "3-second selection and wraparound":
    "SNOOZE_PICK_MS = 3000" in main and "s_snoozeStep % SNOOZE_STEPS + 1" in main,
 "edge-triggered taps prevent held-finger repetition":
    "if (touchJustDown && tp.valid &&" in main and "if (touchJustUp) s_snoozeHeld = false;" in main,
 "long-press preserves historical ALWAYS picker":
    "SNOOZE_HOLD_PICKER_MS = 900" in main and "uiAlertSetSnoozePrompt(true);" in main
    and "AlertSnoozeChoice::ALWAYS" in main and '"ALWAYS", false' in ui,
 "selection blocks alert auto dismiss":
    "break;  // never auto-dismiss while choosing a duration" in main,
 "screen communicates current pick and progress":
    "uiAlertSetSnoozePick(" in hdr and "s_snoozePickLabel" in ui
    and "s_snoozePickLeft" in ui and "TAP AGAIN" in ui and "LONGER" in ui,
 "ALWAYS and manager remain intact":
    "void uiAlertSetSnoozePrompt(bool enabled)" in ui and "SNOOZE_MAX" in ignh
    and "bool unsnooze(" in ignh and "KEY_SNZ" in ign,
}
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "snooze v1.40 integration incomplete"
print(f"Snooze integration: {len(checks)} checks PASS")
