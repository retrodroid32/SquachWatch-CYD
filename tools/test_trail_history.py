#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = r"""
#include "trail_history.h"
#include <cassert>
#include <cstring>
int main() {
    using namespace TrailHistory;
    uint8_t mac[6]={1,2,3,4,5,6};
    clear();
    assert(Places::current()==0);
    note(mac,1,0);
    assert(!cache().find(mac,1));
    Places::forceNewAt(0,0,0);
    assert(Places::exists(Places::current()));
    note(mac,1,10); note(mac,1,20);
    assert(cache().find(mac,1)->count==1);
    Places::noteFix(0,1000,1000);
    assert(Places::current()==1);
    Places::noteFix(0,100000,60000);
    note(mac,1,60000);
    assert(cache().find(mac,1)->count==2);
    assert(Places::distanceM(1,2)>300);
    Places::forceNew(120000); note(mac,1,120000);
    assert(cache().find(mac,1)->count==3);
    Places::forceNew(180000); note(mac,1,180000);
    assert(cache().find(mac,1)->count==3);
    assert(cache().find(mac,1)->stop[0]==2);
    assert(strstr(describe(mac,1,false,false),"3 recent stops"));
    assert(strstr(describe(mac,1,true,false),"hidden"));
    assert(strstr(describe(mac,1,false,true),"Historical record"));
    for (unsigned i=0;i<8;i++) {
        uint8_t other[6]={uint8_t(i+10),0,0,0,0,0};
        note(other,1,200000+i);
    }
    assert(!cache().find(mac,1));
    clear();
    assert(Places::current()==0);
    assert(!cache().find(mac,1));
    Places::forceNewAt(1,0,1799990000);
    Places::forceNewAt(2,0,-1799990000);
    assert(Places::distanceM(1,2)<300);
}
"""
with tempfile.TemporaryDirectory() as td:
    p=Path(td)
    (p/"trail.cpp").write_text(source)
    subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror","-I",str(root/"include"),
        str(p/"trail.cpp"),str(root/"src/places.cpp"),"-o",str(p/"trail")],check=True)
    subprocess.run([str(p/"trail")],check=True)
s=(root/"src/main.cpp").read_text()
d=(root/"src/detection.cpp").read_text()
gps=(root/"src/gps_runtime.cpp").read_text()
assert s.count("s_infoPage = InfoPage::TRAIL;")==2
assert s.count("Settings::privacyMode() || Security::locked(),s_confirmDetection.restored")==2
assert "TrailHistory::clear();" in d.split("void DetectionEngine::clearLog() {",1)[1]
assert "Places::noteBssid" in d.split("void DetectionEngine::processWiFiQ",1)[1]
assert "Gnss::fresh(nowMs) && !Gnss::faked()" in gps
print("Trail stop history, bounded cache, geographic arithmetic and privacy guards PASS")
