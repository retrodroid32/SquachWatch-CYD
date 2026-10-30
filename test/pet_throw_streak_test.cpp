#include "pet_throw_streak.h"
#include "test_util.h"
#include <stdint.h>

int main() {
    suite("STRIPES rolling pet-throw window");
    PetThrowStreak streak;
    for (unsigned i = 0; i < 9; i++)
        ck("first nine throws are not jail", !streak.note(i * 1000u));
    ck("ninth counts", streak.count() == 9);
    ck("tenth in one minute triggers sentence", streak.note(9000u));
    ck("jail streak clears after arrest", streak.count() == 0);
    ck("no immediate retrigger", !streak.note(9500u));

    streak.reset();
    for (unsigned i = 0; i < 9; i++)
        ck("slow throw not jail", !streak.note(i * 60000u));
    ck("60-second-old samples have expired", streak.count() == 1);
    ck("a late throw starts fresh count", !streak.note(9u * 60000u));

    streak.reset();
    for (unsigned i = 0; i < 9; i++)
        ck("boundary setup", !streak.note(i * 1000u));
    ck("at 59999 ms oldest remains in window", streak.note(59999u));

    streak.reset();
    for (unsigned i = 0; i < 9; i++)
        ck("boundary reset", !streak.note(i * 1000u));
    ck("at 60000 ms oldest is expired", !streak.note(60000u));
    ck("only nine active samples remain", streak.count() == 9);

    streak.reset();
    for (unsigned i = 0; i < 10; i++) {
        uint32_t t = UINT32_MAX - 4000u + i * 500u;
        bool done = streak.note(t);
        ck("millis rollover does not disrupt rolling window", done == (i == 9));
    }
    return report();
}
