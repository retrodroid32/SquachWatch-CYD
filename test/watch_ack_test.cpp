// WATCH notification state regression: multiple targets, explicit snooze,
// manual acknowledgment, stable roster identity and removal.
#include "detection.h"
#include "ignore_list.h"
#include "test_util.h"
#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <filesystem>

static Detection sighting(const uint8_t* mac, int8_t rssi) {
    Detection d = {};
    std::memcpy(d.mac, mac, 6);
    d.type = DetectionType::UNKNOWN;
    d.channel = 0;
    d.rssi = rssi;
    return d;
}

int main() {
    const char* dir = "watch_ack_test_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);
    SimClock::virtualTime = true;
    SimClock::nowMs = 1234;

    suite("WATCH per-target acknowledgement");
    DetectionEngine e;
    ck("engine initializes", e.init());
    const uint8_t A[6] = {0x10,0x20,0x30,0x40,0x50,0x60};
    const uint8_t B[6] = {0x11,0x21,0x31,0x41,0x51,0x61};
    ck("add BLE target A", e.toggleWatchBle(A,"Target A") == DetectionEngine::WatchToggle::ADDED);
    ck("add BLE target B", e.toggleWatchBle(B,"Target B") == DetectionEngine::WatchToggle::ADDED);
    e.postBle(sighting(A,-55));
    e.postBle(sighting(B,-67));
    ck("first pending WATCH hit", e.watchHitPending());
    ck("first target stays selected and unacknowledged", e.watchTargetCount()==2 && e.watchUnacknowledgedCount()==1);
    e.deactivateWatch(); // minimize without acknowledgment
    ck("minimize preserves notice", e.watchUnacknowledgedCount()==1);
    ck("second target queues separately", e.watchHitPending());
    ck("two outstanding notices", e.watchUnacknowledgedCount()==2);
    e.acknowledgeWatch();
    ck("ACK removes only second notice, not its watched target", e.watchTargetCount()==2 && e.watchUnacknowledgedCount()==1);
    ck("no duplicate of first unacknowledged event", !e.watchHitPending());
    ck("open target A by identity", e.activateWatchTarget(0));
    e.acknowledgeWatch();
    ck("manually opened first notice can be ACKed", e.watchTargetCount()==2 && e.watchUnacknowledgedCount()==0);

    suite("WATCH versus explicit SNOOZE");
    ck("set manual 10 minute snooze A", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::MIN_10));
    e.postBle(sighting(A,-39));
    ck("snoozed device generates no WATCH modal", !e.watchHitPending());
    ck("snooze does not unwatch target", e.watchTargetCount()==2 && e.isWatched(A,true));
    ck("snoozed device leaves no badge", e.watchUnacknowledgedCount()==0);
    ck("manual unsnooze works", IgnoreList::unsnooze(A));
    e.postBle(sighting(A,-42));
    ck("WATCH resumes after unsnooze", e.watchHitPending());
    ck("new alert gets own unread notice", e.watchUnacknowledgedCount()==1);
    e.clearWatch();
    ck("REMOVE affects only alerting target", e.watchTargetCount()==1 && !e.isWatched(A,true) && e.isWatched(B,true));
    ck("remove clears notice", e.watchUnacknowledgedCount()==0);
    e.clearWatches();
    ck("clear watches leaves none", e.watchTargetCount()==0 && !e.watchHitPending());
    std::filesystem::remove_all(dir);
    return report();
}
