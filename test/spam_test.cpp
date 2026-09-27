// SquachWatch-CYD — tracker-only spam flood rules.
#include "test_util.h"
#include "spam_watch.h"

int main() {
    suite("Eligible tracker classes only");
    ck("AirTag eligible", SpamWatch::eligible(DetectionType::AIRTAG));
    ck("Samsung tag eligible", SpamWatch::eligible(DetectionType::SAMSUNG_TAG));
    ck("Google tag eligible", SpamWatch::eligible(DetectionType::GOOGLE_TAG));
    ck("Tile eligible", SpamWatch::eligible(DetectionType::TILE));
    ck("iBeacon not eligible", !SpamWatch::eligible(DetectionType::IBEACON));
    ck("generic BLE types not eligible", !SpamWatch::eligible(DetectionType::META));

    suite("Short one-visit identities accumulate");
    {
        SpamWatch w;
        uint32_t now = 1000000;
        bool tripped = false;
        for (int i = 0; i < 7; i++) { now += 5000; tripped |= w.noteVanish(DetectionType::AIRTAG, 1500, 1, now); }
        ck("seven do not trip", !tripped && !w.active(DetectionType::AIRTAG, now));
        now += 5000;
        ck("eighth trips", w.noteVanish(DetectionType::AIRTAG, 1500, 1, now));
        ck("flood active", w.active(DetectionType::AIRTAG, now));
        ck("eight counted", w.fakes(DetectionType::AIRTAG) == 8);
    }

    suite("Recurring or long-lived trackers are not evidence");
    {
        SpamWatch w;
        uint32_t now = 2000000;
        for (int i = 0; i < 20; i++) {
            now += 2000;
            w.noteVanish(DetectionType::AIRTAG, 30000, 1, now);
            w.noteVanish(DetectionType::AIRTAG, 1000, 2, now);
        }
        ck("no flood", !w.active(DetectionType::AIRTAG, now));
    }

    suite("One alert per flood");
    {
        SpamWatch w;
        uint32_t now = 3000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(DetectionType::GOOGLE_TAG, 800, 1, now); }
        ck("first announcement allowed", w.takeAnnounce(DetectionType::GOOGLE_TAG));
        ck("second suppressed", !w.takeAnnounce(DetectionType::GOOGLE_TAG));
        ck("other tracker type unaffected", !w.active(DetectionType::TILE, now));
    }

    suite("Fast flood trips on new identity count");
    {
        SpamWatch w;
        ck("39 new identities is not enough", !w.noteBurst(DetectionType::TILE, 39, 4000000));
        ck("40 trips", w.noteBurst(DetectionType::TILE, 40, 4060000));
        ck("camera burst ignored", !w.noteBurst(DetectionType::CAMERA, 200, 4060000));
    }

    suite("Flood clears after quiet period");
    {
        SpamWatch w;
        uint32_t now = 5000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(DetectionType::SAMSUNG_TAG, 800, 1, now); }
        ck("still active exactly at quiet threshold", w.active(DetectionType::SAMSUNG_TAG, now + SpamWatch::QUIET_MS));
        ck("clears just after threshold", !w.active(DetectionType::SAMSUNG_TAG, now + SpamWatch::QUIET_MS + 1));
        ck("announcement resets with next flood", !w.takeAnnounce(DetectionType::SAMSUNG_TAG));
    }

    suite("New evidence after quiet starts a fresh flood");
    {
        SpamWatch w;
        uint32_t now = 6000000;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(DetectionType::AIRTAG, 800, 1, now); }
        ck("first flood announces", w.takeAnnounce(DetectionType::AIRTAG));
        now += SpamWatch::QUIET_MS + 1;
        for (int i = 0; i < 8; i++) { now += 3000; w.noteVanish(DetectionType::AIRTAG, 800, 1, now); }
        ck("new evidence can trip a new flood without an active() poll",
           w.active(DetectionType::AIRTAG, now) && w.takeAnnounce(DetectionType::AIRTAG));
    }

    suite("Old strays decay instead of accumulating forever");
    {
        SpamWatch w;
        uint32_t now = 7000000;
        for (int i = 0; i < 12; i++) { now += 4 * 60000; w.noteVanish(DetectionType::AIRTAG, 1000, 1, now); }
        ck("one stray every four minutes never trips", !w.active(DetectionType::AIRTAG, now));
    }

    return report();
}
