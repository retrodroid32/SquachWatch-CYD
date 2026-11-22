#include "ble_adv_utils.h"
#include "test_util.h"
#include <cstring>

int main() {
    suite("BLE MAC canonical order");
    const uint8_t nimble[6] = {0xC4,0x45,0xCF,0xC1,0xC5,0x5E};
    uint8_t mac[6] = {};
    BleAdv::canonicalMacFromNimble(nimble, mac);
    const uint8_t expect[6] = {0x5E,0xC5,0xC1,0xCF,0x45,0xC4};
    ck("native bytes become conventional display order", std::memcmp(mac, expect, 6) == 0);

    suite("BLE local-name parsing");
    char name[24];

    // Flags + Apple company ID / Nearby Info-like manufacturer data, no
    // local-name AD field. This is the false-positive shape reported in the
    // field: it must remain nameless rather than inheriting stale memory.
    const uint8_t appleNoName[] = {
        2, 0x01, 0x06,
        7, 0xFF, 0x4C, 0x00, 0x10, 0x02, 0x01, 0x00
    };
    BleAdv::copyLocalName(appleNoName, sizeof appleNoName, name, sizeof name);
    ck("nameless Apple advert stays empty", name[0] == '\0');

    const uint8_t shortName[] = {4, 0x08, 'A','B','C'};
    BleAdv::copyLocalName(shortName, sizeof shortName, name, sizeof name);
    ck("shortened local name is copied", std::strcmp(name, "ABC") == 0);

    const uint8_t both[] = {
        4, 0x08, 'O','L','D',
        5, 0x09, 'F','U','L','L'
    };
    BleAdv::copyLocalName(both, sizeof both, name, sizeof name);
    ck("complete name wins over shortened name", std::strcmp(name, "FULL") == 0);

    suite("Indexed BLE manufacturer fields (scan response appended)");
    // Each valid AD structure starts with a count byte and a type byte.
    // The second manufacturer field models NimBLE's assembled scan response:
    // 0x0E29 is the Flipper company ID, and must be reachable even if
    // an unrelated (or malformed short) field was advertised first.
    const uint8_t twoMfg[] = {
        2, 0x01, 0x06,
        4, 0xFF, 0x99, 0x99, 0x01,
        4, 0xFF, 0x29, 0x0E, 0x02
    };
    const uint8_t* field = nullptr;
    size_t fieldSize = 0;
    ck("first manufacturer field found",
       BleAdv::nthField(twoMfg, sizeof twoMfg, 0xFF, 0, field, fieldSize) &&
       fieldSize == 3 && field[0] == 0x99 && field[1] == 0x99);
    ck("second manufacturer field found",
       BleAdv::nthField(twoMfg, sizeof twoMfg, 0xFF, 1, field, fieldSize) &&
       fieldSize == 3 && field[0] == 0x29 && field[1] == 0x0E);
    ck("third manufacturer absent",
       !BleAdv::nthField(twoMfg, sizeof twoMfg, 0xFF, 2, field, fieldSize));
    const uint8_t zeroAndShort[] = {
        0, 2, 0x01, 0x06, 1, 0xFF, 3, 0xFF, 0x29, 0x0E
    };
    ck("zero-length padding does not hide second manufacturer",
       BleAdv::nthField(zeroAndShort, sizeof zeroAndShort, 0xFF, 1, field, fieldSize) &&
       fieldSize == 2 && field[0] == 0x29 && field[1] == 0x0E);
    const uint8_t malformed[] = { 2, 0x01, 0x06, 9, 0xFF, 0x29 };
    ck("truncated manufacturer field is rejected",
       !BleAdv::nthField(malformed, sizeof malformed, 0xFF, 0, field, fieldSize));
    // Name matching remains independent and only uses the earliest name.
    const uint8_t nameWithMfg[] = {4, 0x09, 'A','B','C', 3, 0xFF, 0x29, 0x0E};
    BleAdv::copyLocalName(nameWithMfg, sizeof nameWithMfg, name, sizeof name);
    ck("name still parsed with adjacent manufacturer data",
       std::strcmp(name, "ABC") == 0);

    const uint8_t truncated[] = {5, 0x09, 'B','A'};
    BleAdv::copyLocalName(truncated, sizeof truncated, name, sizeof name);
    ck("truncated field fails closed", name[0] == '\0');

    return report();
}
