// v1.20.x seven-byte IGNORE records migrate transactionally to dev2.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
#include <cstdlib>
#include <filesystem>
#include <cstring>

int main() {
    const char* dir="ignore_migrate_dev_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);
    unsetenv("SQUACHSIM_NVS_FAIL_PUTBYTES_KEY");

    const uint8_t A[6]={0x10,0x11,0x12,0x13,0x14,0x15};
    const uint8_t B[6]={0x20,0x21,0x22,0x23,0x24,0x25};
    uint8_t old[14];
    memcpy(old,A,6); old[6]=(uint8_t)DetectionType::FLOCK;
    memcpy(old+7,B,6); old[13]=(uint8_t)DetectionType::RING;

    Preferences p; p.begin("ignore",false); p.putBytes("dev",old,sizeof old);

    suite("dev to dev2 migration");
    IgnoreList::begin();
    ck("two records loaded", IgnoreList::count()==2);
    ck("first remains IGNORE", IgnoreList::policy(A)==IgnoreList::Policy::IGNORE);
    ck("second remains IGNORE", IgnoreList::policy(B)==IgnoreList::Policy::IGNORE);
    ck("types preserved", IgnoreList::typeAt(0)==DetectionType::FLOCK && IgnoreList::typeAt(1)==DetectionType::RING);

    Preferences q; q.begin("ignore",false);
    ck("new compact blob written", q.getBytesLength("dev2")==16);
    ck("old dev removed only after success", q.getBytesLength("dev")==0);

    std::filesystem::remove_all(dir);
    return report();
}
