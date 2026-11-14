#include "remote_mirror.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace RemoteMirror {
namespace {
struct State {
  bool enabled=false,requested=false,streaming=false,down=false;
  bool uiEnabled=false,uiRequested=false;
  uint8_t uiScreen=255,uiRotation=255;
  uint32_t lastUiAt=0;
  int uiWidth=0,uiHeight=0;
  int x=0,y=0,width=0,height=0,nextY=0;
  uint32_t lastTouch=0,seq=0;
};
State& state(){static State s;return s;}
static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void rowBase64(const uint8_t* src,int count) {
  // 320 RGB332 pixels => 428 base64 characters. No large frame allocation.
  char buf[436];int n=0;
  for(int i=0;i<count;i+=3){
    const unsigned a=src[i],b=(i+1<count?src[i+1]:0),c=(i+2<count?src[i+2]:0);
    buf[n++]=alphabet[a>>2];
    buf[n++]=alphabet[((a&3)<<4)|(b>>4)];
    buf[n++]=(i+1<count)?alphabet[((b&15)<<2)|(c>>6)]:'=';
    buf[n++]=(i+2<count)?alphabet[c&63]:'=';
  }
  Serial.write((const uint8_t*)buf,n);
}
}
bool command(const char* line){
  if(!line||strncmp(line,"MIRROR",6)!=0)return false;
  State& s=state();
  if(strcmp(line,"MIRROR ON")==0){
    s.enabled=true;s.requested=false;s.streaming=false;s.down=false;
    Serial.println("MIRROR READY RGB332 320x240/240x320 TOUCH");
    return true;
  }
  if(strcmp(line,"MIRROR OFF")==0){
    s.enabled=false;s.requested=false;s.streaming=false;s.down=false;
    Serial.println("MIRROR OFF OK");return true;
  }
  if(!s.enabled){
    Serial.println("MIRROR ERROR DISABLED");return true;
  }
  if(strcmp(line,"MIRROR FRAME")==0){
    // Do not queue multiple requests: captured only at the actual LCD push.
    if(!s.streaming)s.requested=true;
    return true;
  }
  int x=0,y=0;char phase[8]={0};
  if(sscanf(line,"MIRROR TOUCH %d %d %7s",&x,&y,phase)==3 &&
     x>=0 && x<480 && y>=0 && y<480){
    if(strcmp(phase,"UP")==0){s.down=false;}
    else if(strcmp(phase,"DOWN")==0||strcmp(phase,"MOVE")==0){
      s.x=x;s.y=y;s.down=true;s.lastTouch=millis();
    }else{Serial.println("MIRROR ERROR TOUCH_PHASE");return true;}
    Serial.println("MIRROR TOUCH OK");return true;
  }
  Serial.println("MIRROR ERROR COMMAND");return true;
}
// SquachEmit-compatible low-bandwidth UI state transport for SquachWatch.
// This is a capability/telemetry foundation, NOT a complete Windows renderer.
// Command handling is reached only after Clock::pollSerial's PIN check.
bool uiCommand(const char* line){
  if(!line||strncmp(line,"UI ",3)!=0)return false;
  State& s=state();
  if(strcmp(line,"UI ON")==0){
    s.uiEnabled=true;s.uiRequested=true;s.lastUiAt=0;
    Serial.println("UI READY 1 SQUACHWATCH TOUCH STATE");
    return true;
  }
  if(strcmp(line,"UI OFF")==0){
    s.uiEnabled=false;s.uiRequested=false;
    if(!s.enabled)s.down=false;
    Serial.println("UI OFF OK");
    return true;
  }
  if(!s.uiEnabled){
    Serial.println("UI ERROR DISABLED");
    return true;
  }
  if(strcmp(line,"UI GET")==0){s.uiRequested=true;return true;}
  int x=0,y=0;char phase[8]={0};
  if(sscanf(line,"UI TOUCH %d %d %7s",&x,&y,phase)==3 &&
     x>=0 && y>=0 && x<s.uiWidth && y<s.uiHeight){
    if(strcmp(phase,"UP")==0)s.down=false;
    else if(strcmp(phase,"DOWN")==0||strcmp(phase,"MOVE")==0){
      s.x=x;s.y=y;s.down=true;s.lastTouch=millis();
    }else{Serial.println("UI ERROR TOUCH_PHASE");return true;}
    Serial.println("UI TOUCH OK");return true;
  }
  Serial.println("UI ERROR COMMAND");return true;
}
void uiTick(uint8_t screen,int width,int height,uint8_t rotation,bool locked){
  State& s=state();
  // A lock acquired after UI ON must also revoke a live session.
  if(locked){
    s.uiEnabled=false;s.uiRequested=false;s.enabled=false;
    s.requested=false;s.streaming=false;s.down=false;
    return;
  }
  if(!s.uiEnabled)return;
  if(width<1||height<1||width>480||height>480)return;
  s.uiWidth=width;s.uiHeight=height;
  const uint32_t now=millis();
  if(!s.uiRequested&&now-s.lastUiAt<200)return; // <=5 updates/s
  if(Serial.availableForWrite()<64)return; // Don't block the radio/UI loop.
  s.uiRequested=false;s.lastUiAt=now;
  s.uiScreen=screen;s.uiRotation=rotation;
  // Numeric screen ID is stable AppState; no screen text, PIN, MAC or
  // potentially sensitive detection details are serialized.
  Serial.printf("UI STATE 1 %u %d %d %u %lu\\n",
    (unsigned)screen,width,height,(unsigned)rotation,(unsigned long)now);
}
bool touch(int width,int height,int& x,int& y){
  State& s=state();
  if((!s.enabled&&!s.uiEnabled)||!s.down)return false;
  if(millis()-s.lastTouch>4000){s.down=false;return false;}
  if(s.x>=width||s.y>=height){s.down=false;return false;}
  x=s.x;y=s.y;return true;
}
bool requested(){return state().enabled&&state().requested;}
void capture(const uint8_t* src,int width,int bandHeight,int panelHeight,int y){
  State& s=state();
  if(!s.enabled||(!s.requested&&!s.streaming)||!src)return;
  if(width<1||width>320||bandHeight<1||panelHeight<1||
     bandHeight>panelHeight||y<0||y+bandHeight>panelHeight)return;
  if(!s.streaming){
    // A logical frame must start at top, otherwise the host would
    // display half a frame from different LCD refresh generations.
    if(y!=0)return;
    s.streaming=true;s.requested=false;s.seq++;
    s.width=width;s.height=panelHeight;s.nextY=0;
    Serial.printf("MIRROR BEGIN %lu %d %d RGB332\n",
      (unsigned long)s.seq,width,panelHeight);
  }
  if(width!=s.width||panelHeight!=s.height||y!=s.nextY){
    Serial.printf("MIRROR ERROR FRAME_ORDER %lu\n",(unsigned long)s.seq);
    s.streaming=false;return;
  }
  for(int row=0;row<bandHeight;row++){
    Serial.printf("MIRROR ROW %lu %d ",(unsigned long)s.seq,y+row);
    rowBase64(src+(size_t)row*width,width);
    Serial.println();
  }
  s.nextY+=bandHeight;
  if(s.nextY>=s.height){
    Serial.printf("MIRROR END %lu\n",(unsigned long)s.seq);
    s.streaming=false;
  }
}
}
