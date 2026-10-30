#!/usr/bin/env python3
"""Render key UI screens and catch blank or wrong-size regressions."""
import os, re, struct, subprocess, tempfile, zlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
BIN=HERE/"squachsim"
CASES=[
 ("clear","clear",["--bg","7","--outfit","13","--frames","60"],(320,240)),
 ("stripes-jail-ball","clear",["--jail","--pet","1","--frames","8"],(320,240)),
 ("stripes-jail-ball-portrait","clear",["--jail","--pet","1","--frames","8","--portrait"],(240,320)),
 ("camera-alert","alert",["--alert","9","--frames","6"],(320,240)),
 ("settings","settings",[],(320,240)),
 ("alert-rules","alertrules",[],(320,240)),
 ("log","log",[],(320,240)),
 ("log-portrait","log",["--portrait"],(240,320)),
 ("log-events","log",["--pose","1"],(320,240)),
 ("log-events-portrait","log",["--pose","1","--portrait"],(240,320)),
 ("colorcheck","colorcheck",[],(320,240)),
 ("solo-default","solo",["--outfit","0","--frames","0"],(320,240)),
 ("solo-legend","solo",["--outfit","14","--frames","0"],(320,240)),
 ("solo-stripes-portrait","solo",["--outfit","19","--portrait","--frames","0"],(240,320)),
 ("camera-alert-portrait","alert",["--alert","9","--portrait","--frames","6"],(240,320)),
]
# Every costume is a separate smoke case in portrait and landscape.
# The count is read from firmware source rather than copied as a constant,
# so adding a 21st costume automatically extends this regression gate.
source = (HERE.parent / "src" / "squachy.cpp").read_text(encoding="utf-8")
start = source.index("static const OutfitDef OUTFITS[]")
block = source[start:source.index("};", start)]
outfit_names = re.findall(r'^\s*\{\s*"([^"]+)"\s*,', block, re.MULTILINE)
assert len(outfit_names) >= 20, "PR #121 must retain all 20 costumes"
for i, name in enumerate(outfit_names):
    for portrait in (False, True):
        variant = "portrait" if portrait else "landscape"
        options = ["--bg", "7", "--outfit", str(i), "--frames", "6"]
        if portrait:
            options.append("--portrait")
        CASES.append((f"costume-{i:02d}-{variant}", "clear", options,
                      (240, 320) if portrait else (320, 240)))

def stats(path):
    d=path.read_bytes()
    assert d[:8]==b"\x89PNG\r\n\x1a\n", f"{path}: not PNG"
    p=8; w=h=None; chunks=bytearray()
    while p+12<=len(d):
        n=struct.unpack(">I",d[p:p+4])[0]; tag=d[p+4:p+8]; body=d[p+8:p+8+n]; p+=12+n
        if tag==b"IHDR": w,h=struct.unpack(">II",body[:8])
        elif tag==b"IDAT": chunks+=body
        elif tag==b"IEND": break
    raw=zlib.decompress(bytes(chunks)); stride=w*3+1
    assert len(raw)==stride*h
    colors=set(); nonblack=0
    for y in range(h):
        row=raw[y*stride:(y+1)*stride]
        assert row[0]==0
        for x in range(w):
            px=row[1+x*3:1+x*3+3]; colors.add(px)
            nonblack += px != b"\x00\x00\x00"
    return (w,h),len(colors),nonblack/(w*h)
def main():
    if not BIN.exists(): raise SystemExit("run make -C sim first")
    with tempfile.TemporaryDirectory(prefix="sqw-ui-") as td:
        td=Path(td)
        for name,screen,opts,expected in CASES:
            out=td/(name+".png")
            subprocess.run([str(BIN),screen,str(out),*opts],cwd=HERE,check=True)
            size,colors,ratio=stats(out)
            assert size==expected, f"{name}: {size} != {expected}"
            assert colors>=8, f"{name}: only {colors} colours"
            assert ratio>=0.03, f"{name}: only {ratio:.1%} non-black"
            print(f"OK {name}: {size[0]}x{size[1]}, {colors} colours, {ratio:.1%} non-black")
        # Assert the bench executes and reports a finite, nonzero count.
        bench_path = td / "solo-bench.png"
        env = os.environ.copy()
        env["SQUACHSIM_BENCH"] = "1"
        result = subprocess.run(
            [str(BIN), "solo", str(bench_path), "--outfit", "18", "--frames", "0"],
            cwd=HERE, check=True, capture_output=True, text=True, env=env,
        )
        reported = re.search(r"BENCH pix ([0-9]+(?:\\.[0-9]+)?)", result.stdout)
        assert reported, f"solo benchmark did not report a count: {result.stdout!r}"
        avg = float(reported.group(1))
        assert 0 < avg < 300000, f"implausible solo pixel-write count: {avg}"
        assert stats(bench_path)[0] == (320,240), "solo benchmark PNG wrong size"
        print(f"OK solo bench: {avg:.0f} sprite writes per draw")
    print("UI screenshot regression suite passed")
if __name__=="__main__": main()
