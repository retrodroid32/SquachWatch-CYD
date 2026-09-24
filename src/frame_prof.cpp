#include "frame_prof.h"

namespace FrameProf {

static uint32_t s_acc[N];   // this frame
static uint32_t s_avg[N];   // smoothed, 1/8 a frame, like the totals in main.cpp
static uint32_t s_last;

// A short rolling window catches the hitch that an EMA hides. 64 uint32s is
// 256 bytes and sorting a local copy happens only when telemetry is printed.
static const uint8_t FRAME_WINDOW = 64;
static uint32_t s_frameUs[FRAME_WINDOW];
static uint8_t  s_frameHead = 0;
static uint8_t  s_frameCount = 0;

void begin() {
    s_last = micros();
    for (uint8_t i = 0; i < N; i++) s_acc[i] = 0;
}

void lap(Slot s) {
    const uint32_t t = micros();
    s_acc[s] += t - s_last;
    s_last = t;
}

void endFrame(uint32_t totalUs) {
    for (uint8_t i = 0; i < N; i++)
        s_avg[i] = s_avg[i] ? s_avg[i] + ((int32_t)s_acc[i] - (int32_t)s_avg[i]) / 8 : s_acc[i];
    s_frameUs[s_frameHead] = totalUs;
    s_frameHead = (uint8_t)((s_frameHead + 1) % FRAME_WINDOW);
    if (s_frameCount < FRAME_WINDOW) s_frameCount++;
}

WindowStats windowStats() {
    WindowStats out = {};
    const uint8_t n = s_frameCount;
    out.samples = n;
    if (!n) return out;

    uint32_t v[FRAME_WINDOW];
    for (uint8_t i = 0; i < n; i++) v[i] = s_frameUs[i];
    for (uint8_t i = 1; i < n; i++) {
        const uint32_t x = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; }
        v[j + 1] = x;
    }
    const uint8_t p95 = (uint8_t)(((uint16_t)n * 95u + 99u) / 100u - 1u);
    out.p95Us = v[p95];
    out.maxUs = v[n - 1];
    for (uint8_t i = 0; i < n; i++) if (v[i] > 50000u) out.over50ms++;
    return out;
}

void print() {
    static const char* NAMES[N] = { "pre", "bg", "squachy", "idle", "headline", "chrome", "post", "push", "x1", "x2", "x3", "x4", "x5", "x6" };
    Serial.print("[frame]");
    for (uint8_t i = 0; i < N; i++)
        if (i < X1 || s_avg[i])
        Serial.printf("  %s %lu.%lu", NAMES[i],
                      (unsigned long)(s_avg[i] / 1000), (unsigned long)((s_avg[i] / 100) % 10));
    const WindowStats ws = windowStats();
    Serial.printf("  jitter p95 %lu.%lu max %lu.%lu >50 %u/%u\n",
                  (unsigned long)(ws.p95Us / 1000), (unsigned long)((ws.p95Us / 100) % 10),
                  (unsigned long)(ws.maxUs / 1000), (unsigned long)((ws.maxUs / 100) % 10),
                  (unsigned)ws.over50ms, (unsigned)ws.samples);
}

}  // namespace FrameProf
