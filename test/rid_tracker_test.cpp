// Multi-aircraft Remote ID record assembly: same real C++ used by firmware.
#include "rid_tracker.h"
#include "test_util.h"
#include <cstring>
#include <cstdint>

using RemoteId::Tracker;
static uint8_t ad[31];
static uint8_t msg[25];

static void basic(const char* serial) {
    memset(msg, 0, sizeof(msg));
    msg[0] = 0x02; // Basic ID, F3411 protocol v2
    msg[1] = 0x12; // serial ID type, quad
    size_t len = strlen(serial);
    if (len > 20) len = 20;
    memcpy(msg + 2, serial, len);
}
static void put32(uint8_t* p, int32_t v) {
    uint32_t u = (uint32_t)v;
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(u >> (8*i));
}
static void location(int32_t lat, int32_t lon) {
    memset(msg, 0, sizeof(msg));
    msg[0] = 0x12;
    put32(msg + 5, lat);
    put32(msg + 9, lon);
}
static const uint8_t* legacy() {
    ad[0] = 30; ad[1] = 0x16; ad[2] = 0xFA; ad[3] = 0xFF;
    ad[4] = 0x0D; ad[5] = 1;
    memcpy(ad + 6, msg, 25);
    return ad;
}
static const uint8_t* pack(uint8_t* out) {
    out[0] = 0xF2; out[1] = 25; out[2] = 1;
    memcpy(out + 3, msg, 25);
    return out;
}

int main() {
    const uint8_t a[6] = {0x02,1,2,3,4,5};
    const uint8_t b[6] = {0x02,1,2,3,4,6};
    Tracker t;

    suite("Two aircraft, interleaved BLE messages");
    basic("AIRCRAFT-A");
    ck("A Basic accepted", t.ingestBle(a, legacy(), sizeof(ad), 1000) != nullptr);
    basic("AIRCRAFT-B");
    ck("B Basic accepted", t.ingestBle(b, legacy(), sizeof(ad), 1001) != nullptr);
    location(400000001, -800000001);
    ck("A Location accepted", t.ingestBle(a, legacy(), sizeof(ad), 1002) != nullptr);
    location(500000002, -900000002);
    ck("B Location accepted", t.ingestBle(b, legacy(), sizeof(ad), 1003) != nullptr);
    const auto* ar = t.find(a, Tracker::BLE_LEGACY, 1003);
    const auto* br = t.find(b, Tracker::BLE_LEGACY, 1003);
    ck("both aircraft in table", t.liveCount(1003) == 2 && ar && br);
    ck("A preserved identity", ar && strcmp(ar->info.serial,"AIRCRAFT-A") == 0);
    ck("B preserved identity", br && strcmp(br->info.serial,"AIRCRAFT-B") == 0);
    ckf("A latitude remains 40 degrees", ar?ar->info.lat:0, 40.0f, 0.001f);
    ckf("B latitude remains 50 degrees", br?br->info.lat:0, 50.0f, 0.001f);
    ck("each independently counted two observations",
       ar && br && ar->observations == 2 && br->observations == 2);
    ck("latest snapshot is B", t.latest(1003) &&
       strcmp(t.latest(1003)->info.serial,"AIRCRAFT-B")==0);

    suite("Different transports do not fuse identical MACs");
    uint8_t wifi[28];
    basic("WIFI-A-IDENTITY");
    ck("Wi-Fi pack accepted", t.ingestWifiPack(a, pack(wifi), sizeof(wifi), 1004) != nullptr);
    const auto* ble = t.find(a, Tracker::BLE_LEGACY, 1004);
    const auto* wf  = t.find(a, Tracker::WIFI_BEACON, 1004);
    ck("same MAC has independent transport identities",
       t.liveCount(1004)==3 && ble && wf && ble != wf);
    ck("BLE serial remains unchanged", ble && strcmp(ble->info.serial,"AIRCRAFT-A")==0);
    ck("Wi-Fi serial independent", wf && strcmp(wf->info.serial,"WIFI-A-IDENTITY")==0);
    uint8_t bad[28] = {};
    ck("invalid pack cannot create a contact",
       t.ingestWifiPack(b, bad, sizeof(bad), 1005) == nullptr &&
       t.liveCount(1005)==3);
    ck("null BLE input does not create contact",
       t.ingestBle(b,nullptr,0,1005) == nullptr);

    suite("Bounded eight contacts with oldest eviction");
    Tracker full;
    for (unsigned i=0;i<Tracker::kCapacity+1;i++) {
        uint8_t mac[6] = {0x02,0xAA,0xBB,0xCC,0,(uint8_t)i};
        basic("TRACK");
        ck("contact accepted", full.ingestBle(mac,legacy(),sizeof(ad),100+i)!=nullptr);
    }
    uint8_t first[6]={0x02,0xAA,0xBB,0xCC,0,0};
    uint8_t last[6]={0x02,0xAA,0xBB,0xCC,0,8};
    ck("capacity never exceeded", full.liveCount(109)==Tracker::kCapacity);
    ck("oldest removed", full.find(first,Tracker::BLE_LEGACY,109)==nullptr);
    ck("newest retained", full.find(last,Tracker::BLE_LEGACY,109)!=nullptr);

    suite("Expiry and millis wraparound");
    Tracker rollover;
    basic("WRAP");
    rollover.ingestBle(a,legacy(),sizeof(ad),0xFFFFFFF0u);
    ck("wraparound age of 48ms remains fresh",
       rollover.find(a,Tracker::BLE_LEGACY,0x20u)!=nullptr);
    ck("contact expired after 30 seconds",
       rollover.liveCount(31000u)==0);
    Tracker expired;
    basic("FIRST");
    expired.ingestBle(a,legacy(),sizeof(ad),100);
    location(450000000,-700000000);
    expired.ingestBle(a,legacy(),sizeof(ad),101);
    basic("NEW");
    const auto* fresh = expired.ingestBle(a,legacy(),sizeof(ad),40101);
    ck("expired record reused as fresh aircraft",
       fresh && strcmp(fresh->info.serial,"NEW")==0 && !fresh->info.haveLoc
       && fresh->observations==1);

    return report();
}
