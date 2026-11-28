#include "info_card.h"
#if SQW_INFO_CARD
#include "theme.h"
#include "clock.h"
#include "trail_history.h"
#include <Arduino.h>
#include <string.h>
namespace InfoCard {
struct Lines { const char* head; const char* tip; uint16_t a,b; };
static const Lines lines[] = {
    {"A device nothing matched.", "Seeing it everywhere you go? WATCH it.", 0x9375, 0xB5BF},
    {"License-plate camera.", "Sold for plates; police use it to track people too.", 0x4B7F, 0x937F},
    {"Police body-cam gear nearby.", "Means police gear is near, not who is filming you.", 0x4B7F, 0x6DBF},
    {"Camera glasses, maybe.", "Look for glasses with a small LED on the frame.", 0xFB75, 0xB37F},
    {"The chip inside card skimmers.", "At a pump or ATM? Don't use it. Tell the clerk.", 0xFA4A, 0xFC8A},
    {"Gunshot-detection sensor.", "It listens all the time, usually from a pole or roof.", 0x4B7F, 0x6C9F},
    {"Someone's Find My tracker.", "Not yours and been with you a while? Check bags and the car.", 0xFDA0, 0xFB75},
    {"A drone, with its ID.", "When it sends one, INFO shows where the pilot is standing.", 0x06DF, 0x6C9F},
    {"License-plate reader.", "Logs every passing plate into a shared database.", 0x4B7F, 0x937F},
    {"A camera's radio.", "In a rental? Look around the room for a lens.", 0xFB75, 0xFDB5},
    {"Someone's Samsung SmartTag.", "Not yours and been with you a while? Check bags and the car.", 0xFDA0, 0xFECA},
    {"A Find My Device tracker.", "Not yours and been with you a while? Check bags and the car.", 0xFDA0, 0x6EDF},
    {"Someone's Tile tracker.", "Not yours and been with you a while? Check bags and the car.", 0xFDA0, 0xFED5},
    {"A Ring doorbell or camera.", "Police can ask its owner for footage via Neighbors.", 0xFB75, 0x05BF},
    {"Someone kicking WiFi users off.", "WiFi may drop. Don't rejoin networks you don't know.", 0xFA4A, 0xFDA0},
    {"A fake copy of a WiFi network.", "Don't join the open one. Use your phone's data instead.", 0xFA4A, 0xB37F},
    {"A store or venue beacon.", "Only apps you installed can use it. Check app permissions.", 0x05B5, 0x6EDF},
    {"A wireless hacking tool.", "Usually a hobbyist. Watch for odd popups or WiFi drops.", 0x27F5, 0x05BF},
    {"A smart speaker with mics.", "In a rental? Ask the host about it, or unplug it.", 0xB37F, 0xFB75}
};
static_assert(sizeof(lines)/sizeof(lines[0]) == (unsigned)DetectionType::COUNT,"card category order");
static Detection row;
static const char* title;
static const char* body;
static bool hidden, enabled;
static uint32_t fedAt;
static unsigned page, pages;
static int buttonY, buttonW;
static char wrapped[32][48];
void clear(){enabled=false;page=0;}
void feed(const Detection& d,const char* name,const char* text,bool privateMode,uint32_t now){
    if(!enabled || row.type!=d.type || memcmp(row.mac,d.mac,6))page=0;
    row=d; title=name; body=text; hidden=privateMode; enabled=true; fedAt=now;
}
bool active(uint32_t now){return enabled && now==fedAt;}
static void label(TFT_eSPI& t,int x,int y,const char* s,uint16_t fg,uint16_t bg){
    t.setTextColor(fg,bg);t.setCursor(x,y);t.print(s);
}
static void fitLabel(TFT_eSPI& t,int x,int y,int w,const char* s,uint16_t fg,uint16_t bg){
    char out[48]; size_t n=strnlen(s,sizeof(out)-1); memcpy(out,s,n);out[n]=0;
    while(n && t.textWidth(out)>w)out[--n]=0;
    label(t,x,y,out,fg,bg);
}
void draw(TFT_eSPI& t,int w,int h){
    const unsigned ix=(unsigned)row.type;
    const Lines& L=lines[ix<(unsigned)DetectionType::COUNT?ix:0];
    const uint16_t A=L.a,B=L.b;
    const int x=8, room=w-16;
    t.setTextFont(1);t.setTextSize(1);t.setTextWrap(false);
    t.fillRect(0,0,w,h,Theme::BG);t.drawRect(3,3,w-6,h-6,A);
    for(int i=4;i<w-4;++i)t.drawFastVLine(i,4,29,Theme::blend(A,B,(i-4)*255/(w-9)));
    const char* grade=row.conf==Confidence::HIGH_CONF?"SURE":row.conf==Confidence::MED_CONF?"LIKELY":"MAYBE";
    const int pill=t.textWidth(grade)+12;
    t.fillRoundRect(w-pill-8,10,pill,16,7,TFT_BLACK);
    label(t,w-pill-2,14,grade,row.conf==Confidence::HIGH_CONF?Theme::GREEN:Theme::AMBER,TFT_BLACK);
    t.setTextSize(2);
    fitLabel(t,x,11,room-pill-6,title?title:"MORE INFO",TFT_BLACK,A);
    t.setTextSize(1);
    int y=40;
    fitLabel(t,x,y,room,L.head,Theme::CYAN,Theme::BG); y+=13;
    const char* ev=evidenceKindShortName(row.evidence);
    char why[48];snprintf(why,sizeof why,"WHY: %s",ev[0]?ev:"EVIDENCE UNAVAILABLE");
    fitLabel(t,x,y,room,why,A,Theme::BG);y+=15;
    buttonY=h-33;buttonW=(w-20)/3;
    const int tileY=buttonY-32;
    const unsigned tipN=Theme::wrapText(t,L.tip,room-14,wrapped,4);
    const int tipY=tileY-8-(int)tipN*11-13;
    t.fillRect(x,tipY,room,tileY-6-tipY,Theme::blend(Theme::BG,A,40));
    t.fillRect(x,tipY,3,tileY-6-tipY,A);
    label(t,x+6,tipY+3,"TIP",A,Theme::blend(Theme::BG,A,40));
    for(unsigned j=0;j<tipN;j++)label(t,x+6,tipY+14+j*11,wrapped[j],Theme::WHITE,Theme::blend(Theme::BG,A,40));
    // No fabricated place history or following verdict. Use only actual cache entries.
    char seen[12]="--",places[12]="--",first[24]="--";
    const char* movement="UNVERIFIED";
    if(!hidden){
        snprintf(seen,sizeof seen,"%uX",(unsigned)row.repeats);
        if(row.restored)Clock::formatEpochStamp(row.firstSeen,first,sizeof first);
        else Clock::formatStamp(row.firstSeen,first,sizeof first);
        first[5]=0;
#if SQW_TRAIL_ENABLED
        const auto* e=TrailHistory::cache().find(row.mac,(uint8_t)row.type);
        if(e && !row.restored){
            unsigned n=0; for(unsigned j=0;j<e->count;j++){bool duplicate=false;for(unsigned k=0;k<j;k++)if(e->stop[k]==e->stop[j])duplicate=true;if(!duplicate && Places::exists(e->stop[j]))++n;}
            snprintf(places,sizeof places,"%u",n);
            if(n>1)movement="MULTI STOP";
        }
#endif
    }
    const char* captions[]={"SEEN","PLACES","FIRST","TRAIL"};
    const char* values[]={seen,places,first,movement};
    const int cell=room/4;
    for(int j=0;j<4;j++){
        const int cx=x+j*cell;t.drawRect(cx,tileY,cell-1,27,A);
        fitLabel(t,cx+3,tileY+3,cell-6,captions[j],A,Theme::BG);
        fitLabel(t,cx+3,tileY+15,cell-6,values[j],Theme::WHITE,Theme::BG);
    }
    // Paginate the full description; no animation state advances during band redraw.
    const unsigned count=Theme::wrapText(t,body?body:"",room,wrapped,32);
    const unsigned per=tipY>y+10?(tipY-y-5)/11:1;
    pages=count?(count+per-1)/per:1;if(page>=pages)page=0;
    for(unsigned j=page*per;j<count && j<(page+1)*per;j++)label(t,x,y+(j-page*per)*11,wrapped[j],Theme::WHITE,Theme::BG);
    char more[20];snprintf(more,sizeof more,"MORE %u/%u",page+1,pages);
    Theme::drawButton(t,6,buttonY,buttonW,26,"BACK",false);
    Theme::drawButton(t,10+buttonW,buttonY,buttonW,26,more,false);
    Theme::drawButton(t,14+2*buttonW,buttonY,buttonW,26,"GOT IT",false);
}
bool tapCloses(int x,int y,int w,int h){
    (void)w;(void)h;
    if(y<buttonY || y>=buttonY+26)return false;
    if(x>=10+buttonW && x<10+2*buttonW){page=(page+1)%(pages?pages:1);return false;}
    if((x>=6 && x<6+buttonW)||(x>=14+2*buttonW && x<14+3*buttonW)){clear();return true;}
    return false;
}
}
#endif
