// HUNT passive receiver regression: quiet channel sweeps must not flicker
// between a valid dBm and 0% while the target continues broadcasting.
// Run against the actual detection_sim bookkeeping with a virtual clock.
#include "detection.h"
#include "hunt_signal.h"
#include "test_util.h"
#include <Arduino.h>
#include <cstring>

int main() {
    suite("Radio-aware HUNT freshness");
    ck("no observation remains waiting", !HuntSignal::fresh(0, 0, 0));
    ck("BLE grace is 9s", HuntSignal::lossWindowMs(false) == 9000u);
    ck("Wi-Fi grace exceeds three ~4s sweeps", HuntSignal::lossWindowMs(true) == 15000u);
    ck("BLE still live at 5s", HuntSignal::fresh(2, 1000, 6001));
    ck("BLE grace inclusive at 9s", HuntSignal::fresh(2, 1000, 10000));
    ck("BLE absent after 9s", !HuntSignal::fresh(2, 1000, 10001));
    ck("Wi-Fi stays present through missed two sweeps", HuntSignal::fresh(1, 1000, 14000, true));
    ck("Wi-Fi grace inclusive at 15s", HuntSignal::fresh(2, 1000, 16000, true));
    ck("Wi-Fi absent after 15s", !HuntSignal::fresh(2, 1000, 16001, true));
    ck("Wi-Fi pause must not reset trends", !HuntSignal::restartHistory(2, 1000, 12500, true));
    ck("Wi-Fi confirmed absence must reset trends", HuntSignal::restartHistory(2, 1000, 16001, true));
    ck("BLE confirmed absence must reset trends", HuntSignal::restartHistory(2, 1000, 10001));
    ck("millis wrap BLE live", HuntSignal::fresh(2, 0xfffffff0u, 0x00000100u));
    ck("millis wrap Wi-Fi live", HuntSignal::fresh(2, 0xfffffff0u, 0x00001000u, true));
    ck("millis wrap BLE expires", !HuntSignal::fresh(2, 0xfffffff0u, 0x00002750u));
    ck("millis wrap Wi-Fi expires", !HuntSignal::fresh(2, 0xfffffff0u, 0x00003b00u, true));
    ck("no-signal strength is 0%, not 0 dBm",
       std::strcmp(HuntSignal::absentReadout(true), "0%") == 0);
    ck("never observed is waiting",
       std::strcmp(HuntSignal::absentReadout(false), "-- dBm") == 0 &&
       std::strcmp(HuntSignal::absentStatus(false), "WAITING FOR SIGNAL...") == 0);
    ck("confirmed lost target says NO SIGNAL",
       std::strcmp(HuntSignal::absentStatus(true), "NO SIGNAL") == 0);

    suite("BLE HUNT periodic reception gaps no longer flicker to 0%");
    SimClock::virtualTime = true;
    SimClock::nowMs = 1000;
    DetectionEngine eng;
    ck("engine initializes", eng.init());
    const uint8_t ble[6] = {0x02,0x10,0x20,0x30,0x40,0x50};
    const uint8_t other[6] = {0x02,0x10,0x20,0x30,0x40,0x51};
    const uint8_t wifi[6] = {0x02,0x11,0x22,0x33,0x44,0x55};
    ck("BLE HUNT selected", eng.huntBle(ble, "Test tag"));
    ck("waiting before any packets", !eng.huntSignalFresh(SimClock::nowMs));
    eng.checkHuntBle(other, -30);
    ck("wrong MAC ignored", eng.huntRssiCount() == 0);
    eng.checkHuntBle(ble, -42);
    ck("first BLE packet live", eng.huntSignalFresh(SimClock::nowMs) &&
       eng.huntRssiCount() == 1 && eng.huntRssiAt(0) == -42);
    // Ten radio visits with ~7s packet gaps: previous five-second
    // cutoff flickered to 0% between EVERY valid BLE reading.
    for (uint8_t i = 1; i <= 10; ++i) {
        SimClock::nowMs = 1000u + (uint32_t)i * 7000u - 500u;
        ck("gap between BLE packets remains live", eng.huntSignalFresh(SimClock::nowMs));
        SimClock::nowMs += 500u;
        eng.checkHuntBle(ble, (int8_t)(-42 - i));
        ck("new BLE sample remains live", eng.huntSignalFresh(SimClock::nowMs));
    }
    // At exact threshold, retains its last sample; afterwards it goes
    // absent, and a genuine return starts new trend history.
    SimClock::nowMs += HuntSignal::BLE_LOST_AFTER_MS;
    ck("BLE exactly at loss window is still present", eng.huntSignalFresh(SimClock::nowMs));
    SimClock::nowMs += 1u;
    ck("BLE confirmed off shows zero", !eng.huntSignalFresh(SimClock::nowMs));
    ck("old RSSI remains history, not live", eng.huntRssiCount() > 0);
    SimClock::nowMs += 500u;
    eng.checkHuntBle(ble, -66);
    ck("BLE reacquisition resets old trend and displays current dBm",
       eng.huntSignalFresh(SimClock::nowMs) && eng.huntRssiCount() == 1 &&
       eng.huntRssiAt(0) == -66);
    DetectionEngine::HuntTargetInfo info;
    ck("selected BLE target retained", eng.huntTargetInfo(0, info) &&
       info.samples == 1 && info.rssi == -66);

    suite("Wi-Fi HUNT revisits 13 channels without false zero");
    SimClock::nowMs = 1000;
    ck("Wi-Fi HUNT selected", eng.huntWifi(wifi, "Test AP"));
    ck("same MAC wrong transport ignored", !eng.huntSignalFresh(SimClock::nowMs));
    eng.checkHuntWifi(wifi, -56);
    ck("first Wi-Fi beacon captured", eng.huntSignalFresh(SimClock::nowMs));
    // 12-second gaps model two missed channel sweeps while radio is
    // broadcasting. Original R1 dropped out after five seconds.
    for (uint8_t i = 1; i <= 6; ++i) {
        SimClock::nowMs = 1000u + (uint32_t)i * 12000u - 250u;
        ck("still live despite missed Wi-Fi sweeps", eng.huntSignalFresh(SimClock::nowMs));
        SimClock::nowMs += 250u;
        eng.checkHuntWifi(wifi, (int8_t)(-56 - i));
        ck("Wi-Fi reception refreshes reading immediately", eng.huntSignalFresh(SimClock::nowMs));
    }
    SimClock::nowMs += HuntSignal::WIFI_LOST_AFTER_MS + 1u;
    ck("actual Wi-Fi silence eventually reaches zero", !eng.huntSignalFresh(SimClock::nowMs));
    ck("roster retains target after loss", eng.huntTargetInfo(1, info) && info.seen);
    SimClock::nowMs += 100u;
    eng.checkHuntWifi(wifi, -61);
    ck("Wi-Fi return resets pre-outage CAUGHT history",
       eng.huntSignalFresh(SimClock::nowMs) && eng.huntRssiCount() == 1 &&
       eng.huntRssiAt(0) == -61);
    eng.deactivateHunt();
    ck("back/deactivate never shows old live signal", !eng.huntSignalFresh(SimClock::nowMs));

    return report();
}
