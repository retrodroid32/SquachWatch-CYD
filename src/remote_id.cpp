// SquachWatch-CYD — ASTM F3411 Remote ID decoder. See remote_id.h.
#include "remote_id.h"
#include <string.h>

namespace RemoteId {

// ---- the wire format ----------------------------------------------------
// A Bluetooth Legacy advert is a run of AD structures, each one a length
// byte, a type byte, then length-1 bytes of data. Remote ID rides in the
// Service Data structure (type 0x16) under UUID 0xFFFA, and inside that
// sits a one-byte application code, a one-byte message counter, then
// exactly 25 bytes of message.
static const uint8_t AD_SERVICE_DATA_16 = 0x16;
static const uint16_t ODID_UUID         = 0xFFFA;
static const uint8_t ODID_APP_CODE      = 0x0D;
static const uint8_t MSG_SIZE           = 25;

// Message types live in the TOP nibble of byte 0. The bottom nibble is the
// protocol version, which we do not check: the field layouts this reads
// have been stable across every published version, and refusing to decode
// a drone because it advertised version 3 would be worse than decoding it.
static const uint8_t MSG_BASIC_ID = 0x0;
static const uint8_t MSG_LOCATION = 0x1;
static const uint8_t MSG_SYSTEM   = 0x4;

// Fixed-point scales, straight out of the reference implementation:
// coordinates are degrees x 10^7, and altitude is half-metres offset by
// 1000m so that below-sea-level values still fit an unsigned 16-bit field.
static const double LATLON_MULT = 10000000.0;
static const float  ALT_DIV     = 0.5f;
static const float  ALT_ADDER   = 1000.0f;

// Little-endian, because the standard's structs are packed and every
// device implementing it is little-endian. Read byte by byte rather than
// cast: the payload comes off the radio unaligned, and a 32-bit load at an
// odd address is a LoadStoreError exception on this chip rather than a
// slow read like it would be on a PC.
static int32_t rd32(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                     ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}
static uint16_t rd16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

const char* uaTypeName(uint8_t t) {
    switch (t) {
        case 1:  return "PLANE";
        case 2:  return "QUAD";
        case 3:  return "HELI";
        case 4:  return "GYRO";
        case 5:  return "VTOL";
        case 6:  return "ORNITH";
        case 7:  return "GLIDER";
        case 8:  return "KITE";
        case 9:  return "FREEBAL";
        case 10: return "CAPBAL";
        case 11: return "AIRSHIP";
        case 12: return "PARA";
        case 13: return "ROCKET";
        case 14: return "TETHER";
        case 15: return "GROUND";
        default: return "UNKNOWN";
    }
}

void reset(Info& out) { out = Info(); }

// Decodes one 25-byte message body into `out`.
static bool mergeMessage(const uint8_t* m, Info& out, uint32_t now) {
    const uint8_t type = (uint8_t)((m[0] >> 4) & 0x0F);

    if (type == MSG_BASIC_ID) {
        // Byte 1 splits IDType (high) from UAType (low); the serial is the
        // next 20 bytes, space- or NUL-padded rather than terminated.
        out.uaType = (uint8_t)(m[1] & 0x0F);
        memcpy(out.serial, m + 2, 20);
        out.serial[20] = '\0';
        for (int i = 19; i >= 0; i--) {
            if (out.serial[i] == ' ' || out.serial[i] == '\0') out.serial[i] = '\0';
            else break;
        }
        out.haveBasic = true;
        out.at = now;
        return true;
    }

    if (type == MSG_LOCATION) {
        const int32_t la = rd32(m + 5);
        const int32_t lo = rd32(m + 9);
        // 0,0 is the standard's "unknown", and it is also a real place, so
        // treating it as no-fix is the lesser wrong of the two.
        if (la == 0 && lo == 0) return true;
        out.lat  = (float)((double)la / LATLON_MULT);
        out.lon  = (float)((double)lo / LATLON_MULT);
        out.altM = (float)rd16(m + 15) * ALT_DIV - ALT_ADDER;
        out.haveLoc = true;
        out.at = now;
        return true;
    }

    if (type == MSG_SYSTEM) {
        // The interesting one. Bytes 2-9 are where the pilot is standing.
        const int32_t la = rd32(m + 2);
        const int32_t lo = rd32(m + 6);
        if (la == 0 && lo == 0) return true;
        out.opLat = (float)((double)la / LATLON_MULT);
        out.opLon = (float)((double)lo / LATLON_MULT);
        out.haveOperator = true;
        out.at = now;
        return true;
    }

    // Auth, Self-ID, Operator-ID and Message Pack all reach here. Pack
    // (0xF) is the one worth naming: it wraps several messages at once and
    // is what the WiFi Beacon form uses, but it does not fit in a 31-byte
    // legacy advert, so nothing that arrives on this path can be one.
    return false;
}

static bool bluetoothPayload(const uint8_t* payload, uint8_t len,
                             const uint8_t*& odid, uint8_t& odidLen) {
    odid = nullptr;
    odidLen = 0;
    if (!payload || len < 4) return false;

    uint8_t i = 0;
    while (i < len) {
        const uint8_t adLen = payload[i];
        if (adLen == 0) break;
        if ((uint16_t)i + 1u + adLen > (uint16_t)len) break;

        const uint8_t  adType = payload[i + 1];
        const uint8_t* adData = payload + i + 2;
        const uint8_t  adSize = (uint8_t)(adLen - 1);

        if (adType == AD_SERVICE_DATA_16 && adSize >= 4 + MSG_SIZE &&
            rd16(adData) == ODID_UUID && adData[2] == ODID_APP_CODE) {
            odid = adData + 4;          // skip UUID, app code, counter
            odidLen = (uint8_t)(adSize - 4);

            // A normal BLE advert carries one 25-byte message. Some
            // implementations use Message Pack framing instead; accept that
            // only when the declared geometry is internally consistent.
            if ((odid[0] >> 4) == 0x0F) {
                if (odidLen < 3) return false;
                const uint8_t singleSize = odid[1];
                const uint8_t count = odid[2];
                if (singleSize != MSG_SIZE || count == 0 || count > 9) return false;
                return odidLen >= (uint8_t)(3u + (uint16_t)count * MSG_SIZE);
            }
            return odidLen >= MSG_SIZE;
        }
        i = (uint8_t)(i + 1 + adLen);
    }
    return false;
}

bool isBluetoothLegacy(const uint8_t* payload, uint8_t len) {
    const uint8_t* odid = nullptr;
    uint8_t odidLen = 0;
    return bluetoothPayload(payload, len, odid, odidLen);
}

static bool validMessagePack(const uint8_t* pack, uint16_t len) {
    if (!pack || len < 3) return false;
    if ((pack[0] >> 4) != 0x0F || pack[1] != MSG_SIZE) return false;
    const uint8_t count = pack[2];
    if (count == 0 || count > 9) return false;
    return len >= (uint16_t)(3u + (uint16_t)count * MSG_SIZE);
}

bool isWifiBeacon(const uint8_t* frame, uint16_t len) {
    if (!frame || len < 36) return false;

    uint16_t i = 36;
    while (i + 2u <= len) {
        const uint8_t id = frame[i];
        const uint8_t ieLen = frame[i + 1];
        const uint16_t next = (uint16_t)(i + 2u + ieLen);
        if (next > len) return false;

        if (id == 0xDD && ieLen >= 8) {
            const uint8_t* d = frame + i + 2;
            const bool asdStan = d[0] == 0xFA && d[1] == 0x0B && d[2] == 0xBC;
            const bool parrot  = d[0] == 0x90 && d[1] == 0x3A && d[2] == 0xE6;
            if ((asdStan || parrot) && (!asdStan || d[3] == 0x0D)) {
                // Both forms place the message pack after OUI/type + one
                // service counter byte. Requiring a structurally valid pack
                // keeps unrelated Parrot vendor IEs from becoming DRONE hits.
                const uint8_t* pack = d + 5;
                const uint16_t packLen = (uint16_t)(ieLen - 5);
                if (validMessagePack(pack, packLen)) return true;
            }
        }
        i = next;
    }
    return false;
}

bool isWifiNanAction(const uint8_t* frame, uint16_t len) {
    // Public Action / vendor-specific NAN frame. Sky-Spy and the OpenDroneID
    // reference implementation both key on the NAN destination plus the
    // "org.opendroneid.remoteid" service hash.
    if (!frame || len < 36) return false;
    if ((frame[0] & 0xFC) != 0xD0) return false;  // management Action frame

    static const uint8_t kDest[6] = {0x51,0x6F,0x9A,0x01,0x00,0x00};
    static const uint8_t kWifiAlliance[3] = {0x50,0x6F,0x9A};
    static const uint8_t kServiceId[6] = {0x88,0x69,0x19,0x9D,0x92,0x09};
    if (memcmp(frame + 4, kDest, sizeof(kDest)) != 0) return false;

    // 24-byte 802.11 header, then Public Action category/code, Wi-Fi Alliance
    // OUI, and NAN OUI type 0x13.
    const uint8_t* p = frame + 24;
    if (p[0] != 0x04 || p[1] != 0x09 ||
        memcmp(p + 2, kWifiAlliance, sizeof(kWifiAlliance)) != 0 ||
        p[5] != 0x13) return false;

    // Attribute layouts vary as optional NAN fields are present. The exact
    // six-byte service hash is the stable discriminator.
    for (uint16_t i = 30; i + sizeof(kServiceId) <= len; ++i) {
        if (memcmp(frame + i, kServiceId, sizeof(kServiceId)) == 0) return true;
    }
    return false;
}

bool merge(const uint8_t* payload, uint8_t len, Info& out, uint32_t now) {
    const uint8_t* odid = nullptr;
    uint8_t odidLen = 0;
    if (!bluetoothPayload(payload, len, odid, odidLen)) return false;

    if ((odid[0] >> 4) == 0x0F) {
        const uint8_t count = odid[2];
        for (uint8_t i = 0; i < count; ++i)
            mergeMessage(odid + 3u + (uint16_t)i * MSG_SIZE, out, now);
        // Transport recognition is still a successful Remote ID sighting even
        // if this pack only contains message types we do not display yet.
        return true;
    }

    mergeMessage(odid, out, now);
    return true;
}

}  // namespace RemoteId
