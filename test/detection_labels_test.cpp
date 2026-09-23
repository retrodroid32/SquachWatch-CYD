// Canonical human-facing detection labels.
#include "state.h"
#include "test_util.h"
#include <cstring>
#include <cstdio>

int main() {
    suite("Detection display labels");
    struct V { DetectionType t; const char* s; };
    const V v[] = {
        {DetectionType::FLOCK,"FLOCK CAM"},{DetectionType::AXON,"AXON BODY"},
        {DetectionType::META,"META GLASSES"},{DetectionType::SKIMMER,"CARD SKIMMER"},
        {DetectionType::RAVEN,"RAVEN"},{DetectionType::AIRTAG,"AIRTAG"},
        {DetectionType::DRONE,"DRONE"},{DetectionType::ALPR,"ALPR"},
        {DetectionType::CAMERA,"CAMERA"},{DetectionType::SAMSUNG_TAG,"SAMSUNG TAG"},
        {DetectionType::GOOGLE_TAG,"GOOGLE TAG"},{DetectionType::TILE,"TILE"},
        {DetectionType::RING,"RING CAM"},{DetectionType::DEAUTH,"DEAUTH"},
        {DetectionType::EVILTWIN,"EVIL TWIN AP"},{DetectionType::IBEACON,"PROXIMITY BEACON"},
        {DetectionType::HACKER,"HACKER HARDWARE"}
    };
    for (const auto& x : v) {
        char msg[96];
        std::snprintf(msg, sizeof msg, "%s display label", detectionTypeName(x.t));
        ck(msg, std::strcmp(detectionTypeDisplayName(x.t), x.s) == 0);
    }
    ck("UNKNOWN stays UNKNOWN", std::strcmp(detectionTypeDisplayName(DetectionType::UNKNOWN), "UNKNOWN") == 0);
    return report();
}
