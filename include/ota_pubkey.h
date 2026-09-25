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
// The real key, generated 2026-09-25 (ECDSA P-256). Replacing it is not a
// quick change: every board in the field only trusts THIS key, so a new one
// reaches them only through a USB install -- or through an over-the-air
// update signed with the old key that carries the new one.
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
//     MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEbUG32oW4ur8vKgqj8pbdmIsbiZFR
//     +omipw2lIVU+5cEupj33zH99q9md6BRqthViqP7XKVWA8UzuTduJrPtK7w==
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
    0x04, 0x6D, 0x41, 0xB7, 0xDA, 0x85, 0xB8, 0xBA, 0xBF, 0x2F, 0x2A, 0x0A,
    0xA3, 0xF2, 0x96, 0xDD, 0x98, 0x8B, 0x1B, 0x89, 0x91, 0x51, 0xFA, 0x89,
    0xA2, 0xA7, 0x0D, 0xA5, 0x21, 0x55, 0x3E, 0xE5, 0xC1, 0x2E, 0xA6, 0x3D,
    0xF7, 0xCC, 0x7F, 0x7D, 0xAB, 0xD9, 0x9D, 0xE8, 0x14, 0x6A, 0xB6, 0x15,
    0x62, 0xA8, 0xFE, 0xD7, 0x29, 0x55, 0x80, 0xF1, 0x4C, 0xEE, 0x4D, 0xDB,
    0x89, 0xAC, 0xFB, 0x4A, 0xEF,
};
