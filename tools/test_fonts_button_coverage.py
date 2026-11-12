#!/usr/bin/env python3
"""All major custom-button codepaths should honor the shared five-step sizes."""
from pathlib import Path
R=Path(__file__).resolve().parents[1]
markers={
"src/theme.cpp": ["drawButtonLabel(t,x,y,w,h,label,fg,fill)",
                 "drawButtonLabel(t,x+2+ox,y+2+ox,w-4-ox,h-4-ox",
                 "drawButtonLabel(t,0,y,w,h,label,CYAN,BG)"],
"src/ui_settings.cpp": ['"[ OK ]",Theme::CYAN','"[ UP ]",Theme::CYAN',
                       "drawButtonLabel(t,x,y,w,h,lbl"],
"src/ui_phone.cpp": ["drawButtonLabel(t,kx,ky,KW,KH,lab",
                    "drawButtonLabel(t,k.x,k.y,k.w,k.h,lab",
                    "drawButtonLabel(t,kx,ky+2,KW,19"],
"src/ui_wifipass.cpp": ["drawButtonLabel(t,x,y,k.w,k.h,lab"],
"src/ui_alert.cpp": ['drawButtonLabel(t,bx,by,bw,half,"MORE"',
                     'drawButtonLabel(t,bx,by,bw,bh,"HUNT"'],
"src/ui_clear.cpp": ["buttonLabelWidth(t,txt,maxLabelW+6,bh)",
                     "drawButtonLabel(t,x+16,y,textW+6,bh"],
"src/ui_sysprops.cpp": ["drawButtonLabel(t,x+2,y+2,w-4,h-4,label",
                         "drawButtonLabel(t,tx,g.tabY+"]
}
for file,strings in markers.items():
    t=(R/file).read_text()
    for marker in strings: assert marker in t,(file,marker)
src=(R/"src/theme.cpp").read_text()
assert "static const uint8_t symbolGlyphs[][5]" in src
assert "if (c >= 'a' && c <= 'z')" in src
assert "ButtonFont::barHeight(Settings::fontStep(Settings::FontKind::BUTTONS))" in src
assert "int buttonLabelWidth(TFT_eSPI& t" in src
print("PASS: direct and custom button text uses the common 5-step glyph renderer")
