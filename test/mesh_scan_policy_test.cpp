// Host-only threshold policy tests; radio timing is verified by LAB hardware.
#include "ble_scan_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace BleScanPolicy;
    static_assert(NORMAL_ENTRY_BYTES == NORMAL_PRESSURE_BYTES, "Legacy BLE unchanged");
    static_assert(MESH_ENTRY_BYTES > MESH_PRESSURE_BYTES, "Hysteresis required");
    static_assert(MESH_PRESSURE_BYTES > 5u * 1024u, "Protect low-memory boards");
    // Actual diagnostics: 3.2" CYD, 8,180 B largest, 30.9 adverts/sec.
    assert(activeEntryFloor(true) <= 8180);
    assert(activeEntryFloor(false) > 8180);
    assert(!belowPressureFloor(true, 8180));
    // Normal BLE remains at 8 KB. Mesh switches back to passive once unsafe.
    assert(belowPressureFloor(false, 8180));
    assert(!belowPressureFloor(true, 7168));
    assert(belowPressureFloor(true, 7167));
    assert(activeEntryFloor(false) == 8192);
    assert(!belowPressureFloor(false, 8192));
    assert(belowPressureFloor(false, 8191));
    std::puts("PASS: mesh receive entry/pressure floors and normal BLE safety");
}
