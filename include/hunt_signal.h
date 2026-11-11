// SquachWatch-CYD -- HUNT signal freshness on concurrently updated clocks.
// The UI loop captures now at its START; NimBLE can deliver a new packet
// before HUNT paints, making lastSeenMs a few milliseconds "in the future".
// Unsigned now-lastSeenMs underflows and makes a live target show 0%.
// Compare signed modular age: a slight future observation is fresh, not old.
// At most one short timeout window is considered; rollover remains safe.
#pragma once
#include <stdint.h>

namespace HuntSignal {
// Active-target Wi-Fi monitoring holds a recently observed source channel.
// It must return to the full sweep when packets stop. BLE runs concurrently.
static constexpr uint32_t BLE_LOST_AFTER_MS  = 4000u;
static constexpr uint32_t WIFI_LOST_AFTER_MS = 6500u;
static constexpr uint32_t WIFI_FOCUS_RELEASE_MS = 2000u;

inline uint32_t lossWindowMs(bool isWifi) {
    return isWifi ? WIFI_LOST_AFTER_MS : BLE_LOST_AFTER_MS;
}
inline int32_t ageMs(uint32_t lastSeenMs, uint32_t nowMs) {
    const int32_t age = (int32_t)(nowMs - lastSeenMs);
    return age < 0 ? 0 : age;
}
inline bool fresh(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs,
                  bool isWifi = false) {
    return samples != 0 && (uint32_t)ageMs(lastSeenMs, nowMs) <= lossWindowMs(isWifi);
}
inline bool restartHistory(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs,
                           bool isWifi = false) {
    return samples > 0 && !fresh(samples, lastSeenMs, nowMs, isWifi);
}
inline bool focusWifi(uint8_t channel, uint8_t samples, uint32_t lastSeenMs,
                      uint32_t nowMs) {
    return channel >= 1 && channel <= 13 && samples > 0 &&
           (uint32_t)ageMs(lastSeenMs, nowMs) <= WIFI_FOCUS_RELEASE_MS;
}
// Absence means 0% signal strength, NOT 0 dBm (a strong received signal).
inline const char* absentReadout(bool previouslySeen) {
    return previouslySeen ? "0%" : "-- dBm";
}
inline const char* absentStatus(bool previouslySeen) {
    return previouslySeen ? "NO SIGNAL" : "WAITING FOR SIGNAL...";
}
}  // namespace HuntSignal
