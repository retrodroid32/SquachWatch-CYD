// SquachWatch-CYD — the cell block: the ball and chain. Included by pet.cpp
// inside namespace Pet, after C1iPPY and T0@$TY, whose throws it counts and
// whose balloon it borrows.
//
// Throw a pet ten times inside a minute and it is taken into custody. Sirens,
// a puff of smoke where the pet stood, and an iron ball drops in with a clank
// and is chained to Squachy's ankle for the ten minutes he serves (the
// sentence itself, the stripes and the yank on a throw live in squachy.cpp).
// The ball has a face, and opinions about people who throw pets.
//
// Lines are three balloon lines of 24 characters at most, 72 in all.
static const char* const BALL_LINES[] = {
    "You threw a pet. Now you've got me.",
    "I'm your pet now. Try throwing me.",
    "Ball and chain. Emphasis on chain.",
    "Nice stripes. They suit you.",
    "Ten minutes. I'll be counting. Out loud.",
    "Your pet's fine. It's in protective custody.",
    "Your pet filed a report. It was mostly crying.",
    "Ten throws a minute is a hobby, not an accident.",
    "Some people collect stamps. You collect charges.",
    "I've met rocks with better manners.",
    "Who throws a toaster? You. That's who.",
    "Fun fact: I don't bounce either.",
    "Rehabilitation starts now. Sit still.",
    "Solid iron. Unlike your alibi.",
    "I have one job. Weighing you down.",
};
static const uint8_t BALL_LINES_N = sizeof(BALL_LINES) / sizeof(BALL_LINES[0]);
static const char* const BALL_ARREST = "Ten throws in one minute? You're under arrest.";
static const char* const BALL_YANK[3] = { "Nope.", "Where do you think you're going?", "Sit." };
static const char* const BALL_POKE[3] = { "Don't touch me. You'll throw me.", "Hands where I can see them.", "Poking counts. I'm writing it down." };
static const char* const BALL_FREE = "Time served. Try petting them this time.";
// Squachy, doing his time.
static const char* const JAIL_SQ[4] = {
    "I was framed. By a toaster.", "Three square meals. All toast.",
    "I want my lawyer. And my pet.", "it was ten throws. ten. tops.",
};

// ---- the throws ---------------------------------------------------------------
// The last ten flings of any pet. The tenth inside a minute of the first is
// the arrest.
static uint32_t s_throwAt[10] = {};
static uint8_t  s_throwN = 0;
static uint32_t s_jArrestAt = 0;          // the arrest's sirens and clank, 0 when not running
static uint32_t s_jFreeAt = 0;            // the walk out, 0 when not running
static const char* s_jLine = nullptr;
static uint32_t s_jSaidAt = 0, s_jNextAt = 0, s_jYankSeen = 0, s_jPokeAt = 0;
static uint8_t  s_jMood = 0;              // 0 talk, 1 laugh, 2 smug
static uint8_t  s_jIx = 0, s_jPoke = 0, s_jSq = 0;
static bool     s_jAnswered = true;
int16_t         s_jX = -100, s_jY = 0, s_jR = 0;   // the ball this frame, for taps
static int      s_jaCx = 0, s_jaHalfW = 0, s_jaBot = 0;
static bool     s_jaValid = false;

static void ballSay(const char* line, uint32_t now, uint8_t mood) {
    s_jLine = line; s_jSaidAt = now ? now : 1; s_jMood = mood; s_jAnswered = false;
}

void arrest(uint32_t now) {
    if (Squachy::jailed() || s_jArrestAt) return;
    Squachy::jailStart();
    s_jArrestAt = now ? now : 1;
    s_jFreeAt = 0;
    s_jLine = nullptr;
    s_jNextAt = now + 60000;
    s_jYankSeen = Squachy::yankedAt();
    s_throwN = 0;
}

static void noteThrow(uint32_t now) {
    if (Squachy::jailed()) return;
    if (s_throwN < 10) s_throwAt[s_throwN++] = now;
    else { for (uint8_t i = 1; i < 10; i++) s_throwAt[i - 1] = s_throwAt[i]; s_throwAt[9] = now; }
    if (s_throwN == 10 && now - s_throwAt[0] <= 60000) arrest(now);
}

static bool jailShowing(uint32_t now) {
    if (s_jFreeAt && now - s_jFreeAt > 4200) s_jFreeAt = 0;
    return Squachy::jailed() || s_jArrestAt || s_jFreeAt;
}

