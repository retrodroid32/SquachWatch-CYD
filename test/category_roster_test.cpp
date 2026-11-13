// Category pills are presentation groups, not new detection identities.
#include "category_roster.h"
#include "test_util.h"
#include <cstring>

int main() {
    suite("Category roster: every underlying device retains its real type");
    ck("AirTag in tracker", CategoryRoster::matches(DetectionType::AIRTAG, DetectionType::AIRTAG));
    ck("Tile in tracker", CategoryRoster::matches(DetectionType::AIRTAG, DetectionType::TILE));
    ck("Google tag in tracker", CategoryRoster::matches(DetectionType::AIRTAG, DetectionType::GOOGLE_TAG));
    ck("Samsung in tracker", CategoryRoster::matches(DetectionType::AIRTAG, DetectionType::SAMSUNG_TAG));
    ck("Tile not Flipper", !CategoryRoster::matches(DetectionType::HACKER, DetectionType::TILE));
    ck("Ring in camera", CategoryRoster::matches(DetectionType::CAMERA, DetectionType::RING));
    ck("Ring not Axon", !CategoryRoster::matches(DetectionType::AXON, DetectionType::RING));
    ck("Evil twin in hacker", CategoryRoster::matches(DetectionType::HACKER, DetectionType::EVILTWIN));
    ck("Hacker in hacker", CategoryRoster::matches(DetectionType::HACKER, DetectionType::HACKER));
    ck("Unknown never matches", !CategoryRoster::matches(DetectionType::UNKNOWN, DetectionType::UNKNOWN));
    ck("display tracker label", strcmp(CategoryRoster::label(DetectionType::AIRTAG), "TRACKER") == 0);

    // For each real detection there must be exactly one of the 12 main
    // screen pills, even where one of the types is grouped.
    const DetectionType categories[] = {
        DetectionType::FLOCK, DetectionType::AXON, DetectionType::META,
        DetectionType::SKIMMER, DetectionType::RAVEN, DetectionType::AIRTAG,
        DetectionType::DRONE, DetectionType::ALPR, DetectionType::CAMERA,
        DetectionType::HACKER, DetectionType::DEAUTH, DetectionType::IBEACON
    };
    for (uint8_t ty = 1; ty < (uint8_t)DetectionType::COUNT; ++ty) {
        int matches = 0;
        for (DetectionType c : categories)
            if (CategoryRoster::matches(c, (DetectionType)ty)) matches++;
        ck("every actual detection maps to exactly one category", matches == 1);
    }
    return report();
}
