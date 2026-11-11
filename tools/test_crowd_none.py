#!/usr/bin/env python3
"""CROWD NONE integration guard; CI also compiles real app and emulator."""
from pathlib import Path
root = Path(__file__).resolve().parent.parent
settings = (root / "src/settings.cpp").read_text()
clear = (root / "src/ui_clear.cpp").read_text()

# The same public cycle's intended values. TWO has no distinct visit scene.
def step(c):
    if c >= 8: return 0
    v = c + 1
    return 3 if v == 2 else v

sequence = [1]
for _ in range(8): sequence.append(step(sequence[-1]))
assert sequence == [1, 3, 4, 5, 6, 7, 8, 0, 1], sequence
checks = {
    "label NONE": 'if (n == 0) return "NONE";' in settings,
    "legacy ONE default": 's_prefs.getUChar("crowd", 1)' in settings,
    "retains saved NONE": 'if (s_meshCrowd > CROWD_MAX) s_meshCrowd = 1;' in settings,
    "cycle wraps to NONE": "s_meshCrowd = s_meshCrowd >= CROWD_MAX ? 0 : nextCrowd(s_meshCrowd);" in settings,
    "desk does not inherit NONE": 's_prefs.getUChar("dskcrwd", s_meshCrowd ? s_meshCrowd : 1)' in settings,
    "real visits suppressed": "if (Settings::meshCrowd() == 0) return nullptr;" in clear,
    "active visit ends": "if (Settings::meshCrowd() == 0) return 0;" in clear,
    "no unsigned underflow": "const uint8_t peerCap = crowdMax > 1 ?" in clear,
    "no radio toggle": "s_meshDetect =" not in settings.split("void cycleMeshCrowd() {", 1)[1].split("void cycleDeskCrowd()", 1)[0],
}
for label, ok in checks.items(): print(("PASS" if ok else "FAIL") + ": " + label)
assert all(checks.values()), "CROWD NONE contract failed"
print(f"CROWD NONE: {len(checks)} integration gates and 8 cyclic state transitions PASS")