static bool ballHit(int x, int y) {
    if (s_jX < -50) return false;
    return x >= s_jX - s_jR - 4 && x <= s_jX + s_jR + 4 && y >= s_jY - 2 * s_jR - 4 && y <= s_jY + 2;
}
static void ballPoke(uint32_t now) {
    if (now - s_jPokeAt > 8000) s_jPoke = 0;
    s_jPokeAt = now;
    ballSay(BALL_POKE[s_jPoke++ % 3], now, 2);
    s_jAnswered = true;
}

// ---- the ball ---------------------------------------------------------------------
// Sitting on the floor at (cx, bottom), radius r. mood: 0 talk, 1 laugh,
// 2 smug, 3 idle, 4 eye roll, 5 squashed.
static void ballDraw(TFT_eSPI& t, int cx, int bottom, int r, uint32_t now, uint8_t mood, int lookDir) {
    const float k = (float)r / 13.0f;
    auto K = [k](float v) { int n = (int)lroundf(v * k); return n < 1 ? 1 : n; };
    const uint16_t BASE = t.color565(43, 46, 52), SHADE = t.color565(22, 24, 28), HI = t.color565(138, 147, 163);
    const uint16_t INK = t.color565(232, 236, 242), PUPIL = t.color565(11, 11, 13), RIM = t.color565(5, 5, 7);
    const uint16_t CH_HI = t.color565(170, 177, 188);
    const float sq = mood == 5 ? 0.78f : 1.0f;
    int ry = (int)lroundf((float)r * sq);
    if (ry < 3) ry = 3;
    if (mood == 1) cx += ((now / 70) & 1) ? 1 : -1;
    const int cy = bottom - ry;
    auto oval = [&](int rr, int ryy, uint16_t col) {
        for (int y = -ryy; y <= ryy; y++) {
            const float u = 1.0f - (float)(y * y) / (float)(ryy * ryy);
            const int half = (int)(rr * sqrtf(u > 0 ? u : 0) + 0.35f);
            t.drawFastHLine(cx - half, cy + y, half * 2 + 1, col);
        }
    };
    t.drawFastHLine(cx - r + 2, bottom + 1, r * 2 - 4, t.color565(20, 20, 20));
    oval(r + 1, ry + 1, RIM);
    oval(r, ry, BASE);
    for (int y = -ry; y <= ry; y++) {
        const float u = 1.0f - (float)(y * y) / (float)(ry * ry);
        const int half = (int)(r * sqrtf(u > 0 ? u : 0) + 0.35f);
        const int s = (int)lroundf(half * 0.35f + y * 0.25f);
        if (s > 0) t.drawFastHLine(cx + half - s + 1, cy + y, s, SHADE);
    }
    t.fillRect(cx - (int)lroundf(r * 0.55f), cy - (int)lroundf(ry * 0.6f), K(3), K(2), HI);
    t.fillRect(cx - (int)lroundf(r * 0.6f), cy - (int)lroundf(ry * 0.3f), 1, K(2), HI);
    // the ring the chain hangs from
    t.drawCircle(cx - (int)lroundf(r * 0.72f), cy - (int)lroundf(ry * 0.72f), K(2), CH_HI);
    // ---- his face ----
    const bool blink = mood == 3 && (now % 3900) < 140;
    const int ey = cy - (int)lroundf(ry * 0.25f);
    const int er = K(2.5f);
    const int exs[2] = { cx - K(5), cx + K(5) };
    for (int e = 0; e < 2; e++) {
        const int x = exs[e];
        if (blink) { t.drawFastHLine(x - er, ey, er * 2 + 1, INK); continue; }
        t.fillCircle(x, ey, er, INK);
        const int p = er * 7 / 10 > 0 ? er * 7 / 10 : 1;
        t.fillRect(x + lookDir * (er - 1 > 0 ? er - 1 : 0), ey, p, p, PUPIL);
        if (mood == 2 || mood == 4) t.fillRect(x - er, ey - er, er * 2 + 1, er, BASE);
        if (mood == 4) t.fillRect(x - 1, ey - er + 1, 2, 1, PUPIL);
    }
    // brows: angry when he talks or watches, one up when he is smug
    {
        const int bw = K(5), by = ey - (K(5) > 3 ? K(5) : 3);
        const bool angry = mood == 0 || mood == 3;
        for (int i = 0; i < bw; i++) {
            t.drawPixel(exs[0] - bw / 2 + i, by + (angry ? i / 2 : 0) - (mood == 2 && i > bw / 2 ? 1 : 0), INK);
            t.drawPixel(exs[1] - bw / 2 + i, by + (angry ? (bw - 1 - i) / 2 : 0) - (mood == 1 ? 1 : 0), INK);
        }
    }
    // the mouth
    const int my = cy + (int)lroundf(ry * 0.4f), mw = K(9) > 3 ? K(9) : 3;
    if (mood == 1) {
        t.fillRect(cx - mw / 2, my - 1, mw, K(4) > 2 ? K(4) : 2, INK);
        t.fillRect(cx - mw / 2 + 1, my, mw - 2, K(2), t.color565(122, 13, 26));
    } else if (mood == 0 && ((now / 150) & 1) == 0) {
        t.fillRect(cx - mw / 2 + 1, my - 1, mw - 2, K(3) > 2 ? K(3) : 2, INK);
    } else {
        t.drawFastHLine(cx - mw / 2, my, mw, INK);
        t.drawPixel(cx + mw / 2, my - 1, INK);          // the smirk
    }
}

