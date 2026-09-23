#!/usr/bin/env python3
"""Render key UI screens and catch blank or wrong-size regressions."""
import struct, subprocess, tempfile, zlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
BIN=HERE/"squachsim"
CASES=[
 ("clear","clear",["--bg","7","--outfit","13","--frames","60"],(320,240)),
 ("camera-alert","alert",["--alert","9","--frames","6"],(320,240)),
 ("settings","settings",[],(320,240)),
 ("log","log",[],(320,240)),
 ("colorcheck","colorcheck",[],(320,240)),
 ("camera-alert-portrait","alert",["--alert","9","--portrait","--frames","6"],(240,320)),
]
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
    print("UI screenshot regression suite passed")
if __name__=="__main__": main()
