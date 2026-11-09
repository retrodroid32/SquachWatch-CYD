// SquachWatch-CYD — channel-yield score adapted from ESP32DualBandWardriver
// by JustCallMeKoko (MIT). A pure helper: no WiFi driver or heap allocation.
// Counts distinct AP BSSID hash buckets per *second*, not all noisy frames.
// 1/4 new sample + 3/4 history keeps one burst from dominating the sweep.
#pragma once
#include <stdint.h>
#include <limits.h>
inline uint16_t updateChannelYieldEma(uint16_t oldPerSec,
                                      uint16_t distinctBssids,
                                      uint32_t spentMs) {
    if (spentMs == 0) return oldPerSec;
    const uint64_t scaled = (uint64_t)distinctBssids * 1000u / spentMs;
    const uint32_t sample = scaled > UINT16_MAX ? UINT16_MAX : (uint32_t)scaled;
    const uint32_t ema = ((uint32_t)oldPerSec * 3u + sample + 2u) / 4u;
    return ema > UINT16_MAX ? UINT16_MAX : (uint16_t)ema;
}
// A productive channel gets at most one extra dwell share. The caller still
// visits all 13 channels per sweep with an enforced >=120ms floor.
inline bool channelYieldBonus(uint16_t yield, uint16_t peak) {
    return peak > 0 && yield > 0 && (uint32_t)yield * 4u >= peak;
}
