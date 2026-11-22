#!/usr/bin/env python3
"""R11: SquachWatch CYD32 ST7798 guarded single-pass framebuffer experiment.
Source structural checks and reserve model. Hardware FPS/heap still mandatory.
"""
from pathlib import Path
main=(Path(__file__).resolve().parents[1]/"src/main.cpp").read_text(encoding="utf-8")
theme=(Path(__file__).resolve().parents[1]/"src/theme.cpp").read_text(encoding="utf-8")
push=(Path(__file__).resolve().parents[1]/"src/frame_push.cpp").read_text(encoding="utf-8")
def between(s,start,end):
    return s.split(start,1)[1].split(end,1)[0]

# No forced FULL8 on a GPS CYD32. Preallocation requires >= 64 KiB free,
# >= 24 KiB contiguous after the 76800-byte buffer PLUS 8 KiB headroom.
guard=between(main,"static bool cyd32CanTryFull8() {","static bool s_frameLent = false;")
assert "(uint32_t)tft.width() * (uint32_t)tft.height()" in guard
assert "pixels + 64u * 1024u + 8u * 1024u" in guard
assert "pixels + 24u * 1024u + 8u * 1024u" in guard
assert "ESP.getFreeHeap() >= 64u * 1024u" in guard
assert "heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) >= 24u * 1024u" in guard
setup=between(main,"void setup() {","void loop() {")
assert "if (cyd32CanTryFull8() &&" in setup
assert "frame.createSprite(tft.width(), tft.height()))" in setup
assert "if (cyd32Full8ReservesIntact())" in setup
assert "frame.deleteSprite();" in setup
assert "s_recoveryBands = cyd32Full8 ? 1 : 2;" in setup
assert "if (!frame.created() &&" in setup
assert "!frame.createSprite(tft.width(), tft.height() / s_recoveryBands)" in setup
assert 'cyd32Full8 ? "FULL8" : "HALF8"' in setup
assert "canvas = s_recoveryBands == 1 ? (TFT_eSPI*)&frame : &tft;" in setup

# After WiFi download framebuffer is lent. Don't silently allocate unsafe
# FULL8 if WiFi has fragmented the heap; preserve 2/4/8/16-band fallback.
restore=between(main,"static void restoreFrameBuffer() {","static void enterWifiPass(")
assert "if (bands == 1 && !cyd32CanTryFull8()) continue;" in restore
assert "if (bands == 1 && !cyd32Full8ReservesIntact())" in restore
assert "frame.deleteSprite();" in restore
assert "for (uint8_t bands = 1; bands <= 16; bands <<= 1)" in restore

# Both modes use TFT_eSPI::pushSprite, not the old custom raw SPI writer.
pushfn=between(main,"static inline void pushFrame(int x, int y) {","// Draw a whole screen")
assert "#if defined(CYD32)" in pushfn
assert "frame.pushSprite(x, y);" in pushfn
assert "FramePush::invalidate();" in pushfn
assert "[push] CYD32 NATIVE TFT_eSPI pushSprite" in pushfn
assert "FramePush::push(tft, frame.buf()" in pushfn # non-CYD32 only

# ONE logical scene step in FULL8, first-band-only in HALF8.
draw=between(main,"static __attribute__((noinline, noclone)) void drawTwoBandCore(","template <typename F>")
assert "invoke(context, (TFT_eSPI&)frame, b == 0);" in draw
assert "invoke(context, *canvas, true);" in draw
assert "DrawBand::set(y, y + bandH);" in draw
assert "[cyd32-frame] screen=%s fps=%lu bands=%u" in draw
clear=between(main,"case AppState::CLEAR: {","case AppState::LOG: {")
assert "drawTwoBand([&](TFT_eSPI& t, bool advance)" in clear
assert "s_cyd32OnePassDrawUs = micros() - paintAt;" in clear
assert "s_cyd32OnePassDrawUs = micros() - paintAt;" in main
assert "mode=NATIVE_FULL8" in main
assert "s_pushAccumUs" in main
assert "if (frameBufferOk && s_recoveryBands == 1 && renderedThisLoop" in main

# Rotation must reshape in-place rather than risking reallocation and
# native ST7798 panel clear occurs only once per rotate, not per frame.
rot=between(main,"if (g_consoleRotate ||","#endif  // !AWOK")
assert "frame.resizeInPlace(tft.width(), tft.height() / s_recoveryBands);" in rot
assert "frame.deleteSprite(" not in rot
assert "tft.fillScreen(Theme::BG);" in rot
assert "static uint16_t s_lut[256];" in push
assert "now == lastMs ? 0u" in theme # R10 Gibson seam corrected

# R7–R9 2.8 only uses its original 8/16-band allocation; SD browser
# and intro remain intact. R11 must not touch these code paths.
assert "for (uint8_t bands = 8; bands <= 16; bands <<= 1)" in setup
assert "SdBrowser::draw(t, tft.width(), tft.height());" in main
assert "[boot-r9]" in main

# Model exercises explicit pre/post reserve cases; the C++ guards are the
# source of truth, this checks intended threshold boundaries.
def can_try(free,largest,pixels=76800):
    return free>=pixels+64*1024+8*1024 and largest>=pixels+24*1024+8*1024
def remains(free,largest):
    return free>=64*1024 and largest>=24*1024
assert can_try(160000,125000)
assert not can_try(130000,125000)
assert not can_try(160000,95000)
assert remains(65536,24576)
assert not remains(65535,24576)
assert not remains(65536,24575)
print("PASS: R11 guarded single-pass ST7798 FULL8/HALF8 selection, native transfer, R10 scene sync and CYD28 SD recovery")
