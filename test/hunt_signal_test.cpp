// Regression for HUNT 0% flashes and excessively slow signal-off handling.
// Uses the same passive-radio freshness / snapshot rules as firmware, with
// deterministic virtual time and both BLE and Wi-Fi source identities.
#include "detection.h"
#include "hunt_signal.h"
#include "test_util.h"
#include <Arduino.h>
#include <cstring>

int main() {
    suite("R3 time arithmetic -- callbacks may arrive after frame now capture");
    ck("unobserved target remains waiting", !HuntSignal::fresh(0, 0, 0));
    ck("BLE absence 4 seconds", HuntSignal::lossWindowMs(false) == 4000u);
    ck("Wi-Fi absence 6.5 seconds", HuntSignal::lossWindowMs(true) == 6500u);
    ck("same-time packet is fresh", HuntSignal::fresh(1, 1000, 1000));
    ck("packet from 1ms AFTER loop timestamp is fresh",
       HuntSignal::fresh(1, 1001, 1000));
    ck("packet from 20ms AFTER loop timestamp is fresh",
       HuntSignal::fresh(2, 1020, 1000, true));
    ck("packet from 150ms AFTER loop timestamp never yields false zero",
       HuntSignal::fresh(2, 1150, 1000));
    ck("future lastSeen clamps to zero age", HuntSignal::ageMs(1050, 1000) == 0);
    ck("BLE remains live at 4s", HuntSignal::fresh(1, 1000, 5000));
    ck("BLE becomes absent at 4s + 1ms", !HuntSignal::fresh(1, 1000, 5001));
    ck("Wi-Fi remains live at 6.5s", HuntSignal::fresh(1, 1000, 7500, true));
    ck("Wi-Fi becomes absent at 6.5s + 1ms", !HuntSignal::fresh(1, 1000, 7501, true));
    ck("BLE history only resets on confirmed absence",
       !HuntSignal::restartHistory(2, 1000, 4999) &&
       HuntSignal::restartHistory(2, 1000, 5001));
    ck("Wi-Fi history only resets on confirmed absence",
       !HuntSignal::restartHistory(2, 1000, 7499, true) &&
       HuntSignal::restartHistory(2, 1000, 7501, true));
    ck("counter rollover keeps short elapsed times correct",
       HuntSignal::fresh(1, 0xfffffff0u, 0x00000010u));
    ck("counter rollover correctly expires BLE",
       !HuntSignal::fresh(1, 0xfffffff0u, 0x00000fc0u));
    ck("counter rollover correctly expires Wi-Fi",
       !HuntSignal::fresh(1, 0xfffffff0u, 0x00001970u, true));
    ck("never heard displays -- dBm",
       std::strcmp(HuntSignal::absentReadout(false), "-- dBm") == 0);
    ck("lost displays 0 percent, never 0 dBm",
       std::strcmp(HuntSignal::absentReadout(true), "0%") == 0 &&
       std::strcmp(HuntSignal::absentStatus(true), "NO SIGNAL") == 0);

    suite("Wi-Fi focused dwell remains on a fresh HUNT target channel");
    ck("unknown channel cannot focus", !HuntSignal::focusWifi(0, 1, 1000, 1100));
    ck("invalid channel cannot focus", !HuntSignal::focusWifi(14, 1, 1000, 1100));
    ck("unseen target cannot focus", !HuntSignal::focusWifi(6, 0, 0, 1000));
    ck("fresh Wi-Fi frame holds the channel", HuntSignal::focusWifi(6, 1, 1000, 3000));
    ck("after 2 seconds of silence channel sweep resumes",
       !HuntSignal::focusWifi(6, 1, 1000, 3001));
    ck("brief clock skew does not release focus",
       HuntSignal::focusWifi(11, 1, 1050, 1000));

    SimClock::virtualTime = true;
    SimClock::nowMs = 1000;
    DetectionEngine eng;
    ck("engine initializes", eng.init());
    const uint8_t ble[6] = {0x02,0x10,0x20,0x30,0x40,0x50};
    const uint8_t other[6] = {0x02,0x10,0x20,0x30,0x40,0x51};
    const uint8_t wifi[6] = {0x02,0x11,0x22,0x33,0x44,0x55};
    DetectionEngine::HuntSignalSnapshot snap;

    suite("BLE active HUNT snapshot never mixes new timestamps with old count");
    ck("BLE HUNT selected", eng.huntBle(ble, "Test tag"));
    ck("no BLE reading until first packet", !eng.huntSignalFresh(1000));
    eng.checkHuntBle(other, -31);
    ck("wrong BLE MAC ignored", eng.huntRssiCount() == 0);
    eng.checkHuntBle(ble, -42);
    ck("BLE snapshot has one sample and no Wi-Fi channel",
       eng.huntSignalSnapshot(snap) && snap.kind == DetectionEngine::WatchKind::BLE &&
       snap.samples == 1 && snap.rssi[0] == -42 && snap.channel == 0);
    ck("callback after display clock still shows strength",
       eng.huntSignalFresh(999) && eng.huntSignalFresh(1000));
    // Regular, sometimes delayed, BLE reception over more than 30s.
    for (uint8_t i = 1; i <= 18; ++i) {
        SimClock::nowMs = 1000u + (uint32_t)i * 1750u - 100u;
        ck("BLE on: no transient 0%", eng.huntSignalFresh(SimClock::nowMs));
        SimClock::nowMs += 100u;
        eng.checkHuntBle(ble, (int8_t)(-42 - (i % 8)));
        ck("BLE callback timestamp newer than frame is still fresh",
           eng.huntSignalFresh(SimClock::nowMs - 1u));
    }
    SimClock::nowMs += HuntSignal::BLE_LOST_AFTER_MS + 1u;
    ck("BLE off: returns to zero by 4s", !eng.huntSignalFresh(SimClock::nowMs));
    SimClock::nowMs += 200u;
    eng.checkHuntBle(ble, -61);
    ck("BLE return: sample history restarts cleanly",
       eng.huntSignalSnapshot(snap) && snap.samples == 1 &&
       snap.rssi[0] == -61 && eng.huntSignalFresh(SimClock::nowMs));

    suite("Wi-Fi HUNT received BSSID channel and repeated sweeps");
    SimClock::nowMs = 1000;
    ck("Wi-Fi HUNT selected", eng.huntWifi(wifi, "Test AP"));
    ck("not a BLE sample carried across", !eng.huntSignalFresh(SimClock::nowMs));
    eng.postWiFi(other, -38, 6); // wrong MAC, cannot refresh
    ck("unrelated BSSID ignored", eng.huntRssiCount() == 0);
    eng.postWiFi(wifi, -55, 6);
    ck("Wi-Fi channel 6 stored for focused monitoring",
       eng.huntSignalSnapshot(snap) && snap.samples == 1 &&
       snap.kind == DetectionEngine::WatchKind::WIFI &&
       snap.channel == 6 && snap.rssi[0] == -55);
    // The normal scan makes a ~3.9s pass through all 13 Wi-Fi channels,
    // then target-focus should hold its channel for tighter packet cadence.
    for (uint8_t i = 1; i <= 12; ++i) {
        SimClock::nowMs = 1000u + (uint32_t)i * 3900u - 100u;
        ck("Wi-Fi beacon still live between full-sweep visits",
           eng.huntSignalFresh(SimClock::nowMs));
        SimClock::nowMs += 100u;
        eng.postWiFi(wifi, (int8_t)(-55 - (i % 8)), 6);
        ck("Wi-Fi callback after frame clock remains live",
           eng.huntSignalFresh(SimClock::nowMs - 1u));
    }
    SimClock::nowMs += HuntSignal::WIFI_LOST_AFTER_MS + 1u;
    ck("Wi-Fi really OFF: returns to zero by 6.5s",
       !eng.huntSignalFresh(SimClock::nowMs));
    ck("channel focus released after loss",
       eng.huntSignalSnapshot(snap) &&
       !HuntSignal::focusWifi(snap.channel, snap.samples, snap.lastSeenMs, SimClock::nowMs));
    SimClock::nowMs += 100u;
    eng.postWiFi(wifi, -63, 11);
    ck("Wi-Fi reacquisition updates channel and clears old RSSI history",
       eng.huntSignalSnapshot(snap) && snap.samples == 1 &&
       snap.channel == 11 && snap.rssi[0] == -63);
    ck("Wi-Fi post-loss callback is fresh",
       eng.huntSignalFresh(SimClock::nowMs - 1u));
    eng.deactivateHunt();
    ck("deactivated HUNT does not show stale source",
       !eng.huntSignalSnapshot(snap) && !eng.huntSignalFresh(SimClock::nowMs));

    return report();
}
