#include "hunt_history.h"
#include <cassert>
#include <cstdio>

int main() {
    HuntHistory::Tracker h;
    assert(h.count == 0 && !h.peakValid && !h.seen);
    h.tick(100, false, false, -127, 0);
    assert(h.count == 1 && h.at(0) == HuntHistory::NO_SIGNAL);
    h.tick(999, true, true, -70, 999);
    assert(h.count == 1 && h.peakValid && h.peakDbm == -70);
    h.tick(1100, true, true, -63, 1100);
    assert(h.count == 2 && h.at(1) == -63 && h.peakDbm == -63);
    h.tick(2100, true, true, -80, 2100);
    assert(h.count == 3 && h.peakDbm == -63);
    h.tick(3100, true, false, -127, 2100);
    assert(h.count == 4 && h.at(3) == HuntHistory::NO_SIGNAL);
    assert(h.seen && h.lastSeenMs == 2100 && h.peakDbm == -63);
    // Forty points are bounded; the oldest is displaced without any heap.
    for (unsigned i = 0; i < 65; i++) h.tick(4100 + i * 1000u, true, true, -75, 4100 + i * 1000u);
    assert(h.count == HuntHistory::CAP);
    assert(h.at(0) == -75 && h.at(HuntHistory::CAP - 1) == -75);
    assert(h.at(HuntHistory::CAP) == HuntHistory::NO_SIGNAL);
    assert(h.peakDbm == -63);
    // Wraparound of the sampling clock never generates runaway samples.
    h.reset();
    h.tick(0xFFFFFFF0u, true, true, -85, 0xFFFFFFF0u);
    h.tick(50u, true, true, -75, 50u);
    assert(h.count == 1 && h.peakDbm == -75);
    h.tick(1050u, true, false, -127, 50u);
    assert(h.count == 2 && h.at(1) == HuntHistory::NO_SIGNAL);
    h.reset();
    assert(h.count == 0 && !h.peakValid && !h.seen);
    puts("PASS HUNT history: 40s bounded chart, peak, signal loss and reset");
}
