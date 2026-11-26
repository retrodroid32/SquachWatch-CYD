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
admit = s.split("static void collectQueuedAlert(const Detection& candidate) {",1)[1].split("static bool nextQueuedAlert()",1)[0]
pop = s.split("static bool nextQueuedAlert() {",1)[1].split("static void acknowledgeAlert()",1)[0]
assert "alertMayInterrupt(" not in admit
policy = s.split("bool alertPolicyReady(",1)[1].split("bool alertMayInterrupt(",1)[0]
assert "IgnoreList::silenced" in policy
assert "alertPolicyReady(*d,always)" in admit
assert "alertPolicyReady(d,always)" in s
assert "Security::locked()" in admit and "Security::locked()" in pop
assert "alertMayInterrupt(*d)" in pop and "engine.findDetection(q.mac,q.type)" in pop
assert "s_alertQueue.clear(); // explicit HUNT owns the next screen" in s
assert "s_alertQueue.clear();" in s.split("static void enterLocked() {",1)[1]
assert "uiAlertSetQueue(Security::locked() ? 0 : s_alertQueue.size())" in s
d=(root/"src/detection.cpp").read_text()
h=(root/"include/detection.h").read_text()
assert d.count("publishAlertOpportunity(")==6
assert "if (_alertSink && !d->restored) _alertSink(*d);" in h
assert "engine.setAlertOpportunitySink((state == AppState::ALERT || state == AppState::CLEAR) ? collectQueuedAlert : nullptr);" in s
assert "if (state != AppState::ALERT) s_alertQueue.clear();" in s
assert "enterAlert(copy,true);" in pop
assert "if (!preserveQueue) s_alertQueue.clear();" in s
assert "const uint32_t alertPollNow = millis();" in s
assert "freshAlertCandidate(*latest, now)" not in s
assert "nextQueuedAlert();" in s.split("case AppState::CLEAR: {",1)[1]
for event in ("DETECTION_REGISTERED","ALERT_ELIGIBLE","ALERT_SUPPRESSED","ALERT_QUEUED","POPUP_OPENED","POPUP_DISMISSED"):
    assert event in s
assert "if (Settings::privacyMode() || Security::locked()) return;" in s
assert "engine.latest()" not in admit
assert "freshAlertCandidate(" not in admit
print("Alert queue FIFO, deduplication, capacity and per-opportunity policy integration PASS")
