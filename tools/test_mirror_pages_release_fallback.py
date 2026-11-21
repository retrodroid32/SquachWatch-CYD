#!/usr/bin/env python3
"""Historical Pages signature failures must fall back safely, never strip files."""
import io
import os
import tempfile
import urllib.error
from pathlib import Path
from unittest import mock

import mirror_pages


def http_error(code):
    return urllib.error.HTTPError(
        "https://example.test/SquachWatch-CYD/v1.20.1/awok-firmware.sig",
        code, "test", {}, io.BytesIO(b""),
    )


with tempfile.TemporaryDirectory() as d:
    mirror = mirror_pages.PagesMirror("https://example.test/SquachWatch-CYD/", Path(d))
    signature = b"example-signed-sidecar"
    calls = []

    def fake_download(cmd, **kwargs):
        calls.append(cmd)
        assert cmd[:4] == ["gh", "release", "download", "v1.20.1"]
        assert cmd[4:8] == ["--repo", "retrodroid32/SquachWatch-CYD",
                            "--pattern", "awok-firmware.sig"]
        Path(cmd[-1], "awok-firmware.sig").write_bytes(signature)
        return mock.Mock(returncode=0)

    with mock.patch.dict(os.environ, {"GITHUB_REPOSITORY": "retrodroid32/SquachWatch-CYD"}), \
         mock.patch.object(mirror_pages.urllib.request, "urlopen",
                           side_effect=http_error(503)), \
         mock.patch.object(mirror_pages.time, "sleep"), \
         mock.patch.object(mirror_pages.subprocess, "run",
                           side_effect=fake_download):
        result = mirror.fetch("v1.20.1/awok-firmware.sig", optional=True)
        assert result == signature
        assert (Path(d) / "v1.20.1/awok-firmware.sig").read_bytes() == signature
        assert len(calls) == 1

    # A missing optional sidecar stays optional, without any fallback.
    with mock.patch.object(mirror_pages.urllib.request, "urlopen",
                           side_effect=http_error(404)), \
         mock.patch.object(mirror_pages.subprocess, "run") as gh:
        assert mirror.fetch("v1.20.1/absent-firmware.sig", optional=True) is None
        gh.assert_not_called()

    # Required manifest, firmware and other Pages assets must remain fail-closed.
    with mock.patch.object(mirror_pages.urllib.request, "urlopen",
                           side_effect=http_error(503)), \
         mock.patch.object(mirror_pages.time, "sleep"), \
         mock.patch.object(mirror_pages.subprocess, "run") as gh:
        try:
            mirror.fetch("v1.20.1/manifest-cyd.json")
        except RuntimeError as exc:
            assert "after 5 attempts" in str(exc)
        else:
            raise AssertionError("required artifact silently bypassed")
        gh.assert_not_called()

    # Non-versioned optional signatures must never pick an unrelated release.
    with mock.patch.object(mirror_pages.urllib.request, "urlopen",
                           side_effect=http_error(503)), \
         mock.patch.object(mirror_pages.time, "sleep"), \
         mock.patch.object(mirror_pages.subprocess, "run") as gh:
        try:
            mirror.fetch("awok-firmware.sig", optional=True)
        except RuntimeError:
            pass
        else:
            raise AssertionError("unversioned 503 silently skipped")
        gh.assert_not_called()

    # If the official release is inaccessible too, leave deployment blocked.
    with mock.patch.dict(os.environ, {"GITHUB_REPOSITORY": "retrodroid32/SquachWatch-CYD"}), \
         mock.patch.object(mirror_pages.urllib.request, "urlopen",
                           side_effect=http_error(503)), \
         mock.patch.object(mirror_pages.time, "sleep"), \
         mock.patch.object(mirror_pages.subprocess, "run",
                           side_effect=FileNotFoundError("gh")):
        try:
            mirror.fetch("v1.21.0/awok-firmware.sig", optional=True)
        except RuntimeError:
            pass
        else:
            raise AssertionError("unrecoverable signature silently omitted")

print("PASS: old signature 503 release fallback; 404 optional; required and missing fail closed")
