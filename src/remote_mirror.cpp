// SquachWatch-CYD USB workbench, authentic RGB332 row capture.
// No TFT GRAM readback and no reconstructed UI state. Every transmitted row
// is copied from the 8-bit sprite immediately associated with a panel push.
// Captures are bounded: two changed rows per rendering pass, zero heap allocs.
// The host applies updates only after an END with a matching sequence.
#include "remote_mirror.h"
#include "security.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32) && (defined(CYD) || defined(CYD32))
#define SQW_CLASSIC_MIRROR 1
#else
#define SQW_CLASSIC_MIRROR 0
#endif

namespace RemoteMirror {
namespace {
constexpr int MAX_H = 480;
constexpr int MAX_W = 320;
constexpr int MAX_SENT_PER_PASS = 2;
constexpr int MAX_SCANNED_PER_PASS = 32;
constexpr uint32_t TOUCH_EXPIRY_MS = 1800;
struct Session {
    bool enabled=false, paused=false, pending=false, running=false;
    bool delta=false, baselineValid=false, down=false;
    int x=0,y=0, width=0,height=0,nextRow=0;
    uint32_t touchAt=0, sequence=0, committed=0, baseRequested=0;
    uint32_t lastProgressAt=0;
};
Session& session() { static Session s; return s; }
// Hashes cost 3840 bytes regardless of orientation or frame-band count;
// never allocate a second 76.8KB LCD image in already-constrained CYD RAM.
uint32_t baselineHash[MAX_H] = {};
uint32_t pendingHash[MAX_H] = {};
const char B64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint32_t hashRow(const uint8_t* p,int n) {
    uint32_t h=2166136261u;
    for (int i=0;i<n;i++) { h^=p[i]; h*=16777619u; }
    return h;
}
bool sendRecord(const char* p,int n) {
#if SQW_CLASSIC_MIRROR
    // Never stall BLE/WiFi/GPS on the USB TX queue. Each complete record
    // enters HardwareSerial with one write, so LOG lines cannot splice into
    // an unfinished BASE64 row.
    if (n<=0 || Serial.availableForWrite()<n+32) return false;
    return Serial.write((const uint8_t*)p,(size_t)n)==(size_t)n;
#else
    (void)p; (void)n;
    return false;
#endif
}
void resetPending(Session& s) {
    s.pending=false; s.running=false; s.nextRow=0;
}
void fail(Session& s,const char* why) {
    resetPending(s);
    s.baselineValid=false;
    Serial.printf("MIRROR ERROR %s\n",why);
}
int encodeRow(char* out,size_t cap,uint32_t seq,int y,const uint8_t* src,int width) {
    int n=snprintf(out,cap,"MIRROR ROW %lu %d ",(unsigned long)seq,y);
    if (n<=0 || n>=int(cap)) return 0;
    for(int i=0;i<width;i+=3) {
        if(n+5>=int(cap)) return 0;
        const unsigned a=src[i],b=i+1<width?src[i+1]:0,c=i+2<width?src[i+2]:0;
        out[n++]=B64[a>>2]; out[n++]=B64[((a&3)<<4)|(b>>4)];
        out[n++]=(i+1<width)?B64[((b&15)<<2)|(c>>6)]:'=';
        out[n++]=(i+2<width)?B64[c&63]:'=';
    }
    out[n++]='\n';
    return n;
}
} // namespace

bool command(const char* line) {
    if(!line || strncmp(line,"MIRROR",6)!=0) return false;
    Session& s=session();
    if(strcmp(line,"MIRROR HELLO")==0 || strcmp(line,"MIRROR CAPS")==0) {
#if SQW_CLASSIC_MIRROR
        Serial.println("MIRROR CAPS 2 RGB332 FULL DELTA ROWS TOUCH PAUSE NO_FAKE_USB");
#else
        Serial.println("MIRROR CAPS 2 UNSUPPORTED_BOARD");
#endif
        return true;
    }
    if(strcmp(line,"MIRROR OFF")==0) {
        s=Session{};
        Serial.println("MIRROR OFF OK");
        return true;
    }
    if(strcmp(line,"MIRROR ON")==0) {
#if SQW_CLASSIC_MIRROR
        s=Session{};
        s.enabled=true;
        // Legacy PR #225 READY prefix remains compatible with the EXE's
        // existing receiver handshake, which expects MIRROR READY.
        Serial.println("MIRROR READY RGB332 320x240/240x320 TOUCH DELTA V2");
#else
        Serial.println("MIRROR ERROR UNSUPPORTED_BOARD");
#endif
        return true;
    }
    if(!s.enabled) { Serial.println("MIRROR ERROR DISABLED"); return true; }
    if(strcmp(line,"MIRROR PAUSE")==0) {
        resetPending(s); s.paused=true; s.down=false; s.baselineValid=false;
        Serial.println("MIRROR PAUSED");
        return true;
    }
    if(strcmp(line,"MIRROR RESUME")==0) {
        resetPending(s); s.paused=false; s.baselineValid=false;
        Serial.println("MIRROR RESUMED");
        return true;
    }
    if(s.paused) { Serial.println("MIRROR ERROR PAUSED"); return true; }
    if(strcmp(line,"MIRROR FRAME")==0 || strncmp(line,"MIRROR FRAME ",13)==0) {
        if(s.running || s.pending) { Serial.println("MIRROR ERROR BUSY"); return true; }
        unsigned long last=0; char extra=0;
        const bool withBase=strncmp(line,"MIRROR FRAME ",13)==0;
        if(withBase && sscanf(line+13,"%lu %c",&last,&extra)!=1) {
            Serial.println("MIRROR ERROR BASE_SEQUENCE"); return true;
        }
        s.baseRequested=withBase?(uint32_t)last:0;
        s.delta=withBase && s.baselineValid && s.baseRequested==s.committed;
        s.pending=true; s.lastProgressAt=millis();
        return true;
    }
    int x=0,y=0; char phase[8]={};
    if(sscanf(line,"MIRROR TOUCH %d %d %7s",&x,&y,phase)==3 &&
       x>=0 && x<480 && y>=0 && y<480) {
        if(strcmp(phase,"UP")==0) {
            s.down=false;
        }else if(strcmp(phase,"DOWN")==0 || (strcmp(phase,"MOVE")==0 && s.down)) {
            s.down=true; s.x=x; s.y=y; s.touchAt=millis();
        }else {
            Serial.println("MIRROR ERROR TOUCH_PHASE"); return true;
        }
        // MOVE traffic can arrive 10 times/second; avoid echoing every move
        // onto the shared receiver LOG/evidence serial channel.
        if(strcmp(phase,"MOVE")!=0) Serial.println("MIRROR TOUCH OK");
        return true;
    }
    Serial.println("MIRROR ERROR COMMAND");
    return true;
}
bool touch(int w,int h,int& x,int& y) {
#if SQW_CLASSIC_MIRROR
    Session& s=session();
    if(Security::locked()) {s.down=false;return false;}
    if(!s.enabled || s.paused || !s.down) return false;
    if((uint32_t)(millis()-s.touchAt)>TOUCH_EXPIRY_MS ||
       s.x>=w || s.y>=h) {s.down=false;return false;}
    x=s.x; y=s.y; return true;
#else
    (void)w;(void)h;(void)x;(void)y;return false;
#endif
}
void pauseEvidence() {
    Session& s=session();
    if(!s.enabled) return;
    if(s.pending || s.running) fail(s,"EVIDENCE_PAUSE");
    s.paused=true; s.down=false; s.baselineValid=false;
}
void unavailable() {
    Session& s=session();
    if(s.enabled && (s.pending||s.running)) fail(s,"NO_AUTHENTIC_BUFFER");
}
void tick() {
#if SQW_CLASSIC_MIRROR
    Session& s=session();
    if(!s.enabled || s.paused || (!s.pending && !s.running)) return;
    // A direct-panel render or an indefinitely congested UART can never
    // become an authentic LCD frame. Fail explicitly instead of leaving
    // the EXE waiting or showing the last image as if it were live.
    if((uint32_t)(millis()-s.lastProgressAt)>6000u)
        fail(s,"NO_FRAME_REFRESH_OR_TX");
#endif
}
void capture(const uint8_t* src,int w,int bandH,int panelH,int y) {
#if SQW_CLASSIC_MIRROR
    Session& s=session();
    if(!s.enabled || s.paused || (!s.pending && !s.running)) return;
    if(Security::locked()) {
        s.down=false; fail(s,"LOCKED"); s.enabled=false; return;
    }
    if(!src || w<1 || w>MAX_W || bandH<1 || panelH<1 || panelH>MAX_H ||
       y<0 || bandH>panelH || y+bandH>panelH) {
        fail(s,"BAD_SPRITE_GEOMETRY");return;
    }
    if(!s.running) {
        if(y!=0) return; // only a full screen top band can begin keyframe
        if(s.width!=w || s.height!=panelH) {
            s.delta=false; s.baselineValid=false;
        }
        uint32_t next=s.sequence+1u;
        if(!next) next=1;
        char rec[100];
        const int n=s.delta?
            snprintf(rec,sizeof(rec),"MIRROR DELTA %lu %lu %d %d RGB332\n",
                     (unsigned long)next,(unsigned long)s.committed,w,panelH):
            snprintf(rec,sizeof(rec),"MIRROR BEGIN %lu %d %d RGB332\n",
                     (unsigned long)next,w,panelH);
        if(n<=0 || n>=int(sizeof(rec)) || !sendRecord(rec,n)) return;
        s.sequence=next; s.width=w; s.height=panelH;
        s.pending=false; s.running=true; s.nextRow=0;
        s.lastProgressAt=millis();
    }
    if(w!=s.width || panelH!=s.height) {fail(s,"ROTATION_RESTART");return;}
    // A half-screen sprite is reused for many bands. Never read a stale band:
    // process the next row only when its pixels are currently in this sprite.
    // If the last row was already queued but END lacked TX space, the
    // following push must still be allowed to complete the transaction.
    if(s.nextRow==s.height) {
        char end[64];
        const int n=snprintf(end,sizeof(end),"MIRROR END %lu\n",
                             (unsigned long)s.sequence);
        if(!sendRecord(end,n)) return;
        memcpy(baselineHash,pendingHash,(size_t)s.height*sizeof(uint32_t));
        s.committed=s.sequence; s.baselineValid=true; resetPending(s);
        return;
    }
    if(s.nextRow<y || s.nextRow>=y+bandH) return;
    int sent=0,scanned=0;
    while(s.nextRow<panelH && s.nextRow<y+bandH &&
          sent<MAX_SENT_PER_PASS && scanned<MAX_SCANNED_PER_PASS) {
        const int row=s.nextRow, inBand=row-y;
        const uint8_t* bytes=src+(size_t)inBand*(size_t)w;
        const uint32_t hash=hashRow(bytes,w);
        if(!s.delta || hash!=baselineHash[row]) {
            char record[480];
            const int n=encodeRow(record,sizeof(record),s.sequence,row,bytes,w);
            if(!n) {fail(s,"ENCODE_OVERFLOW");return;}
            if(!sendRecord(record,n)) return; // next drawn push resumes this row
            sent++; s.lastProgressAt=millis();
        }
        pendingHash[row]=hash;
        ++s.nextRow; ++scanned;
        s.lastProgressAt=millis();
    }
    if(s.nextRow==s.height) {
        char end[64];
        int n=snprintf(end,sizeof(end),"MIRROR END %lu\n",
                       (unsigned long)s.sequence);
        if(!sendRecord(end,n)) return;
        memcpy(baselineHash,pendingHash,(size_t)s.height*sizeof(uint32_t));
        s.committed=s.sequence; s.baselineValid=true; resetPending(s);
    }
#else
    (void)src;(void)w;(void)bandH;(void)panelH;(void)y;
#endif
}
} // namespace RemoteMirror
