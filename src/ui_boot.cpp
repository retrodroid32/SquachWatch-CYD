// SquachWatch-CYD — boot splash implementation
#include "ui_boot.h"
#include "theme.h"
#include "squachy.h"
#include "settings.h"
#include "draw_band.h"

// Stamped in by extra_script.py from `git describe` at build time --
// same macro the Diary screen already reads (see its own guard
// comment). Falls back to "unknown" so this still compiles standalone
// (the PC emulator, an IDE's syntax pass) without the build flag.
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "unknown"
#endif

// A dozen one-liners for Squachy's boot-splash speech bubble — one is
// picked at random each boot (see uiBootInit) so the splash doesn't
// say the exact same thing every single time.
static const char* BOOT_LINES[] = {
    "Surveillance state? Not on my watch.",
    "Smile! I'm watching the watchers.",
    "No cameras were harmed. Yet.",
    "Privacy is dead. I'm the eulogy.",
    "Big Brother's ugly cousin, actually.",
    "I collect MAC addresses, not friends.",
    "They see everything. I see them too.",
    "Cryptid by trade, snitch by hobby.",
    "Somewhere, a camera just got nervous.",
    "Not paranoid. Just well-informed.",
    "Trust no lens.",
    "Detecting nonsense since day one.",
};
static const uint8_t BOOT_LINE_COUNT = sizeof(BOOT_LINES) / sizeof(BOOT_LINES[0]);
static uint8_t s_bootLineIdx = 0;

// When this splash started, and how far through its scripted glitch
// bursts we are. The subtitle's chromatic split rides the shared burst
// (Theme::glitchActive()), but that rolls on its own every 5-10s while
// the splash only lasts 3000ms -- so left to the ambient schedule the
// effect would usually never fire during a boot at all. Driving the
// same shared burst deliberately is what triggerGlitchBurst() is for.
static uint32_t s_bootAt      = 0;
static uint8_t  s_glitchStage = 0;

void uiBootInit(TFT_eSPI& t) {
    // fillScreen() relies on TFT_eSPI's base-class width/height, which
    // TFT_eSprite::createSprite() never updates — it leaves stale
    // remnants when t is a sprite. fillRect() with the (virtual,
    // correctly-overridden) width()/height() clears the whole thing.
    t.fillRect(0, 0, t.width(), t.height(), Theme::BG);
    s_bootLineIdx = (uint8_t)random(0, BOOT_LINE_COUNT);
    s_bootAt      = 0;
    s_glitchStage = 0;
}

