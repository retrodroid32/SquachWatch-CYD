#include "sd_io_policy.h"
#include "test_util.h"
#include <cstdint>

int main() {
    using namespace SdIoPolicy;
    suite("SD card hot-removal fail-closed policy");
    ck("normal append stays enabled", evaluate(true, 96, 96, 15) == Verdict::OK);
    ck("open failure disables writes", evaluate(false, 96, 0, 3) == Verdict::OPEN_FAILED);
    ck("zero write disables writes", evaluate(true, 96, 0, 4) == Verdict::SHORT_WRITE);
    ck("short write disables writes", evaluate(true, 96, 95, 7) == Verdict::SHORT_WRITE);
    ck("slow close disables even if write reported success",
       evaluate(true, 96, 96, MAX_APPEND_MS + 100) == Verdict::SLOW_IO);
    ck("just below threshold stays enabled",
       evaluate(true, 96, 96, MAX_APPEND_MS - 1) == Verdict::OK);
    ck("at threshold disables logging",
       evaluate(true, 96, 96, MAX_APPEND_MS) == Verdict::SLOW_IO);
    // Unsigned timer subtraction remains valid across a millis() wrap.
    const uint32_t start = 0xFFFFFF00u, end = 0x00000110u;
    ck("millis overflow still detects timeout",
       evaluate(true, 96, 96, (uint32_t)(end - start)) == Verdict::SLOW_IO);
    return report();
}
