#!/usr/bin/env python3
"""Mirror the currently deployed Pages flasher into a local directory.

Used only as the lab-build bootstrap when the repository has no GitHub Release
assets yet. The live site remains the source of truth in that case: root
manifests, every part they reference, the version-history directories, update
page, and browser emulator are copied byte-for-byte before lab assets are added.
"""
from __future__ import annotations

import argparse
import json
import posixpath
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


class PagesMirror:
    def __init__(self, base: str, dest: Path) -> None:
        self.base = base.rstrip("/") + "/"
        self.dest = dest
        self.count = 0

    @staticmethod
    def clean(rel: str) -> str:
        rel = rel.lstrip("/")
        norm = posixpath.normpath(rel)
        if not norm or norm == "." or norm == ".." or norm.startswith("../"):
            raise ValueError(f"unsafe Pages path: {rel!r}")
        return norm

    def fetch(self, rel: str, *, optional: bool = False) -> bytes | None:
        rel = self.clean(rel)
        url = urllib.parse.urljoin(self.base, rel)
        req = urllib.request.Request(url, headers={"User-Agent": "SquachWatch-CYD-release-workflow"})
        try:
            with urllib.request.urlopen(req, timeout=30) as response:
                data = response.read()
        except urllib.error.HTTPError as exc:
            if optional and exc.code == 404:
                return None
            raise RuntimeError(f"could not mirror {url}: HTTP {exc.code}") from exc
        except urllib.error.URLError as exc:
            raise RuntimeError(f"could not mirror {url}: {exc.reason}") from exc
        if not data:
            if optional:
                return None
            raise RuntimeError(f"live Pages asset is empty: {url}")
        target = self.dest / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        self.count += 1
        return data

    def manifest(self, rel: str, *, optional: bool = False) -> bool:
        raw = self.fetch(rel, optional=optional)
        if raw is None:
            return False
        try:
            manifest = json.loads(raw)
        except json.JSONDecodeError as exc:
            raise RuntimeError(f"invalid live manifest {rel}: {exc}") from exc
        parent = posixpath.dirname(self.clean(rel))
        for build in manifest.get("builds", []):
            for part in build.get("parts", []):
                path = part.get("path")
                if not isinstance(path, str) or not path:
                    raise RuntimeError(f"invalid part path in live manifest {rel}")
                self.fetch(posixpath.join(parent, path))
        return True


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", required=True, help="deployed Pages root URL")
    ap.add_argument("--dest", required=True, type=Path)
    args = ap.parse_args()

    mirror = PagesMirror(args.base, args.dest)
    args.dest.mkdir(parents=True, exist_ok=True)

    boards_raw = mirror.fetch("boards.json")
    versions_raw = mirror.fetch("versions.json")
    assert boards_raw is not None and versions_raw is not None
    boards = json.loads(boards_raw)
    versions = json.loads(versions_raw)

    for rel in (
        "index.html",
        "update/index.html",
        "emulator/index.html",
        "emulator/squachsim.js",
        "emulator/squachsim.wasm",
    ):
        mirror.fetch(rel)

    manifests: set[str] = set()
    signed_ids: set[str] = set()
    for board in boards.get("boards", []):
        if not board.get("release"):
            continue
        for key in ("manifest", "fast_manifest"):
            value = board.get(key)
            if isinstance(value, str) and value:
                manifests.add(value)
        if board.get("sign") and isinstance(board.get("id"), str):
            signed_ids.add(board["id"])

    if not manifests:
        raise RuntimeError("live boards.json contains no release manifests")

    for manifest in sorted(manifests):
        mirror.manifest(manifest)
    for board_id in sorted(signed_ids):
        mirror.fetch(f"{board_id}-firmware.sig", optional=True)

    for entry in versions.get("versions", []):
        prefix = entry.get("dir", "")
        if not isinstance(prefix, str) or not prefix:
            continue
        prefix = mirror.clean(prefix)
        for manifest in sorted(manifests):
            mirror.manifest(posixpath.join(prefix, manifest), optional=True)
        for board_id in sorted(signed_ids):
            mirror.fetch(posixpath.join(prefix, f"{board_id}-firmware.sig"), optional=True)

    print(f"mirrored {mirror.count} deployed Pages assets from {mirror.base}")


if __name__ == "__main__":
    main()
