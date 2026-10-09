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

    // Real 3.2" diagnostics: 112.2 adverts/s over 5s, 11,252 contiguous B
    // at 43s uptime; AUTO had remained passive despite healthy memory.
    assert(mayEnterActive(true, 112, 11252));
    assert(mayEnterActive(true, 49, 8180));  // original photograph
    assert(!mayEnterActive(true, 112, 8180)); // busy entry needs extra margin
    assert(!mayEnterActive(true, 112, 10239));
    assert(mayEnterActive(true, 112, 10240));
    assert(!mayEnterActive(true, 150, 11252)); // do not enter when too busy
    assert(!mayEnterActive(true, 300, 20000));
    assert(!mayEnterActive(false, 112, 11252)); // generic BLE unchanged
    assert(mayEnterActive(false, 49, 8192));
    assert(!mayEnterActive(false, 49, 8191));
    std::puts("PASS: mesh receive entry/pressure floors and normal BLE safety");
}