// The chain, ankle to ring: a sagging curve of links lying along the floor,
// pulled straight while he is off the ground.
static void chainDraw(TFT_eSPI& t, int ax, int ay, int bx, int by, int floorY, uint32_t now, float k, bool taut) {
    const uint16_t LINK = t.color565(107, 114, 128), LINK_HI = t.color565(170, 177, 188), RIM = t.color565(5, 5, 7);
    int sag = taut ? 0 : (int)lroundf((6.0f + sinf((float)now / 400.0f) * 1.5f) * k);
    if (!taut && sag < 2) sag = 2;
    const float cx = (ax + bx) * 0.5f;
    float cy = (float)((ay > by ? ay : by) + sag * 2);
    if (cy > floorY - 1) cy = (float)(floorY - 1);
    const float step = 3.0f * k > 2.0f ? 3.0f * k : 2.0f;
    int n = (int)(hypotf((float)(bx - ax), (float)(by - ay)) / step);
    if (n < 6) n = 6;
    const int big = (int)lroundf(3 * k) > 2 ? (int)lroundf(3 * k) : 2, small = (int)lroundf(2 * k) > 1 ? (int)lroundf(2 * k) : 1;
    for (int i = 0; i <= n; i++) {
        const float u = (float)i / (float)n;
        const float x = (1 - u) * (1 - u) * ax + 2 * (1 - u) * u * cx + u * u * bx;
        float y = (1 - u) * (1 - u) * ay + 2 * (1 - u) * u * cy + u * u * by;
        if (y > floorY - 1) y = (float)(floorY - 1);
        const int lw = (i & 1) ? small : big, lh = (i & 1) ? big : small;
        const int lx = (int)lroundf(x) - lw / 2, ly = (int)lroundf(y) - lh / 2;
        t.fillRect(lx - 1, ly - 1, lw + 2, lh + 2, RIM);
        t.fillRect(lx, ly, lw, lh, LINK);
        t.drawPixel(lx, ly, LINK_HI);
    }
}

