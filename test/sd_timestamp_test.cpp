#include "sd_timestamp.h"
#include "test_util.h"
#include <cstring>

int main() {
    char row[32];
    suite("SD UTC timestamp formatting");
    ck("trusted UTC is ISO 8601", SdTimestamp::format(row, sizeof row, true,
        1704067200u, 513u) && std::strcmp(row, "2024-01-01T00:00:00Z") == 0);
    ck("no clock uses numeric uptime", !SdTimestamp::format(row, sizeof row, false,
        1704067200u, 12345u) && std::strcmp(row, "12345") == 0);
    ck("unset date uses uptime", !SdTimestamp::format(row, sizeof row, true,
        0u, 678u) && std::strcmp(row, "678") == 0);
    char tiny[5];
    ck("bounded fallback does not overflow", !SdTimestamp::format(tiny, sizeof tiny,
        true, 1704067200u, 12u) && std::strcmp(tiny, "12") == 0);
    return report();
}
