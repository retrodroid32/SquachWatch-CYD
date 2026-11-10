#!/usr/bin/env python3
"""Verify the web-flasher LAB actually serves the partition table built for it.

A LAB build normally mirrors production Pages to avoid changing production
firmware. That becomes dangerous when the LAB changes the app partition table:
the deployed manifest must reference lab-esp32-partitions.bin, not the old
production esp32-partitions.bin. This checks the exact served bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from check_app_image_sizes import ota_app_slots, MIN_HEADROOM

ROOT = Path(__file__).resolve().parents[1]

def digest(data):
    return hashlib.sha256(data).hexdigest()

def verify(site: Path, build_dir: Path, margin: int, expect_size):
    matrix = json.loads((site / "boards.json").read_text())
    verified = 0
    errors = []
    for board in matrix["boards"]:
        if not board.get("lab"):
            continue
        env = board["id"]
        manifest_path = site / ("manifest-lab-" + env + ".json")
        if not manifest_path.is_file():
            errors.append(f"{env}: missing LAB manifest")
            continue
        manifest = json.loads(manifest_path.read_text())
        expected_firmware = "lab-" + env + "-firmware.bin"
        expected_partition = (
            "lab-twatch-s3-partitions.bin" if env == "twatch-s3"
            else "lab-esp32-partitions.bin"
        )
        for build in manifest.get("builds", []):
            parts = {part["offset"]: part["path"] for part in build["parts"]}
            fw_name = parts.get(0x10000)
            pt_name = parts.get(0x8000)
            if fw_name != expected_firmware or pt_name != expected_partition:
                errors.append(f"{env}: LAB manifest refers to {fw_name} / {pt_name}, "
                              f"expected {expected_firmware} / {expected_partition}")
                continue
            firmware = site / fw_name
            partition = site / pt_name
            if not firmware.is_file() or not partition.is_file():
                errors.append(f"{env}: missing LAB firmware or partition image")
                continue
            built_pt = build_dir / env / "partitions.bin"
            if not built_pt.is_file() or digest(partition.read_bytes()) != digest(built_pt.read_bytes()):
                errors.append(f"{env}: LAB site partition differs from compiled firmware layout")
                continue
            slots = ota_app_slots(partition.read_bytes())
            if len(slots) < 2:
                errors.append(f"{env}: expected dual OTA app partitions, got {slots}")
                continue
            capacity = min(s[2] for s in slots)
            remaining = capacity - firmware.stat().st_size
            if env != "twatch-s3" and expect_size is not None and capacity != expect_size:
                errors.append(f"{env}: published app slot is {capacity:#x}, expected {expect_size:#x}")
            if remaining < margin:
                errors.append(f"{env}: LAB firmware.bin is too large for its LAB partition "
                              f"({remaining:+,} B remaining)")
            print(f"{env}: web LAB image={firmware.stat().st_size:,} B "
                  f"slot={capacity:,} B free={remaining:,} B "
                  f"partition_sha256={digest(partition.read_bytes())[:12]}")
            verified += 1
    if errors:
        for error in errors:
            print("ERROR:", error, file=sys.stderr)
        raise SystemExit(1)
    if not verified:
        raise SystemExit("ERROR: no LAB boards verified")
    print(f"PASS: {verified} LAB manifests reference the exact built partition tables")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--site", type=Path, default=ROOT / "_site")
    p.add_argument("--build-dir", type=Path, default=ROOT / ".pio/build")
    p.add_argument("--min-headroom", type=int, default=MIN_HEADROOM)
    p.add_argument("--expected-classic-slot-size", type=lambda s: int(s,0), default=None)
    a = p.parse_args()
    verify(a.site, a.build_dir, a.min_headroom, a.expected_classic_slot_size)
