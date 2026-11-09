// ESP32DualBandWardriver channel discovery-yield adaptation.
#include "channel_yield.h"
#include "test_util.h"
#include <cstdint>
int main() {
    suite("Unique-AP discovery yield normalized by dwell");
    ck("empty duration retains earlier estimate", updateChannelYieldEma(40, 6, 0) == 40);
    ck("one new BSSID in 250 ms yields initial one-per-second",
       updateChannelYieldEma(0, 1, 250) == 1);
    ck("eight new BSSIDs in 500 ms yields four per second EMA",
       updateChannelYieldEma(0, 8, 500) == 4);
    ck("smoother responds gradually to silence", updateChannelYieldEma(40, 0, 500) == 30);
    ck("bounded when sampling extreme durations",
       updateChannelYieldEma(65535, 65535, 1) == 65535);
    suite("One bonus share only for highly productive channels");
    ck("zero observations never get a bonus", !channelYieldBonus(0, 60));
    ck("quarter of peak is eligible", channelYieldBonus(15, 60));
    ck("below quarter peak stays ordinary", !channelYieldBonus(14, 60));
    return report();
}
