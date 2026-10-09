// SquachWatch-CYD — memory floors for BLE scanning and SquachMesh reception.
//
// A message rides in a *scan response*. Starting passive means peers remain
// visible but messages/invites cannot be received. The normal 8 KB floor was
// taken from a 2.8" bench, but one 3.2" diagnostic showed 8,180 contiguous
// bytes at 31 adverts/sec: just 12 bytes below the floor, and permanently
// unable to switch to active scanning.
//
// This is an experimental, MESH-ONLY exception for hardware LAB testing.
// Never disable the callback guard or lift the safety pressure fallback.
// The two thresholds form 512 B hysteresis, avoiding repeated toggles when
// the largest allocation changes slightly during scans.
#pragma once
#include <stdint.h>

namespace BleScanPolicy {
constexpr uint32_t NORMAL_ENTRY_BYTES    = 8192;
constexpr uint32_t MESH_ENTRY_BYTES      = 7680;
constexpr uint32_t MESH_PRESSURE_BYTES   = 7168;
constexpr uint32_t NORMAL_PRESSURE_BYTES = 8192;

// Startup AUTO scanning historically required under 50 adverts/second.
// Once active, it tolerated as many as 300 before yielding, so a reboot
// at 112/s could strand an otherwise healthy 3.2-inch CYD in PASSIVE.
//
// Allow medium-traffic SquachMesh reception only with more contiguous memory
// than a quiet startup. This is NOT a blanket increase for all BLE traffic.
constexpr uint32_t QUIET_ENTRY_RATE_BELOW  = 50;
constexpr uint32_t MESH_ENTRY_RATE_BELOW   = 150;
constexpr uint32_t MESH_BUSY_ENTRY_BYTES   = 10240;

constexpr uint32_t activeEntryFloor(bool meshRx) {
    return meshRx ? MESH_ENTRY_BYTES : NORMAL_ENTRY_BYTES;
}
constexpr uint32_t pressureFloor(bool meshRx) {
    return meshRx ? MESH_PRESSURE_BYTES : NORMAL_PRESSURE_BYTES;
}
constexpr bool belowPressureFloor(bool meshRx, uint32_t largestBlock) {
    return largestBlock < pressureFloor(meshRx);
}

// Consulted only when switching AUTO from PASSIVE to ACTIVE. The separate
// existing 300 adverts/s hard ceiling and heap-pressure recovery are kept.
inline bool mayEnterActive(bool meshRx, uint32_t rate, uint32_t largest) {
    if (!meshRx)
        return rate < QUIET_ENTRY_RATE_BELOW &&
               largest >= NORMAL_ENTRY_BYTES;
    if (rate < QUIET_ENTRY_RATE_BELOW)
        return largest >= MESH_ENTRY_BYTES;
    return rate < MESH_ENTRY_RATE_BELOW &&
           largest >= MESH_BUSY_ENTRY_BYTES;
}
} // namespace BleScanPolicy
