// Stage A detection evidence and rolling RSSI primitives.
#include "state.h"
#include "test_util.h"
#include <cstring>

static Detection blankDetection() {
    Detection d{};
    return d;
}

int main() {
    suite("Detection evidence and RSSI trend");

    ck("full evidence label", std::strcmp(evidenceKindName(EvidenceKind::WIFI_OUI), "WIFI OUI") == 0);
    ck("compact evidence label", std::strcmp(evidenceKindShortName(EvidenceKind::BLE_FINDMY), "FINDMY") == 0);
    ck("unknown evidence stays explicit", std::strcmp(evidenceKindName(EvidenceKind::UNKNOWN), "UNKNOWN") == 0);

    Detection approaching = blankDetection();
    detectionRssiInit(approaching, -70, 0);
    detectionRssiSample(approaching, -66, 2048);
    detectionRssiSample(approaching, -62, 4096);
    ck("three samples establish trend", approaching.rssiHistCount == 3);
    ck("positive 8 dB is approaching", detectionRssiTrend(approaching) == RssiTrend::APPROACHING);
    ck("latest sample is current RSSI", approaching.rssi == -62);
    ck("history reads oldest first", detectionRssiAt(approaching, 0) == -70);
    ck("history reads newest last", detectionRssiAt(approaching, 2) == -62);

    Detection steady = blankDetection();
    detectionRssiInit(steady, -60, 0);
    detectionRssiSample(steady, -58, 2048);
    detectionRssiSample(steady, -61, 4096);
    ck("small total change is steady", detectionRssiTrend(steady) == RssiTrend::STEADY);

    Detection away = blankDetection();
    detectionRssiInit(away, -55, 0);
    detectionRssiSample(away, -59, 2048);
    detectionRssiSample(away, -63, 4096);
    ck("negative 8 dB is moving away", detectionRssiTrend(away) == RssiTrend::MOVING_AWAY);

    Detection learning = blankDetection();
    detectionRssiInit(learning, -65, 0);
    detectionRssiSample(learning, -60, 2048);
    ck("two samples still learning", detectionRssiTrend(learning) == RssiTrend::UNKNOWN);
    ck("unknown trend label is learning", std::strcmp(rssiTrendName(RssiTrend::UNKNOWN), "LEARNING") == 0);

    Detection ring = blankDetection();
    detectionRssiInit(ring, -80, 0);
    for (uint8_t i = 1; i <= 10; i++)
        detectionRssiSample(ring, (int8_t)(-80 + i), (uint32_t)i * 2048u);
    ck("RSSI history remains bounded at eight", ring.rssiHistCount == 8);
    ck("bounded ring drops oldest samples", detectionRssiAt(ring, 0) == -77);
    ck("bounded ring keeps newest sample", detectionRssiAt(ring, 7) == -70);

    Detection sameBucket = blankDetection();
    detectionRssiInit(sameBucket, -70, 0);
    detectionRssiSample(sameBucket, -50, 500);
    ck("chatty packets do not fill history", sameBucket.rssiHistCount == 1);
    ck("current RSSI still refreshes inside bucket", sameBucket.rssi == -50);

    return report();
}
