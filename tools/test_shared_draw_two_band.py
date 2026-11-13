#!/usr/bin/env python3
"""Validate shared drawTwoBand renderer shape and synchronous capture contract."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/"src/main.cpp").read_text(encoding="utf-8")
a=source.index("using DrawTwoBandCallback =")
b=source.index("static inline uint32_t emaUpdate(",a)
body=source[a:b]
assert "static __attribute__((noinline, noclone)) void drawTwoBandCore(" in body
assert body.count("invoke(context, (TFT_eSPI&)frame, true);")==1
assert body.count("invoke(context, (TFT_eSPI&)frame, false);")==1
assert body.count("invoke(context, (TFT_eSPI&)frame, b == 0);")==1
assert body.count("invoke(context, *canvas, true);")==1
assert "DrawBand::set(y, y + bandH);" in body
assert "frame.setViewport(0, -y, tft.width(), tft.height(), true);" in body
assert "pushFrame(0, y);" in body
assert "frame.resetViewport();" in body
assert "DrawBand::all();" in body
assert "drawTwoBandCore(" in body and "&draw);" in body
assert "(*static_cast<typename std::remove_reference<F>::type*>(context))(screen, advance);" in body
assert "#include <type_traits>" in source
assert source.count("drawTwoBand([&](") >= 20
print("PASS: one shared multi-band render core, synchronous callback thunks, unchanged physical panel viewport/push")
