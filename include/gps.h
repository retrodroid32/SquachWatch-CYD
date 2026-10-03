// SquachWatch-CYD — optional external UART GPS/GNSS support
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Gps {

struct Snapshot {
    bool     fix;
    bool     timeValid;
    bool     altValid;
    bool     hdopValid;
    double   lat;
    double   lon;
    float    altM;
    uint8_t  sats;
    uint16_t hdop100;
    uint32_t ageMs;
    uint32_t epoch;
};

void begin();
void tick(uint32_t now);
Snapshot snapshot();
void formatStatus(char* out, size_t n);
bool enabled();

} // namespace Gps
