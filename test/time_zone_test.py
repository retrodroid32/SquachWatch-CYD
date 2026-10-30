#!/usr/bin/env python3
"""Regression checks for src/clock.cpp's persisted zones and geographic order."""
from pathlib import Path
import re

src = (Path(__file__).resolve().parents[1] / "src" / "clock.cpp").read_text(encoding="utf-8")
zone_match = re.search(r"static const Zone ZONES\[\]\s*=\s*\{(.*?)\n\};", src, re.S)
order_match = re.search(r"static const uint8_t ORDER\[\]\s*=\s*\{(.*?)\n\};", src, re.S)
assert zone_match and order_match, "zone table/order not found"
zones = re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', zone_match.group(1))
order = [int(n) for n in re.findall(r"\d+", re.sub(r"//[^\n]*", "", order_match.group(1)))]

# Persistent NVS and SQUAD zone bytes refer to numeric positions in this list.
# Never insert a new zone among these legacy 21 positions.
legacy = [
    ("US EASTERN", "EST5EDT,M3.2.0,M11.1.0"),
    ("US CENTRAL", "CST6CDT,M3.2.0,M11.1.0"),
    ("US MOUNTAIN", "MST7MDT,M3.2.0,M11.1.0"),
    ("ARIZONA", "MST7"),
    ("US PACIFIC", "PST8PDT,M3.2.0,M11.1.0"),
    ("ALASKA", "AKST9AKDT,M3.2.0,M11.1.0"),
    ("HAWAII", "HST10"),
    ("ATLANTIC", "AST4ADT,M3.2.0,M11.1.0"),
    ("NEWFOUNDLAND", "NST3:30NDT,M3.2.0,M11.1.0"),
    ("BRAZIL", "<-03>3"),
    ("UTC", "UTC0"),
    ("UK", "GMT0BST,M3.5.0/1,M10.5.0"),
    ("EU CENTRAL", "CET-1CEST,M3.5.0,M10.5.0/3"),
    ("EU EASTERN", "EET-2EEST,M3.5.0/3,M10.5.0/4"),
    ("MOSCOW", "MSK-3"),
    ("INDIA", "IST-5:30"),
    ("CHINA", "CST-8"),
    ("JAPAN", "JST-9"),
    ("AUS EASTERN", "AEST-10AEDT,M10.1.0,M4.1.0/3"),
    ("AUS WESTERN", "AWST-8"),
    ("NEW ZEALAND", "NZST-12NZDT,M9.5.0,M4.1.0/3"),
]
assert zones[:len(legacy)] == legacy, "a pre-existing saved time-zone index/rule changed"
assert len(zones) == 39, f"expected 21 legacy + 18 added zones, got {len(zones)}"
assert len(set(name for name, _ in zones)) == len(zones), "duplicate zone names"
assert len(order) == len(zones), "ORDER size differs from ZONES"
assert sorted(order) == list(range(len(zones))), "ORDER must contain every index exactly once"

# POSIX TZ rules specify the standard offset WEST of UTC; reverse its sign.
# Checking standard offsets keeps the navigation monotonic even though DST
# temporarily changes the effective order for some neighboring zones.
def east_minutes(rule):
    m = re.match(r"(?:<[^>]+>|[A-Za-z]+)([+-]?\d+(?::\d+)?)", rule)
    assert m, f"cannot parse standard POSIX offset in {rule!r}"
    value = m.group(1)
    sign = -1 if value.startswith("-") else 1
    hours, *minutes = value.lstrip("+-").split(":")
    return -sign * (int(hours) * 60 + (int(minutes[0]) if minutes else 0))

offsets = [east_minutes(zones[i][1]) for i in order]
assert offsets == sorted(offsets), "time-zone step order must move west to east"
assert zones[order[0]][0] == "HAWAII", "westmost zone should start the list"
assert zones[order[-1]][0] == "NEW ZEALAND", "eastmost zone should end the list"

# The source's next/previous indexing must always invert and wrap.
for pos in range(len(order)):
    forward = order[(pos + 1) % len(order)]
    backward = order[(pos - 1) % len(order)]
    assert order[(order.index(forward) - 1) % len(order)] == order[pos]
    assert order[(order.index(backward) + 1) % len(order)] == order[pos]

# Verify the real browser flasher's IANA-zone auto-detection rules, including
# Indiana's two Central Time zones (the general Indiana rule is Eastern).
flasher = (Path(__file__).resolve().parents[1] / "web-flasher" / "index.html").read_text(encoding="utf-8")
assert "var rules = [" in flasher, "browser TZ detection rules not found"
js_rules = flasher.split("var rules = [", 1)[1].split("];", 1)[0]
pairs = re.findall(r"^\s*\[/(.+)/,\s*'([^']+)'\],?$", js_rules, re.M)
assert len(pairs) >= 35, "expected expanded browser time-zone mappings"
def browser_zone(iana):
    return next((name for regex, name in pairs if re.search(regex, iana)), "UTC")

examples = {
    "America/Indiana/Indianapolis": "US EASTERN",
    "America/New_York": "US EASTERN",
    "America/Indiana/Knox": "US CENTRAL",
    "America/Indiana/Tell_City": "US CENTRAL",
    "America/Mexico_City": "MEXICO",
    "America/Santiago": "CHILE",
    "Africa/Johannesburg": "S AFRICA",
    "Asia/Jerusalem": "PALESTINE",
    "Asia/Kolkata": "INDIA",
    "Asia/Seoul": "KOREA",
    "Australia/Darwin": "DARWIN",
    "Australia/Brisbane": "QUEENSLAND",
    "Australia/Sydney": "AUS EASTERN",
    "Pacific/Auckland": "NEW ZEALAND",
}
for iana, want in examples.items():
    got = browser_zone(iana)
    assert got == want, f"{iana}: expected {want}, got {got}"

print("PASS: 39 zones, legacy IDs, west-to-east navigation, 14 browser IANA mappings")
