// SquachWatch-CYD — bounded, allocation-free HUNT session history.
// This is a *display-only* record: it never affects live RSSI, confidence,
// the CAUGHT state, BLE/Wi-Fi scanning or the HUNT radio-loss timeout.
#pragma once
#include <stdint.h>

namespace HuntHistory {
static constexpr uint8_t CAP = 40;        // ~40 seconds at one display sample/s
static constexpr int8_t NO_SIGNAL = -128;  // chart gap, never 0 dBm
static constexpr uint32_t PERIOD_MS = 1000;

struct Tracker {
    int8_t values[CAP] = {};
    uint8_t head = 0, count = 0;
    bool started = false, seen = false, peakValid = false;
    uint32_t lastPlotMs = 0, lastSeenMs = 0;
    int8_t peakDbm = -127;

    void reset() { *this = Tracker(); }

    // 'have' means this target has at least one observation, even if that
    // observation is too old for the live strength meter. Only live readings
    // can extend the graph with a signal or raise the session peak.
    void tick(uint32_t now, bool have, bool live, int8_t rssi, uint32_t lastSeen) {
        if (have) {
            seen = true;
            lastSeenMs = lastSeen;
        }
        if (live && have && (!peakValid || rssi > peakDbm)) {
            peakDbm = rssi;
            peakValid = true;
        }
        if (started && (uint32_t)(now - lastPlotMs) < PERIOD_MS) return;
        started = true;
        lastPlotMs = now;
        values[head] = (live && have) ? rssi : NO_SIGNAL;
        head = (uint8_t)((head + 1u) % CAP);
        if (count < CAP) count++;
    }

    // Chronological: at(0) is the oldest point, count-1 is the newest.
    int8_t at(uint8_t i) const {
        if (i >= count) return NO_SIGNAL;
        return values[(uint8_t)((head + CAP - count + i) % CAP)];
    }
};
} // namespace HuntHistory
