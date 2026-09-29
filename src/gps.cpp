// SquachWatch-CYD — optional external UART GPS/GNSS support
#include "gps.h"
#include "clock.h"
#include <Arduino.h>
#include <stdio.h>

#if defined(GPS_SUPPORT)
#include <TinyGPS++.h>

#ifndef GPS_RX_PIN
#define GPS_RX_PIN 35
#endif
#ifndef GPS_TX_PIN
#define GPS_TX_PIN 22
#endif
#ifndef GPS_BAUD
#define GPS_BAUD 115200
#endif

namespace Gps {

static TinyGPSPlus s_gps;
static Snapshot    s_snap{};
static uint32_t    s_fixAt = 0;
static uint32_t    s_timeAt = 0;
static uint32_t    s_lastCharAt = 0;
static uint32_t    s_lastNmeaAt = 0;
static uint32_t    s_lastUbxAt = 0;
static uint32_t    s_altAt = 0;
static uint32_t    s_hdopAt = 0;
static uint32_t    s_satAt = 0;
static bool        s_started = false;
static bool        s_hadFix = false;

// Days since 1970-01-01. Integer-only civil-date conversion keeps GPS UTC
// independent of the user's local TZ setting.
static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const int mp = (int)m + (m > 2 ? -3 : 9);
    const unsigned doy = (unsigned)((153 * mp + 2) / 5) + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static uint16_t le16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static int32_t le32s(const uint8_t* p) {
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                     ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}
static uint32_t epochFromFields(int y, unsigned mo, unsigned d,
                                unsigned hh, unsigned mm, unsigned ss) {
    if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31 ||
        hh > 23 || mm > 59 || ss > 60) return 0;
    const int64_t days = daysFromCivil(y, mo, d);
    const int64_t sec = days * 86400LL + (int64_t)hh * 3600LL +
                        (int64_t)mm * 60LL + (int64_t)ss;
    return sec > 0 && sec <= 0xFFFFFFFFLL ? (uint32_t)sec : 0;
}

// Minimal UBX stream decoder for the messages FlightMesh enables on its
// GNSS modules: NAV-PVT and NAV-DOP. Keeping this here avoids reconfiguring
// a module back to NMEA just to move it between FlightMesh and SquachWatch.
struct UbxRx {
    enum State : uint8_t { SYNC1, SYNC2, CLS, ID, LEN1, LEN2, PAYLOAD, CKA, CKB };
    State state = SYNC1;
    uint8_t cls = 0, id = 0, ckA = 0, ckB = 0, gotA = 0;
    uint16_t len = 0, pos = 0;
    uint8_t payload[96] = {};

    void add(uint8_t b) { ckA = (uint8_t)(ckA + b); ckB = (uint8_t)(ckB + ckA); }
    void reset() { state = SYNC1; cls = id = ckA = ckB = gotA = 0; len = pos = 0; }

    bool feed(uint8_t b) {
        switch (state) {
            case SYNC1:
                if (b == 0xB5) state = SYNC2;
                break;
            case SYNC2:
                if (b == 0x62) { state = CLS; ckA = ckB = 0; }
                else state = (b == 0xB5) ? SYNC2 : SYNC1;
                break;
            case CLS:
                cls = b; add(b); state = ID; break;
            case ID:
                id = b; add(b); state = LEN1; break;
            case LEN1:
                len = b; add(b); state = LEN2; break;
            case LEN2:
                len |= (uint16_t)b << 8; add(b); pos = 0;
                state = len ? PAYLOAD : CKA; break;
            case PAYLOAD:
                if (pos < sizeof payload) payload[pos] = b;
                pos++; add(b);
                if (pos >= len) state = CKA;
                break;
            case CKA:
                gotA = b; state = CKB; break;
            case CKB: {
                const bool ok = gotA == ckA && b == ckB;
                state = SYNC1;
                return ok;
            }
        }
        return false;
    }
};
static UbxRx s_ubx;

static void acceptEpoch(uint32_t ep, uint32_t now) {
    if (!ep) return;
    s_snap.epoch = ep;
    s_timeAt = now;
    if (!Clock::trusted() && Clock::setEpoch(ep))
        Serial.printf("[gps] clock set from GNSS UTC: %lu\n", (unsigned long)ep);
}

static void handleUbx(uint32_t now) {
    s_lastUbxAt = now;

    if (s_ubx.cls != 0x01) return; // NAV
    if (s_ubx.id == 0x07 && s_ubx.len >= 92) { // NAV-PVT
        const uint8_t* p = s_ubx.payload;
        const uint8_t fixType = p[20];
        const uint8_t flags   = p[21];
        const bool fixOk = (flags & 0x01u) && fixType >= 2;

        s_snap.sats = p[23];
        s_satAt = now;

        if (fixOk) {
            s_snap.lon = (double)le32s(p + 24) / 10000000.0;
            s_snap.lat = (double)le32s(p + 28) / 10000000.0;
            s_snap.altM = (float)le32s(p + 36) / 1000.0f; // hMSL
            s_snap.altValid = true;
            s_altAt = now;
            s_fixAt = now;
        }

        // validDate + validTime. NAV-PVT remains useful for position even
        // when these bits are not set yet.
        if ((p[11] & 0x03u) == 0x03u) {
            acceptEpoch(epochFromFields((int)le16(p + 4), p[6], p[7],
                                        p[8], p[9], p[10]), now);
        }
    } else if (s_ubx.id == 0x04 && s_ubx.len >= 18) { // NAV-DOP
        s_snap.hdop100 = le16(s_ubx.payload + 12);
        s_snap.hdopValid = true;
        s_hdopAt = now;
    }
}

