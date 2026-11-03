#pragma once
// LAB-only, physical UART GNSS diagnostics for PR #169. Never writes to SD.
#include <stdint.h>
namespace Lab169 {
struct Status {
    uint32_t baud, bytes, rateBytes, good, bad, epoch, csvCount;
    uint32_t dollar, newlines, printable, binary, ubx, gga, rmc, rxPeak;
    int32_t lat7, lon7;
    uint8_t sats, view, heard;
    bool valid, fresh, csvPass;
    char prefix[22];  // last real NMEA header, or "--" if none
};
void begin();
void tick(uint32_t now);
Status status(uint32_t now);
}
