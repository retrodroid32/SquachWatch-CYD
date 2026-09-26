// SquachWatch-CYD — per-type alert rule defaults, persistence and validation.
#include "test_util.h"
#include "settings.h"
#include "theme.h"
#include "clock.h"
#include <Preferences.h>
#include <cstdlib>
#include <filesystem>
#include <cstring>

namespace Theme { void applyPalette(uint8_t) {} }
namespace Clock {
uint8_t     zoneCount()       { return 1; }
const char* zoneName(uint8_t) { return "UTC"; }
void        applyZone(uint8_t) {}
}

int main() {
    const char* dir = "alert_rules_test_nvs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directory(dir);
    setenv("SQUACHSIM_NVS", dir, 1);

    suite("Compact defaults preserve existing behavior");
    Settings::load();
    ck("rule record stays four bytes", sizeof(Settings::AlertRule) == 4);
    ck("full alert defaults on", Settings::alertEnabled(DetectionType::FLOCK));
    ck("wake defaults on", Settings::alertWakeScreen(DetectionType::FLOCK));
    ck("confidence inherits global filter", Settings::alertConfidenceInherited(DetectionType::FLOCK));
    ck("default repeat count is one", Settings::alertMinRepeats(DetectionType::FLOCK) == 1);
    ck("default cooldown is off", Settings::alertCooldownSec(DetectionType::FLOCK) == 0);

    suite("Inherited and explicit confidence stay distinct");
    Settings::cycleMinConfidence(); // global LOW -> MED
    ck("inherited rule follows global MED",
       Settings::alertMinConfidence(DetectionType::AIRTAG) == Confidence::MED_CONF);
    Settings::cycleAlertMinConfidence(DetectionType::FLOCK); // GLOBAL -> explicit LOW
    ck("FLOCK is now explicit", !Settings::alertConfidenceInherited(DetectionType::FLOCK));
    ck("explicit FLOCK is LOW",
       Settings::alertMinConfidence(DetectionType::FLOCK) == Confidence::LOW_CONF);
    Settings::cycleMinConfidence(); // global MED -> HIGH
    ck("explicit FLOCK ignores later global changes",
       Settings::alertMinConfidence(DetectionType::FLOCK) == Confidence::LOW_CONF);
    ck("inherited AIRTAG follows global HIGH",
       Settings::alertMinConfidence(DetectionType::AIRTAG) == Confidence::HIGH_CONF);

    suite("Rule changes persist as one compact blob");
    Settings::toggleAlertEnabled(DetectionType::FLOCK);
    Settings::toggleAlertWakeScreen(DetectionType::FLOCK);
    Settings::cycleAlertMinRepeats(DetectionType::FLOCK); // 1 -> 2
    Settings::cycleAlertCooldown(DetectionType::FLOCK);   // off -> 30 sec

    Preferences p;
    p.begin("settings", false);
    ck("one exact-size alert-rule blob is stored",
       p.getBytesLength("alrules") ==
       sizeof(Settings::AlertRule) * (uint8_t)DetectionType::COUNT);

    Settings::load();
    ck("log-only survives reload", !Settings::alertEnabled(DetectionType::FLOCK));
    ck("wake off survives reload", !Settings::alertWakeScreen(DetectionType::FLOCK));
    ck("explicit confidence survives reload", !Settings::alertConfidenceInherited(DetectionType::FLOCK));
    ck("repeat threshold survives reload", Settings::alertMinRepeats(DetectionType::FLOCK) == 2);
    ck("cooldown survives reload", Settings::alertCooldownSec(DetectionType::FLOCK) == 30);

    suite("Supported repeat and cooldown choices wrap");
    Settings::cycleAlertMinRepeats(DetectionType::RING); // 1 -> 2
    Settings::cycleAlertMinRepeats(DetectionType::RING); // 2 -> 3
    Settings::cycleAlertMinRepeats(DetectionType::RING); // 3 -> 5
    ck("repeat reaches five", Settings::alertMinRepeats(DetectionType::RING) == 5);
    Settings::cycleAlertMinRepeats(DetectionType::RING); // 5 -> 1
    ck("repeat wraps to one", Settings::alertMinRepeats(DetectionType::RING) == 1);

    Settings::cycleAlertCooldown(DetectionType::RING); // 30
    Settings::cycleAlertCooldown(DetectionType::RING); // 60
    Settings::cycleAlertCooldown(DetectionType::RING); // 300
    Settings::cycleAlertCooldown(DetectionType::RING); // 900
    ck("cooldown reaches fifteen minutes",
       Settings::alertCooldownSec(DetectionType::RING) == 900);
    Settings::cycleAlertCooldown(DetectionType::RING); // off
    ck("cooldown wraps off", Settings::alertCooldownSec(DetectionType::RING) == 0);

    suite("Corrupt persisted values are clamped on load");
    Settings::AlertRule bad[(uint8_t)DetectionType::COUNT];
    memset(bad, 0, sizeof bad);
    bad[(uint8_t)DetectionType::FLOCK].flags = 0xFF;
    bad[(uint8_t)DetectionType::FLOCK].minConf = 42;
    bad[(uint8_t)DetectionType::FLOCK].minRepeats = 4;
    bad[(uint8_t)DetectionType::FLOCK].cooldownIx = 99;
    p.putBytes("alrules", bad, sizeof bad);
    Settings::load();
    ck("unknown flag bits are stripped",
       Settings::alertEnabled(DetectionType::FLOCK) &&
       Settings::alertWakeScreen(DetectionType::FLOCK));
    ck("bad confidence falls back to GLOBAL",
       Settings::alertConfidenceInherited(DetectionType::FLOCK));
    ck("bad repeat falls back to one",
       Settings::alertMinRepeats(DetectionType::FLOCK) == 1);
    ck("bad cooldown falls back off",
       Settings::alertCooldownSec(DetectionType::FLOCK) == 0);

    std::filesystem::remove_all(dir);
    return report();
}
