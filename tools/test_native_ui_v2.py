#!/usr/bin/env python3
"""CI integration guards for SquachWatch-only native UI model v2.

Source/contract assertions; firmware compilation and physical USB tests remain
separate merge gates. Never call these host checks an LCD fidelity test.
"""
import re
from pathlib import Path
r=Path(__file__).resolve().parents[1]
read=lambda p:(r/p).read_text(encoding='utf8')
ui=read('src/remote_ui.cpp')
h=read('include/remote_ui.h')
main=read('src/main.cpp')
clock=read('src/clock.cpp')
mirror=read('src/remote_mirror.cpp')
sim=read('sim/Makefile')
doc=read('docs/LAB_NATIVE_UI_STATE_USB_V2.md')
assert 'UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL' in ui
assert 'UI READY 2 SQUACHWATCH TOUCH PARTIAL' in ui
for cmd in ('UI HELLO','UI ON','UI GET','UI COUNTS','UI OFF',
            'UI LIST %u %u %c','UI TOUCH %d %d %7s'):
    assert cmd in ui,cmd
for record in ('UI STATE 2 %u %u %d %d %u %lu %u %u',
               'UI COUNTS 2 %u','UI LIST 2 %u %u %u %u %u',
               'UI ROW 2 %u %u %u %u %d %lu %lu %u %s %s',
               'UI END 2 %u'):
    assert record in ui,record
assert 'constexpr uint32_t TOUCH_TTL=1800u' in ui
assert '(strcmp(phase,"MOVE")==0 && s.down)' in ui
assert 'strcmp(phase,"UP")==0' in ui
assert 'Security::locked()' in ui
assert 'Privacy::mac(mac,sizeof(mac),d->mac);' in ui
assert 'nameHex(d->name,name,sizeof(name),s.listPrivacy)' in ui
assert "if(privateMode || !name || !name[0]" in ui
assert 'Privacy::on()!=s.listPrivacy' in ui
assert 'UI ERROR PRIVACY_CHANGED' in ui
assert 'UI ERROR UNSUPPORTED_SCREEN' in ui
assert 'const uint8_t kind=screen==1?1:(screen==3 && logCategory!=255)?2:0;' in ui
assert 'CategoryRoster::countFor(c,fresh)' in ui
assert 'CategoryRoster::countFor(c,boot)' in ui
assert 'CategoryRoster::fresh(*d,now)' in ui
assert 'eng.bootTypeCount(' in ui
assert 'eng.logAt(' in ui
assert 'd->firstSeen' in ui and 'd->restored' in ui
assert 's.listLimit=(uint8_t)limit' in ui
assert 'start>63 || limit<1 || limit>8' in ui
assert 'Serial.availableForWrite()<n+48' in ui
assert '(uint32_t)(now-s.lastAt)>=200u' in ui
assert '(uint32_t)(now-s.lastCountsAt)>=1000u' in ui
assert 'if(s.listRequested){listTick(s,eng,now);return;}' in ui
assert 'RemoteUi::stop();' in clock and 'RemoteMirror::stopForNative();' in clock
assert clock.index('if (Security::locked())') < clock.index('RemoteUi::command(line)')
assert 'void stopForNative() { session()=Session{}; }' in mirror
assert 'if(RemoteMirror::touch(' in main and 'RemoteUi::touch(' in main
assert main.index('if (!readTouchRaw(a, b))') < main.index('RemoteUi::touch(')
assert 'uiLogView()==LogView::DEVICES' in main and ': 255)' in main
assert 'RemoteUi::tick((uint8_t)state' in main
assert '$(SRC)/remote_ui.cpp' in sim
assert 'void tick(' in h
assert 'full native Windows UI parity' in doc
assert 'Missing receiver evidence is UNVERIFIED' in doc
# The type order must remain aligned with CLEAR's exact visible pill grouping.
clear=read('src/ui_clear.cpp')
section=clear.split('FIXED_COUNTER_TYPES[] = {',1)[1].split('};',1)[0]
ids={'FLOCK':1,'AXON':2,'META':3,'SKIMMER':4,'RAVEN':5,'AIRTAG':6,
     'DRONE':7,'ALPR':8,'CAMERA':9,'HACKER':17,'DEAUTH':14}
types=re.findall(r'DetectionType::([A-Z_]+)',section)
assert types==list(ids),types
category_types=re.search(r'const uint8_t categories\[\] = \{([^}]+)\}',ui).group(1)
assert [int(n) for n in category_types.split(',')]==list(ids.values())+[16]
# APPSTATE remains source of truth: do not guess screen names/IDs.
state=read('include/state.h')
assert 'enum class AppState : uint8_t' in state
assert 'SD_FILES         = 41' in state
print('PASS native v2 source/contract guards: privacy, real data, mode, touch, UART, fallbacks')