static uint32_t gpsEpoch() {
    if (!s_gps.date.isValid() || !s_gps.time.isValid()) return 0;
    if (s_gps.date.age() > 5000 || s_gps.time.age() > 5000) return 0;
    const int y = s_gps.date.year();
    const unsigned mo = s_gps.date.month(), d = s_gps.date.day();
    if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0;
    return epochFromFields(y, mo, d, s_gps.time.hour(), s_gps.time.minute(), s_gps.time.second());
}

void begin() {
    if (s_started) return;
    s_started = true;
    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.printf("[gps] UART2 %u baud; RX GPIO%d, TX GPIO%d\n",
                  (unsigned)GPS_BAUD, GPS_RX_PIN, GPS_TX_PIN);
}

void tick(uint32_t now) {
    if (!s_started) return;

    // Bound the parser work so a large UART backlog cannot own a render pass.
    uint16_t budget = 256;
    while (budget-- && Serial2.available() > 0) {
        const uint8_t b = (uint8_t)Serial2.read();
        s_lastCharAt = now;

        // FlightMesh-configured modules emit UBX NAV-PVT/NAV-DOP with NMEA
        // disabled. Parse UBX in parallel with TinyGPS++ so the same receiver
        // can move between projects without being reconfigured.
        if (s_ubx.feed(b)) handleUbx(now);

        const char c = (char)b;
        if (!s_gps.encode(c)) continue;
        s_lastNmeaAt = now;

        if (s_gps.location.isUpdated() && s_gps.location.isValid()) {
            s_snap.lat = s_gps.location.lat();
            s_snap.lon = s_gps.location.lng();
            s_fixAt = now;
        }
        if (s_gps.altitude.isValid()) {
            s_snap.altM = (float)s_gps.altitude.meters();
            s_snap.altValid = true;
            s_altAt = now;
        }
        if (s_gps.satellites.isValid()) {
            const uint32_t n = s_gps.satellites.value();
            s_snap.sats = (uint8_t)(n > 255 ? 255 : n);
            s_satAt = now;
        }
        if (s_gps.hdop.isValid()) {
            const uint32_t h = s_gps.hdop.value();
            s_snap.hdop100 = (uint16_t)(h > 65535 ? 65535 : h);
            s_snap.hdopValid = true;
            s_hdopAt = now;
        }

        acceptEpoch(gpsEpoch(), now);
    }

    const bool fixNow = s_fixAt && (uint32_t)(now - s_fixAt) <= 10000u;
    if (fixNow != s_hadFix) {
        s_hadFix = fixNow;
        if (fixNow)
            Serial.printf("[gps] fix %.7f,%.7f %u SV\n",
                          s_snap.lat, s_snap.lon, (unsigned)s_snap.sats);
        else
            Serial.println("[gps] fix lost");
    }
}

Snapshot snapshot() {
    Snapshot out = s_snap;
    const uint32_t now = millis();
    out.fix = s_fixAt && (uint32_t)(now - s_fixAt) <= 10000u;
    out.ageMs = s_fixAt ? (uint32_t)(now - s_fixAt) : 0xFFFFFFFFu;
    out.timeValid = s_timeAt && (uint32_t)(now - s_timeAt) <= 10000u && out.epoch != 0;
    // Quality fields may come from either NMEA or UBX. Timestamp them at
    // acceptance so a FlightMesh-style UBX-only receiver is first-class.
    out.altValid = out.fix && s_altAt && (uint32_t)(now - s_altAt) <= 10000u;
    out.hdopValid = out.fix && s_hdopAt && (uint32_t)(now - s_hdopAt) <= 10000u;
    if (!out.fix || !s_satAt || (uint32_t)(now - s_satAt) > 10000u)
        out.sats = 0;
    return out;
}

void formatStatus(char* out, size_t n) {
    if (!out || !n) return;
    const uint32_t now = millis();
    if (!s_lastCharAt || (uint32_t)(now - s_lastCharAt) > 5000u) {
        snprintf(out, n, "NO DATA  %u", (unsigned)GPS_BAUD);
        return;
    }
    const bool ubx  = s_lastUbxAt  && (uint32_t)(now - s_lastUbxAt)  <= 5000u;
    const bool nmea = s_lastNmeaAt && (uint32_t)(now - s_lastNmeaAt) <= 5000u;
    if (!ubx && !nmea) {
        snprintf(out, n, "UART DATA  NO GPS FRAME");
        return;
    }
    const Snapshot s = snapshot();
    const char* proto = ubx ? "UBX" : "NMEA";
    if (!s.fix) {
        snprintf(out, n, "%s OK  NO FIX", proto);
        return;
    }
    if (s.hdopValid) snprintf(out, n, "%s FIX %u SV H%.2f", proto, (unsigned)s.sats, (double)s.hdop100 / 100.0);
    else             snprintf(out, n, "%s FIX %u SV", proto, (unsigned)s.sats);
}

bool enabled() { return true; }

} // namespace Gps

#else

namespace Gps {
void begin() {}
void tick(uint32_t) {}
Snapshot snapshot() { Snapshot s{}; s.ageMs = 0xFFFFFFFFu; return s; }
void formatStatus(char* out, size_t n) { if (out && n) snprintf(out, n, "not in this build"); }
bool enabled() { return false; }
} // namespace Gps

#endif
