// HUNT radio-drop regression: live RSSI must expire and reacquisition must
// discard stale trend/catch history. Uses the real HUNT entry bookkeeping
// compiled into the native emulator plus a deterministic virtual clock.
#include "detection.h"
#include "hunt_signal.h"
#include "test_util.h"
#include <Arduino.h>
#include <cstring>

int main() {
    suite("Pure HUNT signal freshness and readout");
    ck("unknown target is not fresh", !HuntSignal::fresh(0, 0, 0));
    ck("fresh packet stays live", HuntSignal::fresh(1, 1000, 1001));
    ck("timeout inclusive at exactly five seconds", HuntSignal::fresh(2, 1000, 6000));
    ck("signal becomes absent after five seconds", !HuntSignal::fresh(2, 1000, 6001));
    ck("no need to restart history on close samples", !HuntSignal::restartHistory(2, 1000, 6000));
    ck("long gap resets the sample trend", HuntSignal::restartHistory(2, 1000, 6001));
    ck("millis wrap stays fresh", HuntSignal::fresh(2, 0xfffffff0u, 0x00000100u));
    ck("millis wrap expires correctly", !HuntSignal::fresh(2, 0xfffffff0u, 0x00001500u));
    ck("zero strength is not 0 dBm", std::strcmp(HuntSignal::absentReadout(true), "0%") == 0);
    ck("never heard stays waiting", std::strcmp(HuntSignal::absentReadout(false), "-- dBm") == 0 &&
        std::strcmp(HuntSignal::absentStatus(false), "WAITING FOR SIGNAL...") == 0);
    ck("lost target explicitly reports NO SIGNAL", std::strcmp(HuntSignal::absentStatus(true), "NO SIGNAL") == 0);

    suite("Active HUNT engine drops old readings without losing the target");
    SimClock::virtualTime = true;
    SimClock::nowMs = 1000;
    DetectionEngine eng;
    ck("engine initializes", eng.init());
    const uint8_t target[6] = {0x02,0x10,0x20,0x30,0x40,0x50};
    const uint8_t other[6] = {0x02,0x10,0x20,0x30,0x40,0x51};
    ck("start BLE HUNT", eng.huntBle(target, "Test tag"));
    ck("before first packet: no signal", !eng.huntSignalFresh(SimClock::nowMs));
    eng.checkHuntBle(other, -30); // wrong identity must not update our target
    ck("other MAC ignored", eng.huntRssiCount() == 0);
    eng.checkHuntBle(target, -42);
    ck("sample displayed live", eng.huntSignalFresh(SimClock::nowMs) &&
       eng.huntRssiCount() == 1 && eng.huntRssiAt(0) == -42);
    SimClock::nowMs = 3000;
    eng.checkHuntBle(target, -44);
    ck("second sample stored", eng.huntRssiCount() == 2);
    SimClock::nowMs = 8000;
    ck("five seconds after last packet still fresh", eng.huntSignalFresh(SimClock::nowMs));
    SimClock::nowMs = 8001;
    ck("expired packet is not live, no frozen dBm", !eng.huntSignalFresh(SimClock::nowMs));
    ck("history retained until next actual packet", eng.huntRssiCount() == 2);
    SimClock::nowMs = 8500;
    eng.checkHuntBle(target, -66);
    ck("reacquisition resets historical trend", eng.huntSignalFresh(SimClock::nowMs) &&
       eng.huntRssiCount() == 1 && eng.huntRssiAt(0) == -66);
    DetectionEngine::HuntTargetInfo info;
    ck("target remains selected and visible", eng.huntTargetInfo(0, info) &&
        info.seen && info.samples == 1 && info.rssi == -66);
    SimClock::nowMs = 10500;
    eng.checkHuntBle(target, -65);
    ck("fresh trend can resume only after new samples", eng.huntRssiCount() == 2);
    eng.deactivateHunt();
    ck("deactivated gauge has no live signal", !eng.huntSignalFresh(SimClock::nowMs));
    return report();
}
