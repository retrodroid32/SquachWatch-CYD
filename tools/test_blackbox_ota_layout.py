#!/usr/bin/env python3
"""Ensure BlackBox's raw flash ring never overlaps the dual OTA app slots."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
rows = {}
for line in (root / "partitions_ota.csv").read_text().splitlines():
    if not line.strip() or line.lstrip().startswith("#"):
        continue
    parts = [p.strip() for p in line.split(",")]
    assert len(parts) >= 5, line
    rows[parts[0]] = (int(parts[3], 0), int(parts[4], 0))
source = (root / "src/blackbox.cpp").read_text()
assert re.search(r"const uint32_t BASE\s*=\s*0x3E4000;", source)
assert re.search(r"const uint8_t\s+SECTORS\s*=\s*12;", source)
base, length = 0x3E4000, 12 * 4096
assert rows["app0"] == (0x10000, 0x1F0000)
assert rows["app1"] == (0x200000, 0x1E4000)
assert rows["coredump"] == (0x3F0000, 0x10000)
assert rows["nvs"] == (0x9000, 0x5000)
assert rows["app0"][0] + rows["app0"][1] <= rows["app1"][0]
assert rows["app1"][0] + rows["app1"][1] == base
assert base + length == rows["coredump"][0]
for label, (start, size) in rows.items():
    assert not (start < base + length and start + size > base), label + " overlaps BlackBox"
assert min(rows["app0"][1], rows["app1"][1]) - 1_966_272 >= 8192, "baseline app too large"
print("PASS: Events BlackBox 48KB region does not overlap any flash partition")
