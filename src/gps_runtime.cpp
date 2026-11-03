// Production CYD GNSS UART receiver; promoted from hardware-validated LAB #171.
// Unlike LAB169 there is no fake Wi-Fi AP, WiGLE CSV or automatic SD export.
#if defined(GPS_SUPPORT) && defined(GPS_RX_PIN)
#include "gps_runtime.h"
#include "clock.h"
#include "theme.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

namespace GpsRuntime {
namespace {
bool s_started = false;
uint32_t s_bytes = 0, s_peak = 0;
uint32_t s_lastReport = 0, s_lastGoodPacketMs = 0;
uint32_t s_prevUbxOk = 0, s_prevNmeaOk = 0;
}

void begin() {
    if (s_started) return;
    // Run only after the SquachMesh crypto self-test: mbedTLS needs a
    // contiguous heap allocation during boot. GPS UART reserves 2 KB.
    Serial.printf("[gps] before UART: heap %lu free, %lu largest\n",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial2.end();
    const size_t actual = Serial2.setRxBufferSize(RX_BUFFER_BYTES);
    Serial2.begin(BAUD, SERIAL_8N1, GPS_RX_PIN, -1); // RX only; GPS TX -> GPIO35
    Gnss::reset();
    s_bytes = s_peak = s_lastReport = 0;
    s_lastGoodPacketMs = s_prevUbxOk = s_prevNmeaOk = 0;
    Theme::setGpsIconStatus(Theme::GpsIconStatus::ABSENT);
    s_started = true;
    Serial.printf("[gps] UART2 RX GPIO%d at %lu, buffer %u; heap %lu free, %lu largest\n",
                  GPS_RX_PIN, (unsigned long)BAUD, (unsigned)actual,
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void tick(uint32_t nowMs) {
    if (!s_started) return;
    // Match the UART throughput/timing validated on 2.8-inch and 3.2-inch CYDs.
    const uint32_t beginUs = micros();
    uint16_t budget = 2048;
    const int queued = Serial2.available();
    if (queued > 0 && (uint32_t)queued > s_peak) s_peak = (uint32_t)queued;
    while (budget-- && Serial2.available() > 0 &&
           (uint32_t)(micros() - beginUs) < 4000u) {
        const int ch = Serial2.read();
        if (ch < 0) break;
        ++s_bytes;
        Gnss::feed((char)ch, nowMs); // UBX + checksum-validated NMEA on the same UART
    }
    // Only recognize a connected receiver after checksum-valid UBX or NMEA
    // packets (not merely bytes on floating GPIO35). The icon disappears
    // after five seconds without valid packets; a fresh FIX turns green.
    // No UART/SD/heap changes and no separate GPS polling task.
    const uint32_t ubxOk = Gnss::ubxGood(), nmeaOk = Gnss::good();
    if (ubxOk != s_prevUbxOk || nmeaOk != s_prevNmeaOk) {
        s_lastGoodPacketMs = nowMs;
        s_prevUbxOk = ubxOk;
        s_prevNmeaOk = nmeaOk;
    }
    const bool gpsOnline = s_lastGoodPacketMs != 0 &&
                           (uint32_t)(nowMs - s_lastGoodPacketMs) < 5000u;
    Theme::setGpsIconStatus(!gpsOnline ? Theme::GpsIconStatus::ABSENT :
                            Gnss::fresh(nowMs) ? Theme::GpsIconStatus::FIX :
                            Theme::GpsIconStatus::SEARCHING);
    // PR136 follow-up: adopt real GPS UTC without a second parser or
    // allocating memory. Only a checksum-validated, CURRENT position/fix
    // may establish the clock. Do this once while time is untrusted.
    // fake() exists for bench tests and must never set the real clock.
    if (!Clock::trusted() && Gnss::fresh(nowMs) && !Gnss::faked()) {
        const uint32_t epoch = Gnss::fix().epoch;
        if (epoch && Clock::setEpoch(epoch))
            Serial.printf("[gps] trusted UTC set from live GNSS fix: %lu\n",
                          (unsigned long)epoch);
    }
    if (nowMs - s_lastReport >= 10000u) {
        s_lastReport = nowMs;
        const Gnss::Fix& f = Gnss::fix();
        Serial.printf("[gps] bytes=%lu UBX=%lu/%lu PVT=%lu SAT=%lu NMEA=%lu/%lu fix=%u fresh=%u\n",
                      (unsigned long)s_bytes, (unsigned long)Gnss::ubxGood(),
                      (unsigned long)Gnss::ubxBad(), (unsigned long)Gnss::ubxPvt(),
                      (unsigned long)Gnss::ubxSat(), (unsigned long)Gnss::good(),
                      (unsigned long)Gnss::bad(), (unsigned)f.valid,
                      (unsigned)Gnss::fresh(nowMs));
    }
}

Status status(uint32_t nowMs) {
    Status s;
    s.bytes = s_bytes;
    s.rxPeak = s_peak;
    s.nmeaGood = Gnss::good(); s.nmeaBad = Gnss::bad();
    s.ubxGood = Gnss::ubxGood(); s.ubxBad = Gnss::ubxBad();
    s.ubxPvt = Gnss::ubxPvt(); s.ubxSat = Gnss::ubxSat();
    s.fix = Gnss::fix(); s.sky = Gnss::sky();
    s.fresh = Gnss::fresh(nowMs);
    return s;
}
}
#endif
