// Current compact device-policy persistence.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
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
    IgnoreList::snooze(B);
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

    std::filesystem::remove_all(dir);
    return report();
}
