#!/usr/bin/env python3
"""LAB web installer must never mix a LAB app with an old production table."""
import json
from pathlib import Path
import struct
import tempfile
from verify_lab_site_partitions import verify

def entry(name, typ, sub, start, size):
    return struct.pack("<HBBII16sI", 0x50AA, typ, sub, start, size,
                       name.encode().ljust(16,b"\x00"), 0)

slot = 0x1F0000
table = (entry("nvs",1,2,0x9000,0x5000) +
         entry("otadata",1,0,0xE000,0x2000) +
         entry("app0",0,0x10,0x10000,slot) +
         entry("app1",0,0x11,0x200000,slot) +
         entry("coredump",1,3,0x3F0000,0x10000)).ljust(3072,b"\xff")
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    site = root / "_site"
    site.mkdir()
    build = root / ".pio" / "build" / "cyd"
    build.mkdir(parents=True)
    (site / "boards.json").write_text(json.dumps({"boards":[{"id":"cyd","lab":True}]}))
    (site / "lab-esp32-partitions.bin").write_bytes(table)
    (build / "partitions.bin").write_bytes(table)
    with (site / "lab-cyd-firmware.bin").open("wb") as f:
        f.seek(1_970_000-1)
        f.write(b"\x00")
    m={"builds":[{"parts":[
        {"offset":0x8000,"path":"lab-esp32-partitions.bin"},
        {"offset":0x10000,"path":"lab-cyd-firmware.bin"}]}]}
    mp = site / "manifest-lab-cyd.json"
    mp.write_text(json.dumps(m))
    verify(site, root / ".pio" / "build", 8192, slot)
    m["builds"][0]["parts"][0]["path"]="esp32-partitions.bin"
    mp.write_text(json.dumps(m))
    try:
        verify(site, root / ".pio" / "build", 8192, slot)
    except SystemExit:
        pass
    else:
        raise AssertionError("must reject a LAB manifest using production partition table")
print("PASS: LAB manifest partition migration cannot silently serve old layout")
