#!/usr/bin/env python3
"""Single source of truth for SquachWatch build/publish board profiles."""
# CI trigger/checkpoint: board matrix validation belongs in every normal build.
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "web-flasher" / "boards.json"

def load():
    data = json.loads(MATRIX.read_text(encoding="utf-8"))
    boards = data.get("boards", [])
    if not isinstance(boards, list) or not boards:
        raise SystemExit("boards.json has no board profiles")
    return boards

def selected(boards, flag):
    return [b for b in boards if b.get(flag) is True]

def validate(boards):
    errors = []
    ids = [b.get("id") for b in boards]
    if len(ids) != len(set(ids)):
        errors.append("duplicate board id in boards.json")
    pio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
    envs = set(re.findall(r"^\[env:([^\]]+)\]", pio, re.M))
    for b in boards:
        bid = b.get("id")
        if not bid or bid not in envs:
            errors.append(f"{bid!r}: missing [env:{bid}] in platformio.ini")
        manifest = b.get("manifest")
        if b.get("release") and not manifest:
            errors.append(f"{bid}: release profile has no manifest")
        if manifest and not (ROOT / "web-flasher" / manifest).is_file():
            errors.append(f"{bid}: missing web-flasher/{manifest}")
        if b.get("release") and not b.get("sign"):
            errors.append(f"{bid}: release firmware is not marked for signing")
        fast = b.get("fast_env")
        if fast and fast not in ids:
            errors.append(f"{bid}: fast_env {fast!r} is not a board profile")
        fast_manifest = b.get("fast_manifest")
        if fast_manifest and not (ROOT / "web-flasher" / fast_manifest).is_file():
            errors.append(f"{bid}: missing web-flasher/{fast_manifest}")
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        errors.append(f"VERSION is not semantic x.y.z: {version!r}")
    note = ROOT / ".github" / "release-notes" / f"v{version}.md"
    if not note.is_file():
        errors.append(f"missing release notes: {note.relative_to(ROOT)}")
    if "cyd32-st7798" not in [b["id"] for b in selected(boards, "ci")]:
        errors.append("cyd32-st7798 must be in normal CI")
    if errors:
        for e in errors:
            print("ERROR:", e, file=sys.stderr)
        return 1
    print(f"board matrix OK: {len(boards)} profiles; version {version}")
    return 0

def release_assets(boards):
    out = ["esp32-bootloader.bin", "esp32-partitions.bin", "esp32-otadata.bin"]
    for b in selected(boards, "release"):
        out.append(f"{b['id']}-firmware.bin")
    out += ["esp32s3-bootloader.bin", "twatch-s3-partitions.bin"]
    for b in selected(boards, "merged"):
        out.append(f"firmware-{b['id']}-merged.bin")
    for b in selected(boards, "release"):
        if b.get("manifest"):
            out.append(b["manifest"])
    return out

def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("list"); p.add_argument("flag")
    p = sub.add_parser("pio-args"); p.add_argument("flag")
    p = sub.add_parser("manifests"); p.add_argument("flag")
    sub.add_parser("release-assets")
    sub.add_parser("validate")
    args = ap.parse_args()
    boards = load()
    if args.cmd == "validate":
        raise SystemExit(validate(boards))
    if args.cmd == "release-assets":
        print(" ".join(release_assets(boards))); return
    rows = selected(boards, args.flag)
    if args.cmd == "list":
        print(" ".join(b["id"] for b in rows))
    elif args.cmd == "pio-args":
        print(" ".join(f"-e {b['id']}" for b in rows))
    elif args.cmd == "manifests":
        print(" ".join(b["manifest"] for b in rows if b.get("manifest")))

if __name__ == "__main__":
    main()
