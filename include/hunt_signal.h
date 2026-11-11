// SquachWatch-CYD — freshness policy for the live HUNT signal gauge.
// A historic RSSI sample is NOT a live signal. Five seconds allows time
// for Wi-Fi channel hopping and BLE advertisement gaps without freezing
// an old strength reading indefinitely. Intentionally independent of the
// 60-second detection LOG retention timeout.
#pragma once
#include <stdint.h>

namespace HuntSignal {
static constexpr uint32_t LOST_AFTER_MS = 5000u;

// Unsigned arithmetic keeps the timeout correct across millis() wraparound.
inline bool fresh(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs) {
    return samples > 0 && (uint32_t)(nowMs - lastSeenMs) <= LOST_AFTER_MS;
}

// The next valid packet after a long silence begins a NEW sample trend.
// Otherwise a previous, strong sample could falsely trigger CAUGHT again.
inline bool restartHistory(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs) {
    return samples > 0 && !fresh(samples, lastSeenMs, nowMs);
}

// Zero denotes *signal strength*, not 0 dBm (which is a strong RSSI).
inline const char* absentReadout(bool previouslySeen) {
    return previouslySeen ? "0%" : "-- dBm";
}
inline const char* absentStatus(bool previouslySeen) {
    return previouslySeen ? "NO SIGNAL" : "WAITING FOR SIGNAL...";
}
}  // namespace HuntSignal
