// The signature tables themselves.
//
// These are hand-maintained lists of hardware addresses, and an audit
// against the IEEE registry found the table saying things that were not
// true: one prefix attributed to Vigilant belonged to Sonos, another to
// "Sierra" belonged to a company called SPECTRA - TEK, one labelled
// Hikvision belonged to Amazon, and a Verkada block had been added twice.
// None of that fails to compile and none of it fails at runtime -- it just
// makes the device confidently wrong.
//
// This cannot re-check the registry (CI has no business fetching 40,000
// rows on every push), so it checks the properties that hold regardless
// of what the registry says, plus a few anchors from the audit.
#include "signatures.h"
#include "test_util.h"
#include <cstring>

static DetectionType lookup(uint8_t a, uint8_t b, uint8_t c, Confidence* conf) {
    const uint8_t mac[6] = { a, b, c, 0x11, 0x22, 0x33 };
    return lookupOui(mac, conf);
}

static DetectionType lookup4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, Confidence* conf) {
    const uint8_t mac[6] = { a, b, c, d, 0x22, 0x33 };
    return lookupOui(mac, conf);
}

int main() {
    suite("Every OUI row is well formed");

    int locallyAdministered = 0, gradedAboveLow = 0;
    for (uint16_t i = 0; i < kOuiCount; i++) {
        // Bit 1 of the first octet means locally administered: a randomised
        // or hand-assigned address, which by definition identifies no
        // vendor. Such a row may exist as a hint, but it can never be
        // strong evidence.
        if (kOuiTable[i].b[0] & 0x02) {
            locallyAdministered++;
            if (kOuiTable[i].conf != Confidence::LOW_CONF) gradedAboveLow++;
        }
    }
    char msg[96];
    snprintf(msg, sizeof(msg), "%d locally-administered rows, none graded above LOW",
             locallyAdministered);
    ck(msg, gradedAboveLow == 0);

    int dupes = 0;
    for (uint16_t i = 0; i < kOuiCount; i++)
        for (uint16_t j = (uint16_t)(i + 1); j < kOuiCount; j++)
            if (memcmp(kOuiTable[i].b, kOuiTable[j].b, 3) == 0) dupes++;
    // A duplicate is not merely untidy: lookupOui returns on the first hit,
    // so the second row is dead code that reads as a maintained signature.
    ck("no duplicate prefixes", dupes == 0);

    int mamDupes = 0;
    for (uint16_t i = 0; i < kOuiMamCount; i++)
        for (uint16_t j = (uint16_t)(i + 1); j < kOuiMamCount; j++)
            if (memcmp(kOuiMamTable[i].b, kOuiMamTable[j].b, 3) == 0 &&
                kOuiMamTable[i].nibble == kOuiMamTable[j].nibble) mamDupes++;
    ck("no duplicate 28-bit prefixes", mamDupes == 0);

    int emptyLabel = 0, unknownType = 0;
    for (uint16_t i = 0; i < kOuiCount; i++) {
        if (!kOuiTable[i].name || !kOuiTable[i].name[0]) emptyLabel++;
        if (kOuiTable[i].type == DetectionType::UNKNOWN) unknownType++;
        // vendor[12] in Detection, so 11 characters plus a terminator.
        if (kOuiTable[i].name && strlen(kOuiTable[i].name) > 11) emptyLabel++;
    }
    for (uint16_t i = 0; i < kOuiMamCount; i++) {
        if (!kOuiMamTable[i].name || !kOuiMamTable[i].name[0]) emptyLabel++;
        if (kOuiMamTable[i].type == DetectionType::UNKNOWN) unknownType++;
        if (kOuiMamTable[i].name && strlen(kOuiMamTable[i].name) > 11) emptyLabel++;
        if (kOuiMamTable[i].nibble > 0x0F) unknownType++;
    }
    ck("every row has a label that fits Detection::vendor", emptyLabel == 0);
    ck("no row matches to UNKNOWN", unknownType == 0);

    suite("lookupOui hands back the row's own grade");

    Confidence conf = Confidence::LOW_CONF;

    // B4:1E:52 is registered to Flock Safety themselves -- the only prefix
    // in the whole FLOCK block that is.
    ck("Flock Safety's own block is FLOCK",
       lookup(0xB4, 0x1E, 0x52, &conf) == DetectionType::FLOCK);
    ck("...and graded HIGH", conf == Confidence::HIGH_CONF);

    suite("Flock field-tested WiFi prefixes");

    static const uint8_t flockField[][3] = {
        {0x70,0xC9,0x4E},{0x3C,0x91,0x80},{0xD8,0xF3,0xBC},{0x80,0x30,0x49},
        {0xB8,0x35,0x32},{0x14,0x5A,0xFC},{0x74,0x4C,0xA1},{0x08,0x3A,0x88},
        {0x9C,0x2F,0x9D},{0xC0,0x35,0x32},{0x94,0x08,0x53},{0xE4,0xAA,0xEA},
        {0xF4,0x6A,0xDD},{0xE0,0x0A,0xF6},{0x24,0xB2,0xB9},{0x00,0xF4,0x8D},
        {0xD0,0x39,0x57},{0xE8,0xD0,0xFC},{0xE0,0x4F,0x43},{0xB8,0x1E,0xA4},
        {0x70,0x08,0x94},{0x58,0x8E,0x81},{0xEC,0x1B,0xBD},{0x3C,0x71,0xBF},
        {0x58,0x00,0xE3},{0x90,0x35,0xEA},{0x5C,0x93,0xA2},{0x64,0x6E,0x69},
        {0x48,0x27,0xEA},{0xA4,0xCF,0x12},{0x14,0xB5,0xCD},{0x82,0x6B,0xF2},
    };
    int flockMisses = 0, flockOvergraded = 0;
    for (size_t i = 0; i < sizeof(flockField) / sizeof(flockField[0]); i++) {
        conf = Confidence::HIGH_CONF;
        if (lookup(flockField[i][0], flockField[i][1], flockField[i][2], &conf) != DetectionType::FLOCK)
            flockMisses++;
        if (conf != Confidence::LOW_CONF) flockOvergraded++;
    }
    ck("all 32 active field prefixes match FLOCK", flockMisses == 0);
    ck("all community field prefixes remain LOW", flockOvergraded == 0);

    // Old generic ESP32 guesses are deliberately gone unless they are in the
    // current field list. They produced predictable false positives on ordinary
    // Espressif hardware and are not part of the 2026-07-16 Flock-You set.
    ck("retired generic ESP32 guess no longer matches",
       lookup(0x24, 0x0A, 0xC4, &conf) == DetectionType::UNKNOWN);
    ck("retired SPECTRA-TEK guess no longer matches",
       lookup(0x00, 0xA0, 0xD8, &conf) == DetectionType::UNKNOWN);

    suite("Flock firmware-derived BLE names");
    ck("Penguin + 10 digits matches FLOCK",
       lookupBtName("Penguin-1234567890") == DetectionType::FLOCK);
    ck("bare 10-digit Penguin serial matches FLOCK",
       lookupBtName("1234567890") == DetectionType::FLOCK);
    ck("non-digit Penguin suffix does not match",
       lookupBtName("Penguin-12345ABCDE") == DetectionType::UNKNOWN);
    ck("random 10-char name does not match",
       lookupBtName("12345abcde") == DetectionType::UNKNOWN);

    suite("Flock wildcard-probe corroboration");

    const uint8_t qca[6] = {0x00,0x03,0x7F,0x50,0x00,0x01};
    ck("firmware-derived QCA prefix is recognized only by helper",
       isFlockFirmwareWifiPrefix(qca));
    conf = Confidence::HIGH_CONF;
    ck("generic QCA prefix is not a blanket FLOCK OUI",
       lookupOui(qca, &conf) == DetectionType::UNKNOWN);

    uint8_t probe[32] = {};
    probe[0] = 0x40;            // management / probe request
    probe[24] = 0x00;           // SSID IE
    probe[25] = 0x00;           // wildcard: empty SSID
    ck("empty-SSID probe is recognized",
       isWifiWildcardProbe(probe, 26));

    probe[25] = 0x03;
    probe[26] = 'a'; probe[27] = 'b'; probe[28] = 'c';
    ck("directed probe is not wildcard",
       !isWifiWildcardProbe(probe, 29));

    probe[0] = 0x80;            // beacon, not probe request
    probe[25] = 0x00;
    ck("empty SSID in a beacon is not a wildcard probe",
       !isWifiWildcardProbe(probe, 26));

    // Malformed/truncated IE must fail closed rather than reading past input.
    probe[0] = 0x40;
    probe[24] = 0xDD;
    probe[25] = 10;
    ck("truncated probe IE fails closed",
       !isWifiWildcardProbe(probe, 28));

    suite("Drone manufacturer OUIs");

    ck("DJI current MA-L block is DRONE",
       lookup(0x4C, 0x43, 0xF6, &conf) == DetectionType::DRONE);
    ck("DJI block graded HIGH", conf == Confidence::HIGH_CONF);
    ck("Parrot is DRONE",
       lookup(0x90, 0x3A, 0xE6, &conf) == DetectionType::DRONE);
    ck("Skydio is DRONE",
       lookup(0x38, 0x1D, 0x14, &conf) == DetectionType::DRONE);
    ck("Teal is DRONE",
       lookup(0xB0, 0x30, 0xC8, &conf) == DetectionType::DRONE);
    ck("Freefly is DRONE",
       lookup(0xEC, 0x71, 0x5E, &conf) == DetectionType::DRONE);
    ck("AeroVironment is DRONE",
       lookup(0x00, 0x1A, 0xF9, &conf) == DetectionType::DRONE);
    ck("PowerVision is DRONE",
       lookup(0x54, 0x7D, 0x40, &conf) == DetectionType::DRONE);
    ck("Zipline is DRONE",
       lookup(0x74, 0xB8, 0x0F, &conf) == DetectionType::DRONE);

    // The next three share their first 24 bits with unrelated companies.
    // Exact high-nibble matching is therefore part of correctness, not just
    // an optimization.
    ck("Autel MA-M exact /28 matches",
       lookup4(0xEC, 0x5B, 0xCD, 0xE1, &conf) == DetectionType::DRONE);
    ck("Autel neighboring /28 does not match",
       lookup4(0xEC, 0x5B, 0xCD, 0xD1, &conf) == DetectionType::UNKNOWN);
    ck("Hubsan MA-M exact /28 matches",
       lookup4(0x98, 0xAA, 0xFC, 0x71, &conf) == DetectionType::DRONE);
    ck("Hubsan neighboring /28 does not match",
       lookup4(0x98, 0xAA, 0xFC, 0x61, &conf) == DetectionType::UNKNOWN);
    ck("Yuneec MA-M exact /28 matches",
       lookup4(0xE0, 0xB6, 0xF5, 0x81, &conf) == DetectionType::DRONE);
    ck("Yuneec neighboring /28 does not match",
       lookup4(0xE0, 0xB6, 0xF5, 0x91, &conf) == DetectionType::UNKNOWN);
    ck("Quantum Systems MA-M matches",
       lookup4(0xAC, 0x86, 0xD1, 0x71, &conf) == DetectionType::DRONE);
    ck("Inspired Flight MA-M matches",
       lookup4(0x34, 0xB5, 0xF3, 0x21, &conf) == DetectionType::DRONE);
    ck("FIMI MA-M exact /28 matches",
       lookup4(0x6C, 0xDF, 0xFB, 0xE1, &conf) == DetectionType::DRONE);
    ck("FIMI neighboring /28 does not match",
       lookup4(0x6C, 0xDF, 0xFB, 0xD1, &conf) == DetectionType::UNKNOWN);
    ck("HOVERAir / Zero Zero MA-M exact /28 matches",
       lookup4(0xC8, 0x63, 0x14, 0x41, &conf) == DetectionType::DRONE);
    ck("Zero Zero neighboring /28 does not match",
       lookup4(0xC8, 0x63, 0x14, 0x31, &conf) == DetectionType::UNKNOWN);

    const uint8_t autelMac[6] = {0xEC,0x5B,0xCD,0xE1,0,0};
    ck("MA-M vendor label resolves",
       std::strcmp(ouiVendorName(autelMac), "Autel") == 0);

    // Ring LLC's own registration versus Amazon's umbrella block, which
    // also covers Echo, Fire TV and Kindle.
    ck("Ring's own block is RING", lookup(0xAC, 0x9F, 0xC3, &conf) == DetectionType::RING);
    ck("...and graded HIGH", conf == Confidence::HIGH_CONF);
    ck("Amazon's umbrella block is RING", lookup(0xFC, 0x65, 0xDE, &conf) == DetectionType::RING);
    ck("...and graded MED", conf == Confidence::MED_CONF);

    suite("A miss leaves the caller's value alone");
    // The WiFi path seeds conf from the type before asking, so lookupOui
    // must not stamp on it when nothing matches.
    conf = Confidence::MED_CONF;
    ck("no match returns UNKNOWN", lookup(0xDE, 0xAD, 0xBE, &conf) == DetectionType::UNKNOWN);
    ck("...and does not touch conf", conf == Confidence::MED_CONF);
    ck("null mac is safe", lookupOui(nullptr, &conf) == DetectionType::UNKNOWN);
    ck("the one-argument form still compiles and works",
       lookupOui((const uint8_t[]){ 0xB4, 0x1E, 0x52, 0, 0, 0 }) == DetectionType::FLOCK);

    suite("Sonos is not a licence plate reader");
    // 00:0E:58 sat in the ALPR block as "Vigilant" for eleven releases. It
    // is registered to Sonos, Inc. and was removed; this is here so it
    // cannot come back by copy-paste from an older detector.
    ck("00:0E:58 no longer matches anything",
       lookup(0x00, 0x0E, 0x58, &conf) == DetectionType::UNKNOWN);

    return report();
}
