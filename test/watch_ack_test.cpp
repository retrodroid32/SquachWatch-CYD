// WATCH notification state regression: multiple targets, explicit snooze,
// manual acknowledgment, stable roster identity and removal.
// PR #149 post-integration CI checkpoint: confirm WATCH regression suite compiles
// alongside adaptive framebuffer recovery; hardware validation remains separate.
#include "detection.h"
#include "ignore_list.h"
#include "watch_alert_actions.h"
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
    ck("automatic WATCH alert returns home",
       WatchAlertActions::returnPage(false)==WatchAlertActions::ReturnPage::HOME);
    ck("roster-opened WATCH alert returns to roster",
       WatchAlertActions::returnPage(true)==WatchAlertActions::ReturnPage::ROSTER);
    ck("normal ALERT WATCH adds BLE target A",
       WatchAlertActions::addOnly(e,A,"Target A",true)==WatchAlertActions::AddResult::ADDED);
    ck("normal ALERT WATCH is idempotent (never removes A)",
       WatchAlertActions::addOnly(e,A,"Target A",true)==WatchAlertActions::AddResult::ALREADY_WATCHED
       && e.watchTargetCount()==1 && e.isWatched(A,true));
    ck("add BLE target B",
       WatchAlertActions::addOnly(e,B,"Target B",true)==WatchAlertActions::AddResult::ADDED);
    e.postBle(sighting(A,-55));
    e.postBle(sighting(B,-67));
    ck("first pending WATCH hit", e.watchHitPending());
    ck("first target stays selected and unacknowledged", e.watchTargetCount()==2 && e.watchUnacknowledgedCount()==1);
    // Simulate the unattended 10-second return to the main screen.
    e.deactivateWatch();
    ck("automatic exit preserves unread notice", e.watchUnacknowledgedCount()==1);
    ck("second target queues separately", e.watchHitPending());
    ck("two outstanding notices", e.watchUnacknowledgedCount()==2);
    e.acknowledgeWatch(); // the only manual dismiss action
    ck("manual dismiss ACKs only this notice, not its WATCH target", e.watchTargetCount()==2 && e.watchUnacknowledgedCount()==1);
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
    suite("One-tap WATCH capacity and Wi-Fi identity");
    const uint8_t WIFI[6]={0xE0,0xE1,0xE2,0xE3,0xE4,0xE5};
    ck("normal ALERT can add a Wi-Fi WATCH",
       WatchAlertActions::addOnly(e,WIFI,"AP",false)==WatchAlertActions::AddResult::ADDED);
    ck("same MAC, Wi-Fi and BLE are distinct targets",
       WatchAlertActions::addOnly(e,WIFI,"BLE",true)==WatchAlertActions::AddResult::ADDED);
    ck("Wi-Fi add does not remove BLE selection",
       WatchAlertActions::addOnly(e,WIFI,"AP",false)==WatchAlertActions::AddResult::ALREADY_WATCHED
       && e.isWatched(WIFI,true) && e.isWatched(WIFI,false));
    for (uint8_t i=0; i<6; i++) {
        const uint8_t mac[6]={0x90,0x91,0x92,0x93,0x94,i};
        WatchAlertActions::addOnly(e,mac,"test",true);
    }
    ck("WATCH list reaches capacity", e.watchTargetCount()==DetectionEngine::WATCH_TARGET_CAP);
    const uint8_t overflow[6]={0xAB,0xBC,0xCD,0xDE,0xEF,0x12};
    ck("full WATCH list refuses new target without changing any",
       WatchAlertActions::addOnly(e,overflow,"extra",true)==WatchAlertActions::AddResult::FULL
       && e.watchTargetCount()==DetectionEngine::WATCH_TARGET_CAP);

    std::filesystem::remove_all(dir);
    return report();
}
