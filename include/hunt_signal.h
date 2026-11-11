// SquachWatch-CYD — radio-aware freshness policy for the live HUNT gauge.
//
// A historical RSSI sample is not proof of a live transmitter, but the
// receiver is not listening continuously: BLE shares airtime with Wi-Fi,
// and the passive Wi-Fi sniffer must cycle through 13 channels (~4 sec
// per sweep). An unconditional 5-sec cutoff produced false NO SIGNAL / 0%
// flicker on continuously broadcasting targets that missed one sweep.
//
// BLE allows 9 seconds of missed advertisements. Wi-Fi allows 15 seconds,
// enough for >3 channel sweeps even if one or two beacons are missed.
// These are absence-confirmation windows, not RSSI averaging windows:
// every received packet still updates the displayed strength immediately.
// They remain much shorter than the 60-second detection LOG retention.
#pragma once
#include <stdint.h>

namespace HuntSignal {
static constexpr uint32_t BLE_LOST_AFTER_MS  = 9000u;
static constexpr uint32_t WIFI_LOST_AFTER_MS = 15000u;

inline uint32_t lossWindowMs(bool isWifi) {
    return isWifi ? WIFI_LOST_AFTER_MS : BLE_LOST_AFTER_MS;
}
// Unsigned arithmetic keeps the timeout correct across millis() wraparound.
inline bool fresh(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs,
                  bool isWifi = false) {
    return samples > 0 && (uint32_t)(nowMs - lastSeenMs) <= lossWindowMs(isWifi);
}

// A valid packet after *confirmed* signal loss starts a NEW RSSI trend.
// Transient scan gaps shorter than the loss window preserve trend history.
inline bool restartHistory(uint8_t samples, uint32_t lastSeenMs, uint32_t nowMs,
                           bool isWifi = false) {
    return samples > 0 && !fresh(samples, lastSeenMs, nowMs, isWifi);
}

// Zero is signal strength, never 0 dBm (a relatively strong RF level).
inline const char* absentReadout(bool previouslySeen) {
    return previouslySeen ? "0%" : "-- dBm";
}
inline const char* absentStatus(bool previouslySeen) {
    return previouslySeen ? "NO SIGNAL" : "WAITING FOR SIGNAL...";
}
}  // namespace HuntSignal
