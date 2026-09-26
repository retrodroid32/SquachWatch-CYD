// A failed dev2 migration must never destroy the v1.20.x source blob.
#include "ignore_list.h"
#include "test_util.h"
#include <Preferences.h>
#include <cstdlib>
#include <filesystem>

int main() {
    const char* dir="ignore_migrate_fail_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);

    const uint8_t old[7]={0x41,0x42,0x43,0x44,0x45,0x46,(uint8_t)DetectionType::FLOCK};
    Preferences p; p.begin("ignore",false); p.putBytes("dev",old,sizeof old);

    setenv("SQUACHSIM_NVS_FAIL_PUTBYTES_KEY","dev2",1);
    suite("failed migration retains source");
    IgnoreList::begin();
    ck("running RAM still reflects legacy record", IgnoreList::count()==1);
    ck("legacy policy remains IGNORE in RAM", IgnoreList::policy(old)==IgnoreList::Policy::IGNORE);

    Preferences q; q.begin("ignore",false);
    ck("dev2 was not partially committed", q.getBytesLength("dev2")==0);
    ck("old dev remains recoverable", q.getBytesLength("dev")==7);

    unsetenv("SQUACHSIM_NVS_FAIL_PUTBYTES_KEY");
    std::filesystem::remove_all(dir);
    return report();
}
