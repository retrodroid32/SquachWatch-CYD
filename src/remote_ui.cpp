// LAB native SquachWatch USB UI model v2. This is NOT a PC LCD renderer:
// every field is copied from current firmware state, and unsupported content
// must use the authentic RGB332 MIRROR fallback. No heap or radio tasks.
#include "remote_ui.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#if defined(ARDUINO_ARCH_ESP32) && defined(CYD)
#include "security.h"
#include "detection.h"
#include "category_roster.h"
#include "privacy.h"
#include "settings.h"
#define SQW_UI_MODEL 1
#else
#define SQW_UI_MODEL 0
#endif

namespace RemoteUi {
namespace {
struct Session {
    bool enabled=false,down=false,force=false,countRequested=false,listRequested=false;
    uint8_t screen=255,kind=0,category=0,rotation=0;
    uint16_t seq=0,listSeq=0;
    uint32_t lastAt=0,lastCountsAt=0,touchAt=0;
    int16_t width=0,height=0,x=0,y=0;
    uint8_t listStage=0,listNext=0,listLimit=0,listStart=0;
    bool listPrivacy=false;
};
Session& session(){static Session s;return s;}
#if SQW_UI_MODEL
constexpr uint32_t TOUCH_TTL=1800u;
const uint8_t categories[] = {1,2,3,4,5,6,7,8,9,17,14,16};
bool send(const char* p,int n){
    if(n<=0 || Serial.availableForWrite()<n+48)return false;
    return Serial.write((const uint8_t*)p,(size_t)n)==(size_t)n;
}
const Detection* nth(const DetectionEngine& eng,DetectionType cat,
                     unsigned nthIdx,uint32_t now) {
    if(cat==DetectionType::UNKNOWN)return eng.logAt((uint8_t)nthIdx);
    unsigned at=0;
    for(int pass=0;pass<2;pass++)
      for(uint8_t i=0;i<eng.logCount();i++) {
        const Detection* d=eng.logAt(i);
        if(d && CategoryRoster::matches(cat,d->type) &&
           CategoryRoster::fresh(*d,now)==(pass==0)){
            if(at++==nthIdx)return d;
        }
      }
    return nullptr;
}
uint8_t total(const DetectionEngine& eng,DetectionType cat) {
    if(cat==DetectionType::UNKNOWN)return eng.logCount();
    uint8_t n=0;
    for(uint8_t i=0;i<eng.logCount();i++){
        const Detection* d=eng.logAt(i);
        if(d && CategoryRoster::matches(cat,d->type))n++;
    }
    return n;
}
bool emitCounts(Session& s,const DetectionEngine& eng,uint32_t now) {
    uint32_t fresh[(uint8_t)DetectionType::COUNT]={};
    uint32_t boot[(uint8_t)DetectionType::COUNT]={};
    for(uint8_t i=1;i<(uint8_t)DetectionType::COUNT;i++)
        boot[i]=eng.bootTypeCount((DetectionType)i);
    for(uint8_t i=0;i<eng.logCount();i++){
        const Detection* d=eng.logAt(i);
        if(d && (uint8_t)d->type<(uint8_t)DetectionType::COUNT &&
           CategoryRoster::fresh(*d,now))
           ++fresh[(uint8_t)d->type];
    }
    char out[432];int n=snprintf(out,sizeof(out),"UI COUNTS 2 %u",
                                (unsigned)s.seq);
    for(unsigned i=0;i<sizeof(categories);i++){
        const DetectionType c=(DetectionType)categories[i];
        if(c==DetectionType::IBEACON && !Settings::typeEnabled(c))continue;
        if(n<=0 || n>=int(sizeof(out))-44)return false;
        n+=snprintf(out+n,sizeof(out)-n," %u:%lu:%lu",
            (unsigned)categories[i],
            (unsigned long)CategoryRoster::countFor(c,fresh),
            (unsigned long)CategoryRoster::countFor(c,boot));
    }
    if(n<=0 || n>=int(sizeof(out))-2)return false;
    out[n++]='\n';
    if(!send(out,n))return false;
    s.countRequested=false;s.lastCountsAt=now;
    return true;
}
// Percent/space-safe hex encoding of the real name, never arbitrary text
// in a command channel; omit identifiers when privacy mode is enabled.
void nameHex(const char* name,char* out,size_t cap,bool privateMode){
    if(privateMode || !name || !name[0] || cap<3){
        out[0]='-';out[1]='\0';return;
    }
    static const char hex[]="0123456789ABCDEF";
    size_t j=0;
    for(size_t i=0; name[i] && i<19 && j+3<cap; i++){
        const unsigned c=(uint8_t)name[i];
        out[j++]=hex[c>>4];out[j++]=hex[c&15];
    }
    out[j]='\0';
}
bool listTick(Session& s,const DetectionEngine& eng,uint32_t now){
    if(Privacy::on()!=s.listPrivacy){
        s.listRequested=false;
        Serial.println("UI ERROR PRIVACY_CHANGED");
        return true;
    }
    const DetectionType cat=(DetectionType)s.category;
    char out[176];int n=0;
    if(s.listStage==0) {
        const unsigned available=total(eng,cat);
        const unsigned count=s.listStart>=available?0:
           ((available-s.listStart)<s.listLimit?(available-s.listStart):s.listLimit);
        s.listLimit=(uint8_t)count;
        n=snprintf(out,sizeof(out),"UI LIST 2 %u %u %u %u %u\n",
           (unsigned)s.listSeq,(unsigned)s.category,(unsigned)s.listStart,
           available,count);
    }else if(s.listNext<s.listLimit){
        const unsigned index=(unsigned)s.listStart+s.listNext;
        const Detection* d=nth(eng,cat,index,now);
        if(!d) {
            s.listRequested=false;
            Serial.println("UI ERROR LIST_CHANGED");
            return true;
        }
        char mac[20];Privacy::mac(mac,sizeof(mac),d->mac);
        char name[44];nameHex(d->name,name,sizeof(name),s.listPrivacy);
        // firstSeen is raw: millis() for this session, epoch for
        // restored older records; the 'restored' flag disambiguates.
        n=snprintf(out,sizeof(out),
            "UI ROW 2 %u %u %u %u %d %lu %lu %u %s %s\n",
            (unsigned)s.listSeq,index,(unsigned)d->type,
            (unsigned)CategoryRoster::fresh(*d,now),(int)d->rssi,
            (unsigned long)(d->restored?0:(now-d->lastSeen)/1000u),
            (unsigned long)d->firstSeen,(unsigned)d->restored,mac,name);
    }else{
        n=snprintf(out,sizeof(out),"UI END 2 %u\n",(unsigned)s.listSeq);
    }
    if(n<=0||n>=int(sizeof(out)) || !send(out,n))return false;
    if(s.listStage==0)s.listStage=1;
    else if(s.listNext<s.listLimit)++s.listNext;
    else s.listRequested=false;
    return true;
}
#endif
} // namespace

void stop(){session()=Session{};}

bool command(const char* line) {
    if(!line || strncmp(line,"UI ",3)!=0)return false;
#if SQW_UI_MODEL
    if(strcmp(line,"UI HELLO")==0){
        Serial.println("UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL");
        return true;
    }
    Session& s=session();
    if(strcmp(line,"UI OFF")==0){
        stop();Serial.println("UI OFF OK");return true;
    }
    if(strcmp(line,"UI ON")==0){
        if(Security::locked()){Serial.println("UI ERROR LOCKED");return true;}
        stop();s.enabled=true;s.force=true;
        Serial.println("UI READY 2 SQUACHWATCH TOUCH PARTIAL");
        return true;
    }
    if(!s.enabled){Serial.println("UI ERROR DISABLED");return true;}
    if(strcmp(line,"UI GET")==0){s.force=true;return true;}
    if(strcmp(line,"UI COUNTS")==0){s.countRequested=true;return true;}
    unsigned start=0,limit=0;char extra=0;
    if(sscanf(line,"UI LIST %u %u %c",&start,&limit,&extra)==2){
        if(s.listRequested){Serial.println("UI ERROR BUSY");return true;}
        if(s.screen!=3 || s.kind!=2){
            Serial.println("UI ERROR UNSUPPORTED_SCREEN");return true;
        }
        if(start>63 || limit<1 || limit>8){
            Serial.println("UI ERROR LIST_RANGE");return true;
        }
        s.listRequested=true;s.listStage=0;s.listStart=(uint8_t)start;
        s.listLimit=(uint8_t)limit;s.listNext=0;s.listPrivacy=Privacy::on();
        s.listSeq=(uint16_t)(s.listSeq+1u);if(!s.listSeq)s.listSeq=1;
        return true;
    }
    int x=0,y=0;char phase[8]={};
    if(sscanf(line,"UI TOUCH %d %d %7s",&x,&y,phase)==3){
        if(strcmp(phase,"UP")==0){
            s.down=false;Serial.println("UI TOUCH OK");return true;
        }
        if(x<0||y<0||x>=s.width||y>=s.height){
            Serial.println("UI ERROR TOUCH_BOUNDS");return true;
        }
        if(strcmp(phase,"DOWN")==0 || (strcmp(phase,"MOVE")==0 && s.down)){
            s.x=x;s.y=y;s.down=true;s.touchAt=millis();
            if(strcmp(phase,"DOWN")==0)Serial.println("UI TOUCH OK");
            return true;
        }
        Serial.println("UI ERROR TOUCH_PHASE");return true;
    }
    Serial.println("UI ERROR COMMAND");return true;
#else
    Serial.println("UI ERROR UNSUPPORTED_BOARD");return true;
#endif
}

bool touch(int w,int h,int& x,int& y) {
#if SQW_UI_MODEL
    Session& s=session();
    if(Security::locked()) {stop();return false;}
    if(!s.enabled||!s.down)return false;
    if((uint32_t)(millis()-s.touchAt)>TOUCH_TTL ||
       s.x<0||s.y<0||s.x>=w||s.y>=h){s.down=false;return false;}
    x=s.x;y=s.y;return true;
#else
    (void)w;(void)h;(void)x;(void)y;return false;
#endif
}

void tick(uint8_t screen,int w,int h,uint8_t rotation,
          const DetectionEngine& eng,uint8_t logCategory) {
#if SQW_UI_MODEL
    Session& s=session();
    if(Security::locked()){stop();return;}
    if(!s.enabled)return;
    if(w<1||h<1||w>480||h>480)return;
    const uint32_t now=millis();
    const uint8_t kind=screen==1?1:screen==3?2:0;
    if(s.screen!=screen||s.width!=w||s.height!=h||s.rotation!=rotation||
       s.category!=logCategory||s.kind!=kind){
        s.screen=screen;s.width=w;s.height=h;s.rotation=rotation;
        s.category=logCategory;s.kind=kind;s.force=true;
        s.listRequested=false; // no row stream across screen/rotation changes
        s.countRequested=(kind==1);
    }
    // One bounded record per loop; no interleaved snapshot lines.
    if(s.listRequested){listTick(s,eng,now);return;}
    if((uint32_t)(now-s.lastAt)>=200u){
        char out[96];
        const uint16_t next=s.seq==65535?1:(uint16_t)(s.seq+1);
        const int n=snprintf(out,sizeof(out),
           "UI STATE 2 %u %u %d %d %u %lu %u %u\n",
           (unsigned)next,(unsigned)screen,w,h,(unsigned)rotation,
           (unsigned long)now,(unsigned)Privacy::on(),(unsigned)kind);
        if(n>0 && n<int(sizeof(out)) && send(out,n)){
            s.seq=next;s.lastAt=now;s.force=false;
        }
        return;
    }
    if(kind==1 && (s.countRequested || (uint32_t)(now-s.lastCountsAt)>=1000u))
        emitCounts(s,eng,now);
#else
    (void)screen;(void)w;(void)h;(void)rotation;(void)eng;(void)logCategory;
#endif
}
} // namespace RemoteUi
