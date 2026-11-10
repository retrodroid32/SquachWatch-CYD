#!/usr/bin/env python3
"""Reproduce the actual 2026-11-10 firmware image overflow against OTA slots."""
import struct
import tempfile
from pathlib import Path
from check_app_image_sizes import check, ota_app_slots

APP_CAPACITY = 0x1E0000

def entry(name, typ, subtype, offset, size):
    label = name.encode("ascii").ljust(16, b"\x00")
    return struct.pack("<HBBII16sI", 0x50AA, typ, subtype, offset, size, label, 0)

part = (
    entry("nvs", 1, 2, 0x9000, 0x5000) +
    entry("otadata", 1, 0, 0xE000, 0x2000) +
    entry("app0", 0, 0x10, 0x10000, APP_CAPACITY) +
    entry("app1", 0, 0x11, 0x1F0000, APP_CAPACITY) +
    entry("coredump", 1, 3, 0x3F0000, 0x10000)
).ljust(0xC00, b"\xff")

assert [n for n, off, size in ota_app_slots(part)] == ["app0", "app1"]

with tempfile.TemporaryDirectory() as t:
    folder = Path(t) / "cyd"
    folder.mkdir()
    (folder / "partitions.bin").write_bytes(part)
    fw = folder / "firmware.bin"
    def set_size(size):
        with fw.open("wb") as f:
            f.seek(size - 1)
            f.write(b"\x00")
    def must_fail(size, margin):
        set_size(size)
        try:
            check(Path(t), "cyd", margin)
        except ValueError:
            return
        raise AssertionError(f"FAIL: allowed binary size {size} with margin {margin}")

    # The historically working binary is under the limit, but with no
    # growth room. With the new safe margin it must be rejected as fragile.
    set_size(1_965_936)
    check(Path(t), "cyd", 0)
    must_fail(1_965_936, 8192)
    # The NVS-only black-screen build exceeded the partition by 240 bytes.
    must_fail(1_966_320, 0)
    # A correctly slimmed firmware leaves enough space for future changes.
    set_size(1_940_000)
    check(Path(t), "cyd", 8192)

print("PASS: actual OTA image capacity overflow + headroom regression")
