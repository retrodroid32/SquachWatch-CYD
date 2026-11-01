// Current compact device-policy persistence.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
#include <Arduino.h>
#include <cstdlib>
#include <filesystem>

static const char* NS = "ignore";
static const char* KEY = "dev2";
static const uint8_t A[6] = {0xB4,0x1E,0x52,0x01,0x02,0x03};
static const uint8_t B[6] = {0xAC,0x9F,0xC3,0x0A,0x0B,0x0C};

static size_t bytesOnDisk(const char* key=KEY) {
    Preferences p; p.begin(NS, false); return p.getBytesLength(key);
}

int main() {
    const char* dir="ignore_test_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);
    unsetenv("SQUACHSIM_NVS_FAIL_PUTBYTES_KEY");
    SimClock::virtualTime = true;
    SimClock::nowMs = 100;


    suite("Compact device policies");
    ck("set IGNORE", IgnoreList::setPolicy(A, DetectionType::FLOCK, IgnoreList::Policy::IGNORE));
    ck("set TRUSTED", IgnoreList::setPolicy(B, DetectionType::RING, IgnoreList::Policy::TRUSTED));
    ck("two records use 16 bytes", bytesOnDisk() == 16);
    ck("contains means IGNORE only", IgnoreList::contains(A) && !IgnoreList::contains(B));
    ck("trusted helper", IgnoreList::trusted(B));
    ck("ignored device is silenced", IgnoreList::silenced(A));
    ck("trusted device is silenced", IgnoreList::silenced(B));

    ck("update to ALWAYS ALERT", IgnoreList::setPolicy(B, DetectionType::RING, IgnoreList::Policy::ALWAYS_ALERT));
    ck("record size unchanged on update", bytesOnDisk() == 16);
    ck("always-alert helper", IgnoreList::alwaysAlert(B));
    IgnoreList::snooze(B, IgnoreList::SnoozeDuration::MIN_10);
    ck("always-alert bypasses snooze", !IgnoreList::silenced(B));

    suite("Removal persists");
    ck("remove one", IgnoreList::remove(A));
    ck("one compact record remains", bytesOnDisk() == 8);
    ck("survivor policy preserved", IgnoreList::policy(B) == IgnoreList::Policy::ALWAYS_ALERT);
    ck("remove last", IgnoreList::remove(B));
    ck("empty list removes dev2 key", bytesOnDisk() == 0);

    suite("clear removes all generations");
    ck("re-add", IgnoreList::setPolicy(A, DetectionType::FLOCK, IgnoreList::Policy::TRUSTED));
    Preferences p; p.begin(NS, false);
    const uint8_t old[7] = {1,2,3,4,5,6,(uint8_t)DetectionType::RING};
    p.putBytes("dev", old, sizeof old);
    const uint8_t older[6] = {6,5,4,3,2,1};
    p.putBytes("macs", older, sizeof older);
    IgnoreList::clear();
    ck("dev2 gone", bytesOnDisk("dev2") == 0);
    ck("dev gone", bytesOnDisk("dev") == 0);
    ck("macs gone", bytesOnDisk("macs") == 0);

    suite("Legacy reversed BLE policy still matches");
    {
        const uint8_t fwd[6] = { 0x5E, 0xC5, 0xC1, 0xCF, 0x45, 0xC4 };
        const uint8_t oldOrder[6] = { 0xC4, 0x45, 0xCF, 0xC1, 0xC5, 0x5E };
        ck("save legacy ignore", IgnoreList::setPolicy(oldOrder, DetectionType::AIRTAG, IgnoreList::Policy::IGNORE));
        ck("canonical address inherits ignore", IgnoreList::contains(fwd));
        ck("canonical unignore removes legacy entry", IgnoreList::remove(fwd) && !IgnoreList::contains(oldOrder));
    }

    suite("Timed and persistent snoozes");
    const uint8_t C[6] = {0x12,0x34,0x56,0x78,0x9A,0xBC};
    SimClock::nowMs = 100;
    ck("10 minute snooze inserted", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::MIN_10));
    ck("selected device is snoozed", IgnoreList::snoozed(A));
    ck("does not silence unrelated MAC", !IgnoreList::snoozed(C));
    uint8_t shown[6]; uint32_t remaining = 0; bool forever = false;
    ck("entry info includes address and timer",
       IgnoreList::snoozeInfo(0, shown, remaining, forever) &&
       memcmp(shown, A, 6) == 0 && !forever && remaining == 600000);
    SimClock::nowMs += 599999;
    ck("10 minute snooze active before deadline", IgnoreList::snoozed(A));
    SimClock::nowMs += 1;
    ck("10 minute snooze automatically expires", !IgnoreList::snoozed(A) && IgnoreList::snoozeCount() == 0);
    ck("30 minute snooze inserted", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::MIN_30));
    SimClock::nowMs += 1800000;
    ck("30 minute deadline expires", !IgnoreList::snoozed(A));
    ck("one hour snooze inserted", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::HOUR_1));
    SimClock::nowMs += 3600000;
    ck("one hour deadline expires", !IgnoreList::snoozed(A));
    SimClock::nowMs = UINT32_MAX - 1000;
    ck("wraparound snooze accepted", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::MIN_10));
    SimClock::nowMs += 599999;
    ck("wraparound active just before expiry", IgnoreList::snoozed(A));
    SimClock::nowMs++;
    ck("wraparound expires correctly", !IgnoreList::snoozed(A));

    ck("ALWAYS snooze persists as MAC-only key", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::ALWAYS) && bytesOnDisk("snz") == 6);
    ck("ALWAYS entry has no expiry", IgnoreList::snoozeInfo(0, shown, remaining, forever) && forever && remaining == 0);
    SimClock::nowMs += 3600000;
    ck("ALWAYS unaffected by elapsed time", IgnoreList::snoozed(A));
    ck("unsnooze individual permanent", IgnoreList::unsnooze(A) && !IgnoreList::snoozed(A));
    ck("unsnooze removes persisted key", bytesOnDisk("snz") == 0);
    ck("permanent-to-timed removes NVS key", IgnoreList::snooze(A, IgnoreList::SnoozeDuration::ALWAYS) &&
       IgnoreList::snooze(A, IgnoreList::SnoozeDuration::MIN_10) && bytesOnDisk("snz") == 0);
    ck("manual timed unsnooze works", IgnoreList::unsnooze(A) && !IgnoreList::snoozed(A));

    // Exhaust the bounded list: the 33rd cannot silently evict another user choice.
    for (uint8_t i = 0; i < IgnoreList::SNOOZE_MAX; i++) {
        uint8_t mac[6] = {0x42,0x43,0x44,0x45,0x46,i};
        IgnoreList::snooze(mac, IgnoreList::SnoozeDuration::HOUR_1);
    }
    ck("all 32 fit", IgnoreList::snoozeCount() == IgnoreList::SNOOZE_MAX);
    ck("33rd reports full", !IgnoreList::snooze(C, IgnoreList::SnoozeDuration::HOUR_1));
    const uint8_t first[6] = {0x42,0x43,0x44,0x45,0x46,0x00};
    ck("remove then add new entry", IgnoreList::unsnooze(first) &&
       IgnoreList::snooze(C, IgnoreList::SnoozeDuration::HOUR_1) &&
       IgnoreList::snoozeCount() == IgnoreList::SNOOZE_MAX);
    IgnoreList::clear();
    ck("clear erases active snooze list", IgnoreList::snoozeCount() == 0);

    std::filesystem::remove_all(dir);
    return report();
}
