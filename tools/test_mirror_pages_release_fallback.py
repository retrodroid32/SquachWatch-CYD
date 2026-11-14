#!/usr/bin/env python3
"""Offline regression tests for safe, narrow LAB Pages signature recovery."""
import tempfile
import unittest
import urllib.error
from pathlib import Path
from unittest import mock

from mirror_pages import PagesMirror


class MirrorRecoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.mirror = PagesMirror("https://example.invalid/SquachWatch-CYD/",
                                  self.root, "retrodroid32/SquachWatch-CYD")
        self.mirror.historical_versions = {"v1.21.0"}

    @staticmethod
    def error(code):
        return urllib.error.HTTPError("https://example.invalid", code, "test",
                                     hdrs=None, fp=None)

    def test_transient_pages_503_recovers_exact_signed_release_asset(self):
        calls = []
        def release(args, **kwargs):
            calls.append(args)
            self.assertEqual(args[:4], ["gh", "release", "download", "v1.21.0"])
            self.assertEqual(args[args.index("--pattern") + 1],
                             "cyd32-st7798-firmware.sig")
            self.assertEqual(args[args.index("--repo") + 1],
                             "retrodroid32/SquachWatch-CYD")
            Path(args[args.index("--dir") + 1],
                 "cyd32-st7798-firmware.sig").write_bytes(b"S" * 70)

        with mock.patch("mirror_pages.urllib.request.urlopen",
                        side_effect=self.error(503)) as urlopen, \
             mock.patch("mirror_pages.time.sleep"), \
             mock.patch("mirror_pages.subprocess.run", side_effect=release):
            data = self.mirror.fetch("v1.21.0/cyd32-st7798-firmware.sig",
                                     optional=True)

        self.assertEqual(data, b"S" * 70)
        self.assertEqual(urlopen.call_count, 5)
        self.assertEqual(len(calls), 1)
        self.assertEqual((self.root / "v1.21.0/cyd32-st7798-firmware.sig").read_bytes(), data)

    def test_required_file_never_falls_back_to_release(self):
        with mock.patch("mirror_pages.urllib.request.urlopen",
                        side_effect=self.error(503)), \
             mock.patch("mirror_pages.time.sleep"), \
             mock.patch("mirror_pages.subprocess.run") as gh:
            with self.assertRaisesRegex(RuntimeError, "could not mirror"):
                self.mirror.fetch("boards.json")
        gh.assert_not_called()

    def test_optional_404_is_still_optional(self):
        with mock.patch("mirror_pages.urllib.request.urlopen",
                        side_effect=self.error(404)), \
             mock.patch("mirror_pages.subprocess.run") as gh:
            self.assertIsNone(self.mirror.fetch("v1.21.0/unused-firmware.sig",
                                                 optional=True))
        gh.assert_not_called()

    def test_reject_unknown_release_or_other_asset(self):
        for rel in ("v9.9.9/cyd32-st7798-firmware.sig",
                    "v1.21.0/cyd32-st7798-firmware.bin",
                    "v1.21.0/../boards.json"):
            with self.subTest(rel=rel):
                with self.assertRaises((RuntimeError, ValueError)):
                    self.mirror.restore_historical_signature(rel)

    def test_github_release_asset_missing_fails_closed(self):
        with mock.patch("mirror_pages.urllib.request.urlopen",
                        side_effect=self.error(503)), \
             mock.patch("mirror_pages.time.sleep"), \
             mock.patch("mirror_pages.subprocess.run", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "asset missing"):
                self.mirror.fetch("v1.21.0/cyd32-st7798-firmware.sig",
                                  optional=True)

    def test_invalid_signature_size_fails_closed(self):
        def small_asset(args, **kwargs):
            Path(args[args.index("--dir")+1],
                 "cyd32-st7798-firmware.sig").write_bytes(b"tiny")
        with mock.patch("mirror_pages.urllib.request.urlopen",
                        side_effect=self.error(503)), \
             mock.patch("mirror_pages.time.sleep"), \
             mock.patch("mirror_pages.subprocess.run", side_effect=small_asset):
            with self.assertRaisesRegex(RuntimeError, "signature length"):
                self.mirror.fetch("v1.21.0/cyd32-st7798-firmware.sig",
                                  optional=True)


if __name__ == "__main__":
    unittest.main()
