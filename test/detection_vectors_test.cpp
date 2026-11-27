// Representative WiFi/BLE capture/signature vectors.
#include "signatures.h"
#include "remote_id.h"
#include "test_util.h"

static DetectionType oui(uint8_t a,uint8_t b,uint8_t c,Confidence* conf=nullptr) {
    const uint8_t mac[6]={a,b,c,0x11,0x22,0x33};
    return lookupOui(mac,conf);
}

int main() {
    suite("WiFi vectors");
    Confidence c=Confidence::LOW_CONF;
    ck("Flock registered OUI", oui(0xB4,0x1E,0x52,&c)==DetectionType::FLOCK);
    ck("Flock registered OUI high", c==Confidence::HIGH_CONF);
    ck("Wyze camera OUI", oui(0x2C,0xAA,0x8E,&c)==DetectionType::CAMERA);
    ck("Ring OUI", oui(0xAC,0x9F,0xC3,&c)==DetectionType::RING);
    ck("Axon setup SSID", lookupSsid("AB3-4F82")==DetectionType::AXON);
    ck("Pineapple SSID", lookupSsid("Pineapple_1337")==DetectionType::HACKER);

    suite("BLE vectors");
    ck("Ray-Ban Meta UUID", lookupUuid(0xFD5F)==DetectionType::META);
    ck("Tile UUID", lookupUuid(0xFEED)==DetectionType::TILE);
    ck("Samsung SmartTag UUID", lookupUuid(0xFD5A)==DetectionType::SAMSUNG_TAG);
    ck("Bare Remote ID UUID is not a drone", lookupUuid(0xFFFA)==DetectionType::UNKNOWN);
    ck("Flipper company ID", lookupMfgId(0x0E29)==DetectionType::HACKER);
    ck("Flock setup name", lookupBtName("Flock_Setup")==DetectionType::FLOCK);
    ck("HC-05 name", lookupBtName("HC-05")==DetectionType::SKIMMER);

    suite("Raw payload vectors");
    const uint8_t bareRid[] = {3, 0x03, 0xFA, 0xFF};
    const uint8_t shortRid[] = {5, 0x16, 0xFA, 0xFF, 0x0D, 0};
    const uint8_t framedRid[31] = {30, 0x16, 0xFA, 0xFF, 0x0D, 0, 0x02, 0x12, 0x41};
    ck("Bare UUID rejected by Remote ID parser", !RemoteId::isBluetoothLegacy(bareRid,sizeof bareRid));
    ck("Short Remote ID service data rejected", !RemoteId::isBluetoothLegacy(shortRid,sizeof shortRid));
    ck("Framed Remote ID accepted", RemoteId::isBluetoothLegacy(framedRid,sizeof framedRid));
    const uint8_t autel[] = {0xEC,0x5B,0xCD,0xE1,0x22,0x33};
    ck("Autel MA-M remains DRONE", lookupOui(autel)==DetectionType::DRONE);
    const uint8_t findMy[]={0x02,0x01,0x06,0x1E,0xFF,0x4C,0x00,0x12,0x19,0,1,2,3,4,5,6,7};
    ck("Find My manufacturer structure", isAirTagPayload(findMy,sizeof findMy));
    ck("separated Find My subtype is stronger evidence",
       isSeparatedFindMyPayload(findMy, sizeof findMy));
    const uint8_t applePairing[]={0x1E,0xFF,0x4C,0x00,0x07,0x19,0,0,0};
    ck("ambiguous Apple pairing advert is rejected, not misreported as AirTag",
       !isAirTagPayload(applePairing, sizeof applePairing));
    // v1.50: actual unpaired AirTag model 00:55 versus AirPods/other
    // Apple proximity-pairing frames sharing the same general prefix.
    const uint8_t realAirTagPairing[] = {0x1E,0xFF,0x4C,0x00,0x07,0x19,0x05,0x00,0x55,0x10,0x00};
    const uint8_t airPodsPairing[] = {0x1E,0xFF,0x4C,0x00,0x07,0x19,0x05,0x01,0x20,0x10,0x00};
    const uint8_t nearbyAirTag[] = {0x04,0xFF,0x4C,0x00,0x12,0x02,0xA5};
    ck("AirTag-specific unpaired model is detected",
       isAirTagPayload(realAirTagPairing, sizeof realAirTagPairing));
    ck("AirPods proximity pairing is not detected as an AirTag",
       !isAirTagPayload(airPodsPairing, sizeof airPodsPairing));
    ck("nearby-owner Find My frame is detected",
       isAirTagPayload(nearbyAirTag, sizeof nearbyAirTag));
    ck("ambiguous Apple pairing advert is not separated Find My",
       !isSeparatedFindMyPayload(applePairing, sizeof applePairing));
    ck("generic Eddystone UUID is not a Google tracker",
       lookupUuid(0xFEAA)==DetectionType::UNKNOWN);
    ck("Samsung tag specific UUID still matches",
       lookupUuid(0xFD5A)==DetectionType::SAMSUNG_TAG);
    const uint8_t ib[25]={0x4C,0,0x02,0x15,0xB9,0x40,0x7F,0x30,0xF5,0xF8,0x46,0x6E,0xAF,0xF9,0x25,0x55,0x6B,0x57,0xFE,0x6D,0,0x0A,0,0x2A,0xC5};
    ck("iBeacon block", isIBeacon(ib,sizeof ib));

    suite("Near misses");
    ck("random OUI", oui(0xDE,0xAD,0xBE)==DetectionType::UNKNOWN);
    ck("nearby UUID", lookupUuid(0xFEEE)==DetectionType::UNKNOWN);
    ck("ordinary SSID", lookupSsid("HomeWiFi")==DetectionType::UNKNOWN);
    return report();
}
