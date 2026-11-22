// SquachWatch-CYD — allocation-free BLE advertisement helpers.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace BleAdv {

// NimBLE/Mynewt exposes ble_addr_t::val in controller/native byte order.
// Convert it once at the scan boundary so every Detection/raw-scan/watch
// address uses the conventional human-readable order used by other tools.
static inline void canonicalMacFromNimble(const uint8_t raw[6], uint8_t out[6]) {
    if (!raw || !out) return;
    for (uint8_t i = 0; i < 6; ++i) out[i] = raw[5 - i];
}

// Indexed AD fields are used by the exact-signature detector as well as
// the name parser. This is bounded by payload length and never allocates;
// malformed/truncated fields fail closed instead of stepping off the buffer.
static inline bool nthField(const uint8_t* payload, size_t len, uint8_t type,
                            uint8_t wanted, const uint8_t*& data, size_t& dataLen) {
    if (!payload) return false;
    size_t pos = 0;
    uint8_t seen = 0;
    while (pos < len) {
        const uint8_t fieldLen = payload[pos];
        if (fieldLen == 0) { ++pos; continue; }
        const size_t next = pos + 1u + fieldLen;
        if (next > len) return false;
        if (payload[pos + 1] == type) {
            if (seen++ == wanted) {
                data = payload + pos + 2;
                dataLen = fieldLen - 1u;
                return true;
            }
        }
        pos = next;
    }
    return false;
}
static inline bool findField(const uint8_t* payload, size_t len, uint8_t type,
                             const uint8_t*& data, size_t& dataLen) {
    return nthField(payload, len, type, 0, data, dataLen);
}

// Copy the advertised local name without ever taking c_str() from a temporary
// std::string. Prefer Complete Local Name (0x09), then Shortened Local Name
// (0x08), matching NimBLE's user-visible behavior.
static inline void copyLocalName(const uint8_t* payload, size_t len,
                                 char* dst, size_t dstSize) {
    if (!dst || dstSize == 0) return;
    dst[0] = '\0';
    const uint8_t* data = nullptr;
    size_t n = 0;
    if (!findField(payload, len, 0x09, data, n) &&
        !findField(payload, len, 0x08, data, n)) return;
    if (n >= dstSize) n = dstSize - 1;
    if (n) memcpy(dst, data, n);
    dst[n] = '\0';
}

} // namespace BleAdv
