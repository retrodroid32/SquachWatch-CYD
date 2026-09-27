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
#define GPS_TX_PIN 26
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

static uint32_t gpsEpoch() {
    if (!s_gps.date.isValid() || !s_gps.time.isValid()) return 0;
    if (s_gps.date.age() > 5000 || s_gps.time.age() > 5000) return 0;
    const int y = s_gps.date.year();
    const unsigned mo = s_gps.date.month(), d = s_gps.date.day();
    if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0;
    const int64_t days = daysFromCivil(y, mo, d);
    const int64_t sec = days * 86400LL +
                        (int64_t)s_gps.time.hour() * 3600LL +
                        (int64_t)s_gps.time.minute() * 60LL +
                        (int64_t)s_gps.time.second();
    return sec > 0 && sec <= 0xFFFFFFFFLL ? (uint32_t)sec : 0;
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
        const char c = (char)Serial2.read();
        s_lastCharAt = now;
        if (!s_gps.encode(c)) continue;

        if (s_gps.location.isUpdated() && s_gps.location.isValid()) {
            s_snap.lat = s_gps.location.lat();
            s_snap.lon = s_gps.location.lng();
            s_fixAt = now;
        }
        if (s_gps.altitude.isValid()) {
            s_snap.altM = (float)s_gps.altitude.meters();
            s_snap.altValid = true;
        }
        if (s_gps.satellites.isValid()) {
            const uint32_t n = s_gps.satellites.value();
            s_snap.sats = (uint8_t)(n > 255 ? 255 : n);
        }
        if (s_gps.hdop.isValid()) {
            const uint32_t h = s_gps.hdop.value();
            s_snap.hdop100 = (uint16_t)(h > 65535 ? 65535 : h);
            s_snap.hdopValid = true;
        }

        const uint32_t ep = gpsEpoch();
        if (ep) {
            s_snap.epoch = ep;
            s_timeAt = now;
            // A real GPS time replaces an unset/guessed clock, but does not
            // churn NVS every second after the clock is already trusted.
            if (!Clock::trusted()) {
                if (Clock::setEpoch(ep))
                    Serial.printf("[gps] clock set from GNSS UTC: %lu\n", (unsigned long)ep);
            }
        }
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
    return out;
}

void formatStatus(char* out, size_t n) {
    if (!out || !n) return;
    const uint32_t now = millis();
    if (!s_lastCharAt || (uint32_t)(now - s_lastCharAt) > 5000u) {
        snprintf(out, n, "NO DATA  115200");
        return;
    }
    const Snapshot s = snapshot();
    if (!s.fix) {
        snprintf(out, n, "NMEA  NO FIX");
        return;
    }
    if (s.hdopValid) snprintf(out, n, "FIX %u SV  HDOP %.2f", (unsigned)s.sats, (double)s.hdop100 / 100.0);
    else             snprintf(out, n, "FIX %u SV", (unsigned)s.sats);
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
