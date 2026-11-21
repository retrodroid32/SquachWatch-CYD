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

    def _release_signature_fallback(self, rel: str) -> bytes | None:
        """Recover a historical optional signature from its official release.

        Pages intermittently 503s on old .sig files, despite the official
        versioned GitHub Release still carrying that exact signed sidecar.
        Only accept well-formed vX.Y.Z/BOARD-firmware.sig names in THIS repo.
        Never substitute other firmware, manifests, or required Pages files.
        Missing/failed fallback is a hard failure in fetch(), not a silently
        incomplete production mirror.
        """
        parts = rel.split("/")
        if len(parts) != 2:
            return None
        tag, filename = parts
        if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag):
            return None
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*-firmware\.sig", filename):
            return None
        repo = os.environ.get("GITHUB_REPOSITORY", "")
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo):
            return None

        with tempfile.TemporaryDirectory(prefix="sqw-release-sig-") as tmp:
            cmd = [
                "gh", "release", "download", tag,
                "--repo", repo, "--pattern", filename, "--dir", tmp,
            ]
            try:
                subprocess.run(cmd, check=True, capture_output=True,
                               text=True, timeout=60)
                data = (Path(tmp) / filename).read_bytes()
            except (OSError, subprocess.CalledProcessError,
                    subprocess.TimeoutExpired) as exc:
                print(f"::warning::Historical signature fallback failed for "
                      f"{rel}: {type(exc).__name__}", flush=True)
                return None
        # Signatures are tiny (about 70 bytes). Reject anything implausible,
        # such as an HTML error page or a accidentally downloaded firmware.
        if not data or len(data) > 4096:
            raise RuntimeError(f"invalid historical release signature {rel}: {len(data)} bytes")
        print(f"::notice::Recovered historical {rel} from GitHub Release "
              f"{tag} after Pages failures; production history retained", flush=True)
        return data

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
                if optional:
                    fallback = self._release_signature_fallback(rel)
                    if fallback is not None:
                        data = fallback
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

    mirror = PagesMirror(args.base, args.dest)
    args.dest.mkdir(parents=True, exist_ok=True)

    boards_raw = mirror.fetch("boards.json")
    versions_raw = mirror.fetch("versions.json")
    assert boards_raw is not None and versions_raw is not None
    boards = json.loads(boards_raw)
    versions = json.loads(versions_raw)

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
