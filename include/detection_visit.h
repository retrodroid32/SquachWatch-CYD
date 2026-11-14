// SquachWatch-CYD -- track each visit without changing firstSeen.
// The original Detection::firstSeen is a per-row, since-boot record.
// A return starts a new visit for short-lived identity flood protection.
#pragma once
#include <stdint.h>

namespace DetectionVisit {
inline void restart(uint32_t& visitStartedMs, uint32_t seenNowMs) {
    visitStartedMs = seenNowMs;
}

// Unsigned subtraction keeps the elapsed visit duration correct even when
// Arduino millis() wraps (roughly every 49.7 days).
inline uint32_t elapsedMs(uint32_t visitStartedMs, uint32_t lastSeenMs) {
    return (uint32_t)(lastSeenMs - visitStartedMs);
}
}  // namespace DetectionVisit
