"""Verify that an OTA private key matches include/ota_pubkey.h.

The private key is never printed or copied into the repository. This tool
extracts its public P-256 point with OpenSSL and compares it byte-for-byte with
the point compiled into the firmware.

Usage:
    python tools/verify_ota_key.py --key ota-private.pem \
        --header include/ota_pubkey.h --require-production
"""
import argparse
import hashlib
import re
import subprocess
import sys
from pathlib import Path


def compiled_point(header: Path) -> tuple[bytes, bool]:
    text = header.read_text(encoding="utf-8")
    m = re.search(
        r"OTA_PUBKEY_POINT\s*\[\s*65\s*\]\s*=\s*\{(.*?)\};",
        text,
        re.S,
    )
    if not m:
        raise ValueError("OTA_PUBKEY_POINT[65] was not found")
    point = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1)))
    if len(point) != 65 or point[0] != 0x04:
        raise ValueError("compiled OTA key is not an uncompressed 65-byte P-256 point")

    tm = re.search(r"#define\s+OTA_PUBKEY_IS_TEST\s+([01])", text)
    is_test = bool(tm and tm.group(1) == "1")
    return point, is_test


def private_public_der(key: Path) -> bytes:
    p = subprocess.run(
        ["openssl", "pkey", "-in", str(key), "-pubout", "-outform", "DER"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if p.returncode != 0:
        msg = p.stderr.decode(errors="replace").strip()
        raise ValueError("OpenSSL could not read the OTA private key" + (": " + msg if msg else ""))
    if not p.stdout:
        raise ValueError("OpenSSL returned an empty public key")
    return p.stdout


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--key", required=True, type=Path)
    ap.add_argument("--header", default=Path("include/ota_pubkey.h"), type=Path)
    ap.add_argument("--require-production", action="store_true")
    args = ap.parse_args()

    try:
        point, is_test = compiled_point(args.header)
        if args.require_production and is_test:
            raise ValueError("OTA_PUBKEY_IS_TEST is set; refusing a production release")
        der = private_public_der(args.key)
        if not der.endswith(point):
            raise ValueError("OTA_SIGNING_KEY does not match the public key compiled into ota_pubkey.h")
    except (OSError, ValueError) as exc:
        print("OTA key verification failed: %s" % exc, file=sys.stderr)
        raise SystemExit(1)

    fp = hashlib.sha256(point).hexdigest()[:16]
    print("OTA signing key matches compiled public key (public-point sha256 %s)" % fp)


if __name__ == "__main__":
    main()