void uiBootTick(TFT_eSPI& t, uint32_t now, int physicalW,
                int physicalH, bool suppressGlitch) {
    if (s_bootAt == 0) s_bootAt = now ? now : 1;
    const uint32_t bootEl = now - s_bootAt;
    // Two bursts inside the 3s window: one early enough to be seen, one
    // late enough to feel like the thing is still settling. Intensities
    // stay mild -- 3 and up add a full-screen pixel-shift tear, which
    // on a splash reads as a fault rather than as style.
    // R8: in classic CYD28 recovery mode, BOOT is now rendered into the
    // same low-memory stripes as MAIN. The scripted chromatic/CRT bursts
    // look like the reported screen flashing; suppress them on this board
    // while leaving every other device's boot artwork unchanged.
    if (!suppressGlitch) {
        if (s_glitchStage == 0 && bootEl > 500)  { Theme::triggerGlitchBurst(2); s_glitchStage = 1; }
        if (s_glitchStage == 1 && bootEl > 1750) { Theme::triggerGlitchBurst(1); s_glitchStage = 2; }
    }

    // A band sprite is only 15/30/40px high, but splash geometry and all
    // absolute draw coordinates must use the whole rotated LCD. The stripe
    // viewport clips those absolute coordinates to its allocated RAM.
    const int w = physicalW > 0 ? physicalW : t.width();
    const int h = physicalH > 0 ? physicalH : t.height();
    int yHoriz = (h * 5) / 8;

    // The same composed scene the SYNTHWAVE background uses -- sky,
    // sun, ridgeline, reflective water and rungs -- rather than the
    // four separate calls this used to make. That version predated the
    // reflection, so the splash was showing a strictly worse sunset
    // than the background did; this keeps the two from drifting apart
    // again. Gulls stay layered on top, since they belong to the splash
    // and not to the background. Redrawn from scratch every frame
    // (cheap at boot-screen size, and simpler than dirty-rect tracking
    // for a screen that is only up for 3s). Text below is drawn with a
    // transparent background so the scene shows through around every
    // glyph instead of sitting in solid boxes.
    Theme::drawSynthwave(t, now, 0, h, 5.0f / 8.0f);   // same waterline this screen always had
    Theme::drawSeagulls(t, now, 0, yHoriz);

    // Big title, Bangers comic-impact font — landscape-only screen
    // (boot always starts at rotation 1) so the width is never tight.
    const char* title = "SQUACHWATCH";
    int tw = Theme::bangersTextWidth(title, Theme::BangersSize::LG);
    int tx = (w - tw) / 2;
    // 3px black outline. The title sits over a bright sunset now, and
    // pink-on-orange had almost no separation where the sun passed
    // behind it. Drawn as a dilation -- the same glyphs at every offset
    // inside a radius-3 disc -- rather than a rectangular halo, so the
    // stroke follows the letterforms instead of boxing them.
    // R9: this was 28+ complete glyph scans on EACH of R8's 8/16
    // recovery stripes, even though the title only spans the top ~50px.
    // The existing outline helper does the same thick silhouette in one
    // scan; skip it entirely on stripes that cannot contain the title.
    // Keep the original 3px circular dilation on every other board.
    if (DrawBand::has(0, 52)) {
        if (suppressGlitch) {
            Theme::drawBangersOutline(t, tx, 8, title, Theme::BLACK,
                                     Theme::BangersSize::LG, 3);
        } else {
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    if (dx * dx + dy * dy > 9 || (dx == 0 && dy == 0)) continue;
                    Theme::drawBangersText(t, tx + dx, 8 + dy, title, Theme::BLACK,
                                           Theme::BangersSize::LG);
                }
            }
        }
        Theme::drawBangersText(t, tx, 8, title, Theme::VAPOR_PINK, Theme::BangersSize::LG);
    }

    // The gradient divider also lives in only one or two real stripes.
    if (DrawBand::has(46, 47)) {
        for (int x = 0; x < w; x++)
            t.drawFastHLine(x, 46, 1, Theme::titlebarColor(x, w));
    }

    // TALKING SASQUACH subtitle -- the brand line under the product
    // name, not just the product name again.
    //
    // Chromatic: a white core with cyan and magenta copies split either
    // side, over a hard black drop shadow. The shadow replaces the old
    // 8-way outline and does the same job -- purple over the sun's
    // oranges had almost no separation -- but reads as depth rather
    // than as a sticker.
    //
    // The split rides Theme::glitchActive(), the shared burst the
    // Bangers headings already roll every 5-10s, rather than running a
    // timer of its own. That is the whole point of that flag being
    // exposed: when the wordmark above corrupts, this corrupts with it
    // and the two read as one event instead of two things that happen
    // to twitch near each other.
    //
    // Deliberately NOT built with setViewport, which is the obvious way
    // to slice a proper tear: real TFT_eSPI moves the drawing origin
    // when a viewport is set (vpDatum defaults to true) while the
    // emulator shim ignores that argument and treats it as a pure clip
    // rectangle. Anything built on it would look correct in one and
    // wrong on the other, and the emulator is where this gets checked.
    t.setTextSize(2);
    const char* sub = "TALKING SASQUACH";
    const int sw = t.textWidth(sub);
    const int sx = (w - sw) / 2;
    const int sy = 54;

    // Subtitle and its shadows occupy ~28px, not an entire panel.
    // Avoid repeating the text renderer across the other 6-14 stripes.
    if (DrawBand::has(sy - 4, sy + 27)) {
    const bool glitch = !suppressGlitch && Theme::glitchActive();
    // Freeze random displacement across all sprite stripes that constitute
    // one screen frame, so the subtitle cannot jump between adjacent bands.
    static uint32_t s_chromaAt = UINT32_MAX;
    static int s_chromaSplit = 1;
    static int s_chromaJitter = 0;
    if (s_chromaAt != now) {
        s_chromaAt = now;
        s_chromaSplit = glitch ? 3 + (int)random(0, 3) : 1;
        s_chromaJitter = glitch ? (int)random(-2, 3) : 0;
    }
    const int split = s_chromaSplit;
    const int jitter = s_chromaJitter;

    // Shadow first, so everything else sits on top of it. Two pixels
    // down and right: enough to lift the word off the sunset without
    // the gap reading as a second, blurrier copy of the text.
    t.setTextColor(Theme::BLACK);
    t.setCursor(sx + 2, sy + 2);
    t.print(sub);

    // Magenta trails right, cyan leads left -- the direction a
    // mistracked CRT actually smears.
    t.setTextColor(Theme::PINK);
    t.setCursor(sx + split + jitter, sy);
    t.print(sub);
    t.setTextColor(Theme::CYAN);
    t.setCursor(sx - split + jitter, sy);
    t.print(sub);

    // Burst only: one more cyan copy dropped a row or two, like a scan
    // that failed to land. Kept to a couple of rows so it reads as a
    // fault and not as a second line of text.
    if (glitch) {
        t.setTextColor(Theme::CYAN);
        t.setCursor(sx + jitter * 2, sy + 1 + (int)random(0, 2));
        t.print(sub);
    }

    // White core last, so the word stays legible whatever the copies
    // are doing around it.
    t.setTextColor(Theme::WHITE);
    t.setCursor(sx + jitter, sy);
    t.print(sub);

    // Snow over the subtitle's own box. A no-op while glitchActive() is
    // false, so this is safe to call every frame.
    if (!suppressGlitch)
        Theme::drawGlitchStatic(t, sx - 4, sy - 2, sx + sw + 4, sy + 18);
    } // subtitle's physical band only

    // Squachy himself, standing on the floor just past the horizon,
    // giving a friendly wave (and a little on-brand attitude) while
    // everything spins up. Sized as big as the gap between the
    // subtitle and INITIALIZING... below allows — dropping the v1.0
    // line and tightening the title stack above is what buys him the
    // extra room to be this big. Skipped in "boring mode" — the rest
    // of the boot splash (wordmark, subtitle, INITIALIZING...) is
    // unaffected, this only cuts the mascot cameo.
    // On CYD28 the cameo and speech bubble are entirely below h/5;
    // skipping invisible top stripes avoids repeating very costly body
    // polygons while keeping exactly the same wave pose/time everywhere.
    if (!Settings::boringMode() && DrawBand::has(h / 5, h)) {
        // 2.0, up from 1.6, standing on h-19 rather than yHoriz+50.
        //
        // Measured off a render: the subtitle's ink ends at row 67 and
        // INITIALIZING starts at 224, so there are about 150 usable rows and
        // he was only occupying 94 of them. The rest was air above his head.
        //
        // Both numbers had to move together. drawWaving puts his speech
        // bubble a fixed 34px above the head anchor, and the anchor is
        // baseY - 58*scale, so growing him alone drives the bubble up into
        // the subtitle -- at 1.6 it already sat only six rows clear. Lowering
        // where he stands buys that back. His feet barely move either way,
        // since they land at baseY - 5*scale.
        //
        // 2.06 is the hard ceiling with the bubble above him: 150 rows has to
        // hold a 14px bubble, his 60 base units, and air at each end. 2.0 hits
        // it exactly and leaves 2px top and bottom, which is a rounding error
        // away from touching. 1.95 gives 4px at both ends instead.
        // 1.95 was fitted by hand to a 240-tall panel, where the band between
        // the subtitle and INITIALIZING is about 150 rows and the bubble above
        // his head was what capped it. A 320-tall panel leaves 80 rows more and
        // he sat in the middle of them, small, with air all round -- the same
        // complaint the main screen's geometry solved years ago by sizing him
        // from the band he is handed rather than from a constant.
        //
        // So on a wide panel he is sized from the band. The limit is unchanged:
        // his head anchor is baseY - 58*scale and the bubble sits above that,
        // so the scale that just clears the subtitle is (band / 58). Narrow
        // panels keep the measured 1.95 exactly -- this is the one screen
        // where the constant was chosen against a render, and it still holds
        // there.
        float bootScale = 1.95f;
        if (w >= 400) {
            const int band = (h - 21) - 34 - (sy + 16);   // feet, bubble, subtitle
            bootScale = (float)(band - 10) / 58.0f;       // 10 rows of air at the top
            if (bootScale < 1.95f) bootScale = 1.95f;
            if (bootScale > 3.2f)  bootScale = 3.2f;
        }
        Squachy::drawWaving(t, w / 2, h - 21, now, bootScale, BOOT_LINES[s_bootLineIdx]);
    }

    // INITIALIZING...  vX.Y.Z -- version tacked onto this line rather
    // than given its own row. Everything from the subtitle down to
    // here is already tightly packed around Squachy's cameo (see its
    // own comment above about the room dropping the old standalone
    // version line bought him), and this is the one line on the splash
    // that already reads as status text, not brand/character content,
    // so it's the natural place for a version stamp without touching
    // his space.
    t.setTextSize(1);
    t.setTextColor(Theme::CYAN);
    char init[56];   // room for a full "git describe --dirty" string, not just a bare tag
#if defined(CYD35)
    // The 3.5" ships as a beta and says so on every boot. That board is newer
    // than the rest of the port by years, it is the only one that draws in two
    // bands, and it is the one most likely to still surprise somebody -- so
    // the word goes where nobody can miss it rather than in a release note
    // they may never have read.
    snprintf(init, sizeof(init), "3.5in BETA   %s", FIRMWARE_VERSION);
#else
    snprintf(init, sizeof(init), "INITIALIZING...  %s", FIRMWARE_VERSION);
#endif
    int iw = t.textWidth(init);
    t.setCursor((w - iw) / 2, h - 16);
#if defined(CYD35)
    t.setTextColor(Theme::AMBER);   // not the cyan the rest of the line uses
#endif
    if (DrawBand::has(h - 19, h)) t.print(init);

    // animated scanline sweeping top to bottom every 600 ms
    int phase = (int)((now / 600) % (uint32_t)h);
    if (DrawBand::has(phase, phase + 1))
        Theme::drawScanline(t, phase, Theme::VAPOR_PURPLE);
}

bool uiBootDone(uint32_t startMs, uint32_t holdMs) {
    return (millis() - startMs) >= holdMs;
}
