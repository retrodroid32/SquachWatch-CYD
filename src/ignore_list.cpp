// SquachWatch-CYD — per-device alert policy implementation
#include "ignore_list.h"
#include <Preferences.h>
#include <string.h>

namespace IgnoreList {

struct __attribute__((packed)) PolicyRec {
    uint8_t mac[6];
    uint8_t type;
    uint8_t policy;
    char    label[16];
};
static_assert(sizeof(PolicyRec) == 24, "device policy record must stay compact");

static Preferences s_prefs;
static bool        s_loaded = false;
static PolicyRec   s_rec[MAX];
static uint8_t     s_count = 0;

static const char* NS       = "ignore";
static const char* KEY      = "dev2";   // PolicyRec records
static const char* KEY_TYPED= "dev";    // old 7-byte mac+type format
static const char* KEY_OLD  = "macs";   // oldest 6-byte MAC format

static void save() {
    if (s_count == 0) {
        s_prefs.remove(KEY);
        return;
    }
    s_prefs.putBytes(KEY, s_rec, (size_t)s_count * sizeof(PolicyRec));
}

static void copyLabel(char* out, const char* in) {
    memset(out, 0, 16);
    if (in && in[0]) strncpy(out, in, 15);
}

void begin() {
    if (s_loaded) return;
    s_prefs.begin(NS, false);
    s_count = 0;

    size_t len = s_prefs.getBytesLength(KEY);
    if (len >= sizeof(PolicyRec)) {
        if (len > sizeof(s_rec)) len = sizeof(s_rec);
        s_prefs.getBytes(KEY, s_rec, len);
        s_count = (uint8_t)(len / sizeof(PolicyRec));
        for (uint8_t i = 0; i < s_count; i++) {
            if (s_rec[i].type >= (uint8_t)DetectionType::COUNT)
                s_rec[i].type = (uint8_t)DetectionType::UNKNOWN;
            if (s_rec[i].policy < (uint8_t)Policy::IGNORE ||
                s_rec[i].policy > (uint8_t)Policy::ALWAYS_ALERT)
                s_rec[i].policy = (uint8_t)Policy::IGNORE;
            s_rec[i].label[15] = '\0';
        }
        s_loaded = true;
        return;
    }

    // v1.20.x: migrate mac[6]+type records to explicit IGNORE policies.
    size_t old = s_prefs.getBytesLength(KEY_TYPED);
    if (old >= 7) {
        uint8_t tmp[MAX * 7];
        if (old > sizeof(tmp)) old = sizeof(tmp);
        s_prefs.getBytes(KEY_TYPED, tmp, old);
        s_count = (uint8_t)(old / 7u);
        for (uint8_t i = 0; i < s_count; i++) {
            memcpy(s_rec[i].mac, &tmp[(size_t)i * 7u], 6);
            const uint8_t ty = tmp[(size_t)i * 7u + 6];
            s_rec[i].type = ty < (uint8_t)DetectionType::COUNT ? ty : (uint8_t)DetectionType::UNKNOWN;
            s_rec[i].policy = (uint8_t)Policy::IGNORE;
            s_rec[i].label[0] = '\0';
        }
        save();
        s_prefs.remove(KEY_TYPED);
        s_prefs.remove(KEY_OLD);
        s_loaded = true;
        return;
    }

    // Pre-type format: MACs only.
    old = s_prefs.getBytesLength(KEY_OLD);
    if (old >= 6) {
        uint8_t tmp[MAX * 6];
        if (old > sizeof(tmp)) old = sizeof(tmp);
        s_prefs.getBytes(KEY_OLD, tmp, old);
        s_count = (uint8_t)(old / 6u);
        for (uint8_t i = 0; i < s_count; i++) {
            memcpy(s_rec[i].mac, &tmp[(size_t)i * 6u], 6);
            s_rec[i].type = (uint8_t)DetectionType::UNKNOWN;
            s_rec[i].policy = (uint8_t)Policy::IGNORE;
            s_rec[i].label[0] = '\0';
        }
        save();
        s_prefs.remove(KEY_OLD);
    }
    s_loaded = true;
}

static int indexOf(const uint8_t* mac) {
    if (!mac) return -1;
    for (uint8_t i = 0; i < s_count; i++)
        if (memcmp(s_rec[i].mac, mac, 6) == 0) return (int)i;
    return -1;
}

Policy policy(const uint8_t* mac) {
    begin();
    const int i = indexOf(mac);
    return i < 0 ? Policy::NORMAL : (Policy)s_rec[i].policy;
}

bool contains(const uint8_t* mac) { return policy(mac) == Policy::IGNORE; }
bool trusted(const uint8_t* mac) { return policy(mac) == Policy::TRUSTED; }
bool alwaysAlert(const uint8_t* mac) { return policy(mac) == Policy::ALWAYS_ALERT; }

bool setPolicy(const uint8_t* mac, DetectionType type, Policy p, const char* label) {
    begin();
    if (!mac) return false;
    int idx = indexOf(mac);

    if (p == Policy::NORMAL) {
        if (idx < 0) return true;
        const uint8_t last = (uint8_t)(s_count - 1);
        if ((uint8_t)idx != last) s_rec[idx] = s_rec[last];
        s_count--;
        save();
        return true;
    }

    if (idx < 0) {
        if (s_count >= MAX) return false;
        idx = s_count++;
        memset(&s_rec[idx], 0, sizeof s_rec[idx]);
        memcpy(s_rec[idx].mac, mac, 6);
    }
    s_rec[idx].type = (uint8_t)type < (uint8_t)DetectionType::COUNT
                    ? (uint8_t)type : (uint8_t)DetectionType::UNKNOWN;
    s_rec[idx].policy = (uint8_t)p;
    if (label) copyLabel(s_rec[idx].label, label);
    save();
    return true;
}

bool setLabel(const uint8_t* mac, const char* label) {
    begin();
    const int idx = indexOf(mac);
    if (idx < 0) return false;
    copyLabel(s_rec[idx].label, label);
    save();
    return true;
}

const char* labelFor(const uint8_t* mac) {
    begin();
    const int idx = indexOf(mac);
    return idx < 0 ? "" : s_rec[idx].label;
}

bool add(const uint8_t* mac, DetectionType type) {
    if (contains(mac)) return false;
    return setPolicy(mac, type, Policy::IGNORE);
}

bool remove(const uint8_t* mac) {
    begin();
    const int idx = indexOf(mac);
    if (idx < 0) return false;
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
    return v < (uint8_t)DetectionType::COUNT ? (DetectionType)v : DetectionType::UNKNOWN;
}

Policy policyAt(uint8_t idx) {
    begin();
    if (idx >= s_count) return Policy::NORMAL;
    const uint8_t p = s_rec[idx].policy;
    return (p >= (uint8_t)Policy::IGNORE && p <= (uint8_t)Policy::ALWAYS_ALERT)
           ? (Policy)p : Policy::NORMAL;
}

const char* labelAt(uint8_t idx) {
    begin();
    return idx < s_count ? s_rec[idx].label : "";
}

void clear() {
    begin();
    s_count = 0;
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
    return p == Policy::IGNORE || p == Policy::TRUSTED || snoozed(mac);
}

}  // namespace IgnoreList