static void jailTick(TFT_eSPI& t, uint32_t now, int screenW) {
    int cx, halfW, top, bot;
    if (!Squachy::lastFootprint(cx, halfW, top, bot)) { s_jX = -100; return; }
    (void)top;
    // The ball stays where he stands; a carried or thrown Squachy pulls the
    // chain, not the ball.
    if (Squachy::isHeld() || Squachy::thrown()) {
        if (!s_jaValid) { s_jX = -100; return; }
        cx = s_jaCx; halfW = s_jaHalfW; bot = s_jaBot;
    } else { s_jaCx = cx; s_jaHalfW = halfW; s_jaBot = bot; s_jaValid = true; }
    const int H = t.height();
    const float scale = (float)halfW / 24.0f, k = scale / 2.7f;
    int r = (int)lroundf(13.0f * k);
    if (r < 6) r = 6;
    int ballX = cx + (int)lroundf(76.0f * k);
    if (ballX > screenW - r - 4) ballX = screenW - r - 4;
    const int floorY = bot + 1;

    // Walked out: the ball says its piece, then goes up in smoke.
    if (Squachy::takeJailFreed()) { s_jFreeAt = now ? now : 1; s_jArrestAt = 0; ballSay(BALL_FREE, now, 2); s_jAnswered = true; }

    int drop = 0;
    bool ballOn = true, clank = false, siren = false;
    float puff = -1.0f;
    if (s_jArrestAt) {
        const uint32_t a = now - s_jArrestAt;
        siren = a < 1800;
        if (a < 1000) ballOn = false;
        if (a >= 500 && a < 1300) puff = (float)(a - 500) / 800.0f;
        if (a >= 1000 && a < 1500) { const float u = (float)(a - 1000) / 500.0f; drop = (int)lroundf((1.0f - u * u) * -90.0f * H / 240.0f); }
        if (a >= 1450 && a < 1900) clank = true;
        if (a >= 1500 && !s_jLine) { ballSay(BALL_ARREST, now, 2); s_jAnswered = true; }
        if (a > 1900) s_jArrestAt = 0;
    }
    if (s_jFreeAt) {
        const uint32_t a = now - s_jFreeAt;
        if (a > 3400) { ballOn = false; if (a < 4200) puff = (float)(a - 3400) / 800.0f; }
    }
    // The chain stopped a throw: he enjoyed that.
    const uint32_t ya = Squachy::yankedAt();
    if (ya && ya != s_jYankSeen) { s_jYankSeen = ya; ballSay(BALL_YANK[random(0, 3)], now, 1); s_jAnswered = true; }
    // Something to say about once a minute, and Squachy answers now and then.
    const bool talking = s_jLine && now - s_jSaidAt < 5500;
    if (Squachy::jailed() && !s_jArrestAt && !talking && (int32_t)(now - s_jNextAt) >= 0) {
        ballSay(BALL_LINES[s_jIx++ % BALL_LINES_N], now, random(0, 3) == 0 ? 1 : 0);
        s_jNextAt = now + 45000 + (uint32_t)random(0, 30000);
    }
    if (s_jLine && !talking && !s_jAnswered) {
        s_jAnswered = true;
        if (random(0, 3) == 0) Squachy::visitSay(JAIL_SQ[s_jSq++ % 4]);
    }

    if (ballOn) {
        const int bb = floorY + drop;
        int ax, ay;
        const bool chained = Squachy::chainPoint(ax, ay);
        const int ringX = ballX - (int)lroundf(r * 0.72f), ringY = bb - r - (int)lroundf(r * 0.72f);
        if (chained && drop == 0) chainDraw(t, ax, ay, ringX, ringY, floorY, now, k, Squachy::isHeld() || Squachy::thrown());
        uint8_t mood = 3;
        if (talking) mood = (s_jMood == 1 && now - s_jSaidAt < 1600) ? 1 : (s_jMood == 2 && now - s_jSaidAt > 2000 ? 2 : 0);
        else if ((now % 9000) > 7600) mood = 4;
        if (s_jArrestAt && now - s_jArrestAt >= 1500 && now - s_jArrestAt < 1700) mood = 5;
        ballDraw(t, ballX, bb, r, now, mood, -1);
        s_jX = (int16_t)ballX; s_jY = (int16_t)bb; s_jR = (int16_t)r;
        if (clank) {
            t.setTextSize(1);
            const int cw = t.textWidth("CLANK") + 4;
            t.fillRect(ballX + r - 2, bb - 2 * r - 10, cw, 10, t.color565(255, 225, 77));
            t.setTextColor(Theme::BLACK);
            t.setCursor(ballX + r, bb - 2 * r - 9);
            t.print("CLANK");
        }
        // On a short screen the balloon goes beside him, so hang it from
        // higher up or it sits on the buttons.
        if (talking && drop == 0) petBalloon(t, s_jLine, ballX - r, bb - 2 * r - (H < 200 ? 3 * r : 0), 2 * r, screenW, H);
    } else s_jX = -100;
    if (puff >= 0.0f) {
        const uint16_t smoke = t.color565(217, 217, 217);
        for (int i = 0; i < 10; i++) {
            const float a = (float)i / 10.0f * 6.2831853f, d = 4.0f + puff * 18.0f * k;
            const int pr = (int)lroundf(3 * k) > 1 ? (int)lroundf(3 * k) : 1;
            if (puff < 0.85f) t.fillCircle(ballX + (int)lroundf(cosf(a) * d), floorY - (int)lroundf(10 * k) + (int)lroundf(sinf(a) * d * 0.6f), pr, smoke);
        }
    }
    if (siren) {
        const bool red = ((now / 180) & 1) == 0;
        const uint16_t R_ = t.color565(255, 32, 32), B_ = t.color565(32, 80, 255);
        t.fillRect(0, 0, screenW, 4, red ? R_ : B_);
        t.fillRect(0, H - 4, screenW, 4, red ? B_ : R_);
        t.setTextSize(H >= 200 ? 3 : 2);
        const char* s = "BUSTED!";
        const int tw = t.textWidth(s), ty = H * 2 / 5;
        t.setTextColor(Theme::BLACK);
        t.setCursor((screenW - tw) / 2 + 2, ty + 2); t.print(s);
        t.setTextColor(t.color565(255, 225, 77));
        t.setCursor((screenW - tw) / 2, ty); t.print(s);
        t.setTextSize(1);
    }
}

static void jailReset() { s_jX = -100; s_jaValid = false; }
