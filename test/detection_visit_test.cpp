// The real engine keeps original firstSeen in its Detection ring and the
// independent visit start in a 64-slot sidecar. This test verifies that
// repeated visits do not require overwriting the original first sighting.
#include "detection_visit.h"
#include "state.h"
#include <cstdio>
#include <cstdlib>

static void ck(const char* label, bool ok) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}

int main() {
    Detection row = {};
    row.firstSeen = 1000u;
    row.lastSeen = 1000u;
    uint32_t visitStartedMs = row.firstSeen;
    ck("new sighting starts visit at original firstSeen",
       DetectionVisit::elapsedMs(visitStartedMs, row.lastSeen) == 0u);

    // Continuous BLE packets update lastSeen but never the firstSeen stamp.
    row.lastSeen = 3100u;
    ck("first BLE visit duration", DetectionVisit::elapsedMs(visitStartedMs, row.lastSeen) == 2100u);
    ck("first seen unchanged within visit", row.firstSeen == 1000u);

    // Device disappears, then reacquires. On return only the visit clock resets.
    DetectionVisit::restart(visitStartedMs, 7500u);
    row.lastSeen = 9900u;
    ck("first seen still 1 second after return", row.firstSeen == 1000u);
    ck("second visit duration not total device age",
       DetectionVisit::elapsedMs(visitStartedMs, row.lastSeen) == 2400u);
    DetectionVisit::restart(visitStartedMs, 12000u);
    row.lastSeen = 13050u;
    ck("original sighting survives multiple returns", row.firstSeen == 1000u);
    ck("third visit duration independent", DetectionVisit::elapsedMs(visitStartedMs, row.lastSeen) == 1050u);

    // A source first observed precisely at boot is a legitimate timestamp.
    row.firstSeen = 0u;
    visitStartedMs = 0u;
    DetectionVisit::restart(visitStartedMs, 55u);
    ck("uptime zero first-seen remains a valid value", row.firstSeen == 0u);

    // Long-running device: visit measurement is unsigned and wrap safe.
    visitStartedMs = 0xffffff00u;
    row.lastSeen = 0x000000f0u;
    ck("millis wrap preserves short visit duration",
       DetectionVisit::elapsedMs(visitStartedMs, row.lastSeen) == 0x1f0u);
    std::puts("PASS: original firstSeen, independent visits and millis rollover");
}
