#!/usr/bin/env python3
"""Fail CI/release if a *final firmware.bin* does not fit its real app partition.

PlatformIO's 'Flash: used x/y' line measures ELF sections, not the ESP32
bootable image length. The 2026-11-10 CYD black-screen regression passed this
estimate although firmware.bin was 240 bytes LARGER than each 0x1e0000 slot.

Read each environment's *built binary* partition table, not a guessed limit.
OTA builds must fit BOTH active and inactive app slots. Keep a small growth
buffer so the next harmless diagnostic line cannot brick a board.
"""
import argparse
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
PART_ENTRY_BYTES = 32
PART_MAGIC = bytes.fromhex("aa50")
MIN_HEADROOM = 8192

def ota_app_slots(data: bytes):
    slots = []
    for offset in range(0, len(data) - PART_ENTRY_BYTES + 1, PART_ENTRY_BYTES):
        row = data[offset:offset + PART_ENTRY_BYTES]
        if row[:2] != PART_MAGIC:
            if row == b"\xff" * PART_ENTRY_BYTES:
                break
            continue
        part_type, subtype, start, size = struct.unpack_from("<BBII", row, 2)
        name = row[12:28].split(b"\x00")[0].decode("ascii", "replace")
        if part_type == 0 and (subtype == 0 or 0x10 <= subtype <= 0x1f):
            slots.append((name, start, size))
    return slots

def check(build_dir: Path, env: str, min_headroom: int):
    firmware = build_dir / env / "firmware.bin"
    partitions = build_dir / env / "partitions.bin"
    if not firmware.is_file() or not partitions.is_file():
        raise ValueError(f"{env}: missing firmware.bin or partitions.bin")
    slots = ota_app_slots(partitions.read_bytes())
    if not slots:
        raise ValueError(f"{env}: no application partitions in the built table")
    size = firmware.stat().st_size
    max_size = min(s[2] for s in slots)
    remaining = max_size - size
    print(f"{env}: REAL firmware.bin {size:,} B / app slot {max_size:,} B "
          f"({remaining:+,} B free; need >= {min_headroom:,} B); "
          f"slots={','.join(x[0] for x in slots)}")
    if remaining < min_headroom:
        raise ValueError(f"{env}: unsafe firmware image: "
                         f"{size:,} B with {remaining:+,} B remaining; "
                         f"reduce binary size before packaging or publication")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("environments", nargs="+")
    parser.add_argument("--min-headroom", type=int, default=MIN_HEADROOM)
    parser.add_argument("--build-dir", type=Path, default=ROOT / ".pio/build")
    args = parser.parse_args()
    errors = []
    for env in args.environments:
        try:
            check(args.build_dir, env, args.min_headroom)
        except ValueError as exc:
            errors.append(str(exc))
    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        sys.exit(1)
    print(f"PASS: all {len(args.environments)} final firmware images fit their "
          f"actual app partitions with >= {args.min_headroom:,} B margin")

if __name__ == "__main__":
    main()
