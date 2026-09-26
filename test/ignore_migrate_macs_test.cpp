// Oldest six-byte MAC-only IGNORE records migrate to UNKNOWN-typed dev2.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
#include <cstdlib>
#include <filesystem>

int main() {
    const char* dir="ignore_migrate_macs_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);
    unsetenv("SQUACHSIM_NVS_FAIL_PUTBYTES_KEY");

    const uint8_t A[6]={0x31,0x32,0x33,0x34,0x35,0x36};
    Preferences p; p.begin("ignore",false); p.putBytes("macs",A,sizeof A);

    suite("macs to dev2 migration");
    IgnoreList::begin();
    ck("one record loaded", IgnoreList::count()==1);
    ck("policy remains IGNORE", IgnoreList::policy(A)==IgnoreList::Policy::IGNORE);
    ck("unknown type is honest", IgnoreList::typeAt(0)==DetectionType::UNKNOWN);

    Preferences q; q.begin("ignore",false);
    ck("new 8-byte record written", q.getBytesLength("dev2")==8);
    ck("old macs removed after success", q.getBytesLength("macs")==0);

    std::filesystem::remove_all(dir);
    return report();
}
