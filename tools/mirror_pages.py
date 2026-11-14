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
import os
import posixpath
import re
import subprocess
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path


class PagesMirror:
    def __init__(self, base: str, dest: Path, release_repo: str = "") -> None:
        self.base = base.rstrip("/") + "/"
        self.dest = dest
        self.release_repo = release_repo
        self.historical_versions: set[str] = set()
        self.count = 0

    def restore_historical_signature(self, rel: str) -> bytes:
        """Recover one exact immutable release signature, never an arbitrary file.

        Called ONLY after Pages exhausted transient HTTP/network retries.
        Release assets must exist and have the expected detached-signature size;
        otherwise fail before any wholesale Pages deployment occurs.
        """
        components = rel.split("/")
        if (len(components) != 2 or
            components[0] not in self.historical_versions or
            not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", components[0]) or
            not re.fullmatch(r"[A-Za-z0-9-]+-firmware\.sig", components[1])):
            raise RuntimeError(f"unsafe historical signature fallback: {rel}")
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", self.release_repo):
            raise RuntimeError("GitHub release repository unavailable for safe Pages recovery")
        with tempfile.TemporaryDirectory(prefix="sqw-pages-restore-") as folder:
            args = ["gh", "release", "download", components[0],
                    "--pattern", components[1], "--dir", folder,
                    "--repo", self.release_repo]
            try:
                subprocess.run(args, check=True, capture_output=True, text=True, timeout=90)
            except (OSError, subprocess.CalledProcessError,
                    subprocess.TimeoutExpired) as exc:
                raise RuntimeError(f"GitHub release fallback failed for {rel}: {exc}") from exc
            path = Path(folder) / components[1]
            if not path.is_file():
                raise RuntimeError(f"GitHub release asset missing: {rel}")
            data = path.read_bytes()
        if not 64 <= len(data) <= 256:
            raise RuntimeError(f"invalid GitHub release signature length: {rel} ({len(data)} bytes)")
        print(f"::warning::Recovered preserved {rel} from matching immutable GitHub "
              f"Release {components[0]} because deployed Pages was unavailable", flush=True)
        return data

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
        # A lab publish mirrors the live production root byte-for-byte before
        # adding lab files. GitHub Pages can briefly return 503 while serving
        # an otherwise healthy manifest. Retry only transient server errors,
        # rate limits and network failures; a missing *required* asset remains
        # a hard failure, so we never deploy a partial production mirror.
        attempts = 5
        for attempt in range(1, attempts + 1):
            try:
                with urllib.request.urlopen(req, timeout=30) as response:
                    data = response.read()
                break
            except urllib.error.HTTPError as exc:
                if optional and exc.code == 404:
                    return None
                if exc.code != 429 and not (500 <= exc.code < 600):
                    raise RuntimeError(f"could not mirror {url}: HTTP {exc.code}") from exc
                reason = f"HTTP {exc.code}"
            except (urllib.error.URLError, TimeoutError) as exc:
                reason = str(getattr(exc, "reason", exc))
            if attempt == attempts:
                # Version-history detached signatures are also attached to
                # their exact immutable GitHub Releases. If Pages itself is
                # unhealthy, recover ONLY this narrowly-scoped historical
                # asset. Never skip it or fabricate bytes.
                if optional and rel.endswith("-firmware.sig") and "/" in rel:
                    data = self.restore_historical_signature(rel)
                    break
                raise RuntimeError(
                    f"could not mirror {url} after {attempts} attempts: {reason}"
                )
            delay = 2 ** attempt  # 2s, 4s, 8s, 16s between attempts
            print(
                f"::warning::Pages mirror transient failure ({reason}) fetching "
                f"{rel}; retry {attempt + 1}/{attempts} in {delay}s",
                flush=True,
            )
            time.sleep(delay)
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

    mirror = PagesMirror(args.base, args.dest,
                         release_repo=os.environ.get("GITHUB_REPOSITORY", ""))
    args.dest.mkdir(parents=True, exist_ok=True)

    boards_raw = mirror.fetch("boards.json")
    versions_raw = mirror.fetch("versions.json")
    assert boards_raw is not None and versions_raw is not None
    boards = json.loads(boards_raw)
    versions = json.loads(versions_raw)
    mirror.historical_versions = {
        mirror.clean(entry["dir"])
        for entry in versions.get("versions", [])
        if isinstance(entry, dict) and isinstance(entry.get("dir"), str)
        and entry["dir"]
    }

    # upload-pages-artifact adds/uses this marker and the deployed copy is
    # intentionally zero bytes. Recreate it locally instead of trying to
    # download an empty HTTP response through fetch(), which rejects empty
    # required assets by design.
    (args.dest / ".nojekyll").touch()

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
