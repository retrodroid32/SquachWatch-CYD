#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r"""
#include "alert_queue.h"
#include <cassert>
enum class Kind : unsigned char { A, B };
int main() {
    AlertQueue<Kind,4> q;
    AlertQueue<Kind,4>::Entry e;
    unsigned char a[6]={1}, b[6]={2}, c[6]={3}, d[6]={4};
    assert(!q.pop(e));
    assert(q.push(a,Kind::A));
    assert(!q.push(a,Kind::A));
    assert(q.push(a,Kind::B));
    assert(q.push(b,Kind::A));
    assert(q.push(c,Kind::A));
    assert(!q.push(d,Kind::A));
    assert(q.size()==4);
    assert(q.pop(e) && e.mac[0]==1 && e.type==Kind::A);
    assert(q.pop(e) && e.mac[0]==1 && e.type==Kind::B);
    assert(q.push(d,Kind::A));
    assert(q.pop(e) && e.mac[0]==2);
    assert(q.pop(e) && e.mac[0]==3);
    assert(q.pop(e) && e.mac[0]==4);
    assert(!q.pop(e));
    assert(q.push(a,Kind::A));
    q.clear();
    assert(q.size()==0 && !q.pop(e));
}
"""
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    (p / "queue.cpp").write_text(source)
    subprocess.run(["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "include"), str(p / "queue.cpp"),
                    "-o", str(p / "queue")], check=True)
    subprocess.run([str(p / "queue")], check=True)
s = (root / "src/main.cpp").read_text()
admit = s.split("static void collectQueuedAlert(uint32_t now) {",1)[1].split("static bool nextQueuedAlert()",1)[0]
pop = s.split("static bool nextQueuedAlert() {",1)[1].split("static void acknowledgeAlert()",1)[0]
assert "alertMayInterrupt(" not in admit
assert "IgnoreList::silenced" in admit
assert "Security::locked()" in admit and "Security::locked()" in pop
assert "alertMayInterrupt(*d)" in pop and "engine.findDetection(q.mac,q.type)" in pop
assert "s_alertQueue.clear(); // explicit HUNT owns the next screen" in s
assert "s_alertQueue.clear();" in s.split("static void enterLocked() {",1)[1]
assert "uiAlertSetQueue(Security::locked() ? 0 : s_alertQueue.size())" in s
print("Alert queue FIFO, deduplication, capacity and policy integration PASS")
