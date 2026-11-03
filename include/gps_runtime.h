// Live GNSS receiver for classic ESP32 CYD boards.
// Enabled only in CYD profiles with GPS_SUPPORT/GPS_RX_PIN.
// Uses the hardware-tested #169 UBX/NMEA parser; never writes to SD.
#pragma once
#if defined(GPS_SUPPORT) && defined(GPS_RX_PIN)
#include <stdint.h>
#include "gnss.h"

namespace GpsRuntime {
constexpr uint32_t BAUD = 115200u;
constexpr uint16_t RX_BUFFER_BYTES = 2048u;
struct Status {
    uint32_t bytes = 0;
    uint32_t rxPeak = 0;
    uint32_t nmeaGood = 0, nmeaBad = 0;
    uint32_t ubxGood = 0, ubxBad = 0, ubxPvt = 0, ubxSat = 0;
    Gnss::Fix fix;
    Gnss::Sky sky;
    bool fresh = false;
};
// Called after MeshTalk::begin(): preserve heap for AES/CCM self-test.
void begin();
// Drain UART2 with a 2048-byte / 4 ms cap so UI, SD and radio stay responsive.
void tick(uint32_t nowMs);
Status status(uint32_t nowMs);
}
#endif
