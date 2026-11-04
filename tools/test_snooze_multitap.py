#!/usr/bin/env python3
"""PR182 six-choice snooze wiring/persistence guard. Hardware testing is mandatory."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")
main, ui, hdr, ign, ignh = [read(p) for p in
 ("src/main.cpp", "src/ui_alert.cpp", "include/ui_alert.h",
  "src/ignore_list.cpp", "include/ignore_list.h")]
table = main.split("static const SnoozeStep SNOOZE_STEP[] = {",1)[1].split("};",1)[0]
order = ['"10 MIN"', '"30 MIN"', '"1 HOUR"', '"4 HOURS"', '"ALWAYS"', '"OFF"']
checks = {
 "six choices and order": all(table.count(x)==1 for x in order)
   and all(table.index(a)<table.index(b) for a,b in zip(order,order[1:])),
 "4h and persisted ALWAYS": "HOUR_4" in ignh
   and "4UL * 60UL * 60UL * 1000UL" in ign
   and "SnoozeDuration::ALWAYS" in table and "saveSnoozes()" in ign,
 "3-second wrap": "SNOOZE_PICK_MS = 3000" in main
   and "s_snoozeStep % SNOOZE_STEPS + 1" in main,
 "touch edge does not repeat held press": "if (touchJustDown && tp.valid &&" in main
   and "s_snoozeHeld" not in main and "s_snoozeHoldAt" not in main,
 "OFF is idempotent and persists removal": 
   "(!IgnoreList::snoozed(s_alertMac) || IgnoreList::unsnooze(s_alertMac))" in main
   and "pick.off" in main and "previous.always && !saveSnoozes()" in ign,
 "no old picker or long-press branch": all(k not in main+ui+hdr for k in
   ("SNOOZE_HOLD_PICKER_MS","uiAlertSnoozePrompt","uiAlertHitSnoozeChoice",
    "uiAlertSetSnoozePrompt","s_snoozePrompt")),
 "auto-dismiss blocked": "break;  // never auto-dismiss while choosing a duration" in main,
 "visual cue and progress": "uiAlertSetSnoozePick(" in hdr
   and "s_snoozePickLabel" in ui and "s_snoozePickLeft" in ui
   and "TAP AGAIN" in ui and "TO CHANGE" in ui,
 "manager retained": "SNOOZE_MAX" in ignh and "bool unsnooze(" in ignh
   and "KEY_SNZ" in ign,
}
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "Six-choice SNOOZE integration incomplete"
print(f"Snooze integration: {len(checks)} checks PASS")
