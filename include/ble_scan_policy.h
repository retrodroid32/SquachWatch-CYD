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

constexpr uint32_t activeEntryFloor(bool meshRx) {
    return meshRx ? MESH_ENTRY_BYTES : NORMAL_ENTRY_BYTES;
}
constexpr uint32_t pressureFloor(bool meshRx) {
    return meshRx ? MESH_PRESSURE_BYTES : NORMAL_PRESSURE_BYTES;
}
constexpr bool belowPressureFloor(bool meshRx, uint32_t largestBlock) {
    return largestBlock < pressureFloor(meshRx);
}
} // namespace BleScanPolicy
