// SquachWatch-CYD — persistent per-device alert policies
#include "ignore_list.h"
#include <Preferences.h>
#include <string.h>

namespace IgnoreList {

struct __attribute__((packed)) PolicyRec {
    uint8_t mac[6];
    uint8_t type;
    uint8_t policy;
};
static_assert(sizeof(PolicyRec) == 8, "device policy record must remain 8 bytes");

static Preferences s_prefs;
static bool        s_loaded = false;
static PolicyRec   s_rec[MAX];
static uint8_t     s_count = 0;

static const char* NS        = "ignore";
static const char* KEY       = "dev2";   // current 8-byte policy records
static const char* KEY_TYPED = "dev";    // v1.20.x: 7-byte MAC + type
static const char* KEY_OLD   = "macs";   // oldest: 6-byte MAC only

static bool validPolicy(uint8_t p) {
    return p >= (uint8_t)Policy::IGNORE && p <= (uint8_t)Policy::ALWAYS_ALERT;
}

static bool saveCurrent() {
    if (s_count == 0) {
        // putBytes(0) is a no-op on the real Preferences API. Removing the key
        // is the only way an empty list really survives a reboot.
        return s_prefs.remove(KEY);
    }
    const size_t want = (size_t)s_count * sizeof(PolicyRec);
    return s_prefs.putBytes(KEY, s_rec, want) == want;
}

static int indexOf(const uint8_t* mac) {
    if (!mac) return -1;
    for (uint8_t i = 0; i < s_count; i++)
        if (memcmp(s_rec[i].mac, mac, 6) == 0) return (int)i;
    return -1;
}

static void sanitizeLoaded() {
    bool dirty = false;
    for (uint8_t i = 0; i < s_count; i++) {
        if (s_rec[i].type >= (uint8_t)DetectionType::COUNT) {
            s_rec[i].type = (uint8_t)DetectionType::UNKNOWN;
            dirty = true;
        }
        // NORMAL is never stored; an unknown/corrupt policy fails safe to the
        // historical behavior (IGNORE) rather than unexpectedly interrupting.
        if (!validPolicy(s_rec[i].policy)) {
            s_rec[i].policy = (uint8_t)Policy::IGNORE;
            dirty = true;
        }
    }
    if (dirty) (void)saveCurrent();
}

void begin() {
    if (s_loaded) return;
    s_prefs.begin(NS, false);
    s_count = 0;

    // Current format is authoritative when it is structurally valid.
    size_t len = s_prefs.getBytesLength(KEY);
    if (len >= sizeof(PolicyRec) && (len % sizeof(PolicyRec)) == 0) {
        if (len > sizeof(s_rec)) len = sizeof(s_rec);
        const size_t got = s_prefs.getBytes(KEY, s_rec, len);
        if (got == len && (got % sizeof(PolicyRec)) == 0) {
            s_count = (uint8_t)(got / sizeof(PolicyRec));
            sanitizeLoaded();
            // Clean up leftovers from an interrupted older migration only
            // after the current blob has been read completely. A short read
            // leaves the legacy source intact so the next boot still has a
            // recoverable copy.
            s_prefs.remove(KEY_TYPED);
            s_prefs.remove(KEY_OLD);
            s_loaded = true;
            return;
        }
        // Treat a short/corrupt read as unusable current data and fall through
        // to the legacy formats below. Do not delete anything here.
        s_count = 0;
    }

    // v1.20.x: MAC[6] + DetectionType. Every old entry was an IGNORE.
    size_t old = s_prefs.getBytesLength(KEY_TYPED);
    if (old >= 7) {
        uint8_t tmp[MAX * 7];
        if (old > sizeof(tmp)) old = sizeof(tmp);
        const size_t got = s_prefs.getBytes(KEY_TYPED, tmp, old);
        s_count = (uint8_t)(got / 7u);
        for (uint8_t i = 0; i < s_count; i++) {
            memcpy(s_rec[i].mac, &tmp[(size_t)i * 7u], 6);
            const uint8_t ty = tmp[(size_t)i * 7u + 6];
            s_rec[i].type = ty < (uint8_t)DetectionType::COUNT
                          ? ty : (uint8_t)DetectionType::UNKNOWN;
            s_rec[i].policy = (uint8_t)Policy::IGNORE;
        }
        // Critical migration rule: never destroy the source until the new blob
        // has actually been written in full.
        if (saveCurrent()) {
            s_prefs.remove(KEY_TYPED);
            s_prefs.remove(KEY_OLD);
        }
        s_loaded = true;
        return;
    }

    // Pre-type format: MACs only. Same verified-write rule.
    old = s_prefs.getBytesLength(KEY_OLD);
    if (old >= 6) {
        uint8_t tmp[MAX * 6];
        if (old > sizeof(tmp)) old = sizeof(tmp);
        const size_t got = s_prefs.getBytes(KEY_OLD, tmp, old);
        s_count = (uint8_t)(got / 6u);
        for (uint8_t i = 0; i < s_count; i++) {
            memcpy(s_rec[i].mac, &tmp[(size_t)i * 6u], 6);
            s_rec[i].type = (uint8_t)DetectionType::UNKNOWN;
            s_rec[i].policy = (uint8_t)Policy::IGNORE;
        }
        if (saveCurrent()) s_prefs.remove(KEY_OLD);
    }

    s_loaded = true;
}

Policy policy(const uint8_t* mac) {
    begin();
    const int idx = indexOf(mac);
    return idx < 0 ? Policy::NORMAL : (Policy)s_rec[(uint8_t)idx].policy;
}

bool contains(const uint8_t* mac) { return policy(mac) == Policy::IGNORE; }
bool trusted(const uint8_t* mac) { return policy(mac) == Policy::TRUSTED; }
bool alwaysAlert(const uint8_t* mac) { return policy(mac) == Policy::ALWAYS_ALERT; }

bool setPolicy(const uint8_t* mac, DetectionType type, Policy p) {
    begin();
    if (!mac) return false;
    if (p < Policy::NORMAL || p > Policy::ALWAYS_ALERT) return false;

    const int idx = indexOf(mac);

    if (p == Policy::NORMAL) {
        if (idx < 0) return true;
        const uint8_t at = (uint8_t)idx;
        const PolicyRec removed = s_rec[at];
        const uint8_t last = (uint8_t)(s_count - 1);
        if (at != last) s_rec[at] = s_rec[last];
        s_count--;
        if (saveCurrent()) return true;

        // Roll the in-memory mutation back if persistence failed.
        s_count++;
        s_rec[at] = removed;
        return false;
    }

    if (idx >= 0) {
        const uint8_t at = (uint8_t)idx;
        const PolicyRec before = s_rec[at];
        s_rec[at].policy = (uint8_t)p;
        // A raw-scan action has no classification; do not erase a useful type
        // we already learned from a classified detection.
        if (type != DetectionType::UNKNOWN ||
            s_rec[at].type == (uint8_t)DetectionType::UNKNOWN)
            s_rec[at].type = (uint8_t)type;

        if (saveCurrent()) return true;
        s_rec[at] = before;
        return false;
    }

    if (s_count >= MAX) return false;
    PolicyRec& r = s_rec[s_count];
    memcpy(r.mac, mac, 6);
    r.type = (uint8_t)type;
    r.policy = (uint8_t)p;
    s_count++;
    if (saveCurrent()) return true;
    s_count--;
    return false;
}

bool add(const uint8_t* mac, DetectionType type) {
    if (contains(mac)) return false;
    return setPolicy(mac, type, Policy::IGNORE);
}

bool remove(const uint8_t* mac) {
    begin();
    if (indexOf(mac) < 0) return false;
    return setPolicy(mac, DetectionType::UNKNOWN, Policy::NORMAL);
}

uint8_t count() { begin(); return s_count; }

const uint8_t* macAt(uint8_t idx) {
    begin();
    return idx < s_count ? s_rec[idx].mac : nullptr;
}

DetectionType typeAt(uint8_t idx) {
    begin();
    if (idx >= s_count) return DetectionType::UNKNOWN;
    const uint8_t v = s_rec[idx].type;
    return v < (uint8_t)DetectionType::COUNT ? (DetectionType)v
                                             : DetectionType::UNKNOWN;
}

Policy policyAt(uint8_t idx) {
    begin();
    return idx < s_count ? (Policy)s_rec[idx].policy : Policy::NORMAL;
}

void clear() {
    begin();
    s_count = 0;
    // Remove every historical key as well; otherwise a failed/partial old
    // migration could resurrect devices after a later downgrade/upgrade.
    s_prefs.remove(KEY);
    s_prefs.remove(KEY_TYPED);
    s_prefs.remove(KEY_OLD);
}

// ---- SNOOZE: RAM only, gone at restart ----------------------------------
static const uint8_t SNOOZE_MAX = 32;
static uint8_t s_snz[SNOOZE_MAX][6];
static uint8_t s_snzN = 0, s_snzNext = 0;

bool snoozed(const uint8_t* mac) {
    if (!mac) return false;
    for (uint8_t i = 0; i < s_snzN; i++)
        if (memcmp(s_snz[i], mac, 6) == 0) return true;
    return false;
}

void snooze(const uint8_t* mac) {
    if (!mac || snoozed(mac)) return;
    memcpy(s_snz[s_snzNext], mac, 6);
    s_snzNext = (uint8_t)((s_snzNext + 1) % SNOOZE_MAX);
    if (s_snzN < SNOOZE_MAX) s_snzN++;
}

bool silenced(const uint8_t* mac) {
    const Policy p = policy(mac);
    if (p == Policy::ALWAYS_ALERT) return false;
    if (p == Policy::IGNORE || p == Policy::TRUSTED) return true;
    return snoozed(mac);
}

}  // namespace IgnoreList
