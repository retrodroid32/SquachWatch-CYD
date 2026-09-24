// SquachWatch-CYD — the public half of the firmware signing key.
//
// Over-the-air updates are only installed when their signature verifies
// against this key (see ota_core.h). The private half never lives in this
// repository: it is the OTA_SIGNING_KEY secret the release workflow signs with,
// plus the owner's offline backup.
//
// A public key is safe to publish. What matters is that this one and that
// secret are a PAIR: sign with anything else and every board refuses the image.
//
// Fork-specific key generated 2026-09-24 (ECDSA P-256) for
// retrodroid32/SquachWatch-CYD. Fork secrets do not copy from upstream, so
// keeping the upstream public key here would make this fork unable to sign
// its own OTA releases. Boards running an older build must take one USB/web
// flash containing this key before they can accept OTA from this fork.
//
// It is kept here as the RAW CURVE POINT rather than as the PEM text it came
// from. Handing mbedtls a PEM means handing it the general-purpose key parser,
// which drags in base64, ASN.1 key structures and RSA -- 15 KB of flash, for
// one fixed key on one curve that this firmware has known since it was built.
// The point below is exactly what that parser would have produced.
//
// The PEM it was taken from, for anyone checking it against the private half:
//
//     -----BEGIN PUBLIC KEY-----
//     MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEgl2wHKh8UW0b5BqDahRRprLlwNdJ
//     pcE2dVQKHi6uNK5VrBNJMWXHA/Zp/Hfk2MzngPuYkM775G6SNVJ6ZvLdCA==
//     -----END PUBLIC KEY-----
//
// To regenerate the array after a key change (the last 65 bytes of the DER
// are the point; 0x04 marks it uncompressed, then X and Y, 32 bytes each):
//
//     openssl ec -pubin -in key.pub.pem -outform DER | tail -c 65 | xxd -i
#pragma once

#define OTA_PUBKEY_IS_TEST 0

// Uncompressed P-256 point: 0x04 || X (32) || Y (32).
static const unsigned char OTA_PUBKEY_POINT[65] = {
    0x04, 0x82, 0x5D, 0xB0, 0x1C, 0xA8, 0x7C, 0x51, 0x6D, 0x1B, 0xE4, 0x1A,
    0x83, 0x6A, 0x14, 0x51, 0xA6, 0xB2, 0xE5, 0xC0, 0xD7, 0x49, 0xA5, 0xC1,
    0x36, 0x75, 0x54, 0x0A, 0x1E, 0x2E, 0xAE, 0x34, 0xAE, 0x55, 0xAC, 0x13,
    0x49, 0x31, 0x65, 0xC7, 0x03, 0xF6, 0x69, 0xFC, 0x77, 0xE4, 0xD8, 0xCC,
    0xE7, 0x80, 0xFB, 0x98, 0x90, 0xCE, 0xFB, 0xE4, 0x6E, 0x92, 0x35, 0x52,
    0x7A, 0x66, 0xF2, 0xDD, 0x08
};
