#!/usr/bin/env python3
"""Guardrail tests for the real flash helper, without connecting to hardware."""
import sys
import unittest
from types import SimpleNamespace
from unittest import mock

import flash_known


class FlashKnownSafetyTests(unittest.TestCase):
    def execute(self, port, mac, *flags):
        with mock.patch.object(flash_known, "read_mac", return_value=mac), \
             mock.patch.object(sys, "argv", ["flash_known.py", port, *flags]), \
             mock.patch.object(flash_known.subprocess, "run",
                               return_value=SimpleNamespace(returncode=0)) as run:
            try:
                result = flash_known.main()
                exit_code = None
                exit_message = None
            except SystemExit as e:
                result = None
                exit_code = e.code
                exit_message = str(e)
            return result, exit_code, exit_message, run.call_args_list

    def test_two_known_cyd32c_macs_are_exact(self):
        ids = ("14:33:5c:6c:20:b8", "14:33:5c:6d:22:84")
        for mac in ids:
            self.assertEqual(flash_known.BOARDS[mac][0], "cyd32c")
            result, code, message, calls = self.execute("COM8", mac, "--check")
            self.assertIsNone(result)
            self.assertIsNone(code)
            self.assertIsNone(message)
            self.assertFalse(calls, "--check must never flash")

    def test_unverified_cyd32c_blocks_default_flashing(self):
        for mac in ("14:33:5c:6c:20:b8", "14:33:5c:6d:22:84"):
            _, code, message, calls = self.execute("COM8", mac)
            self.assertIsInstance(code, str)
            self.assertIn("--allow-experimental", message)
            self.assertFalse(calls)

    def test_cyd32c_explicit_opt_in_only_for_known_mac(self):
        _, code, _, calls = self.execute("COM8", "14:33:5c:6c:20:b8",
                                         "--allow-experimental")
        self.assertEqual(code, 0)
        self.assertEqual(len(calls), 1)
        args, kwargs = calls[0]
        self.assertIn("cyd32c", args[0])
        self.assertIn("--upload-port", args[0])
        self.assertIn("COM8", args[0])

    def test_classic_cyd_mapping_unaffected(self):
        _, code, message, calls = self.execute("COM8", "d4:8a:fc:c8:e5:e4")
        self.assertEqual(code, 0)
        self.assertIsNone(message)
        self.assertEqual(len(calls), 1)
        self.assertIn("cyd-fast", calls[0].args[0])

    def test_unknown_mac_rejected_even_with_opt_in(self):
        _, code, message, calls = self.execute("COM8", "de:ad:be:ef:12:34",
                                               "--allow-experimental")
        self.assertIsInstance(code, str)
        self.assertIn("unknown board", message)
        self.assertFalse(calls)

    def test_com13_is_restricted_to_known_watch(self):
        _, code, message, calls = self.execute("COM13", "14:33:5c:6c:20:b8")
        self.assertIsInstance(code, str)
        self.assertIn("refusing COM13", message)
        self.assertFalse(calls)

    def test_forbidden_port_refused(self):
        _, code, message, calls = self.execute("COM10", "14:33:5c:6c:20:b8",
                                               "--allow-experimental")
        self.assertIsInstance(code, str)
        self.assertIn("never flashed", message)
        self.assertFalse(calls)

    def test_esptool_mac_parser(self):
        with mock.patch.object(flash_known.subprocess, "run",
                               return_value=SimpleNamespace(
                                   stdout="Connected to ESP32\nMAC: 14:33:5C:6C:20:B8\n")) as run:
            self.assertEqual(flash_known.read_mac("COM8"), "14:33:5c:6c:20:b8")
            self.assertIn("read_mac", run.call_args.args[0])


if __name__ == "__main__":
    unittest.main()
