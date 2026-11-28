// Experimental SquachWatch Freenove FNK0104N (3.5-inch, ST77922).
// Original board integration; register values are hardware parameters based on
// vendor ST77922 panel initialization. This source is NOT compiled for CYD.
// v1: Wi-Fi v1.50 signatures, display, touch, RAM and battery diagnostics.
#if defined(FREENOVE35_N)
#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include "st77922_qspi.h"
#include "st77922_touch.h"
#include <esp_heap_caps.h>
#include "signatures.h"
#include "ui_boot.h"
#include "ui_clear.h"
#include "ui_settings.h"
#include "ui_log.h"
#include "ui_rawscan.h"
#include "theme.h"
#include "settings.h"
#include "detection.h"
#include "squachy.h"
#include "clock.h"
#include "blackbox.h"
#include "ui_desk.h"
#include "ui_detfilter.h"
#include "ui_alertrules.h"
#include "ui_beaconwarn.h"
#include "ui_outfit.h"
#include "pet.h"
#include "ui_outfit_unlock.h"
#include "ui_bingo.h"
#include "ui_dex.h"
#include "ui_diary.h"
#include "ui_colorcheck.h"
#include "ui_ignorelist.h"
#include "ignore_list.h"
#include "ui_diagnostics.h"
#include <esp_system.h>
#include "notices.h"
#include "bingo.h"
#include "dex.h"
#include "regulars.h"


// Board hardware is the exact SquachWatch upstream ST77922 implementation,
// not the earlier experimental SPI command/animation copy.
// 320x480 panel is native portrait. Reuse 2.8-CYD app's screen renderers.
namespace HW {constexpr int PW=320,PH=480; int W=PW,H=PH; uint8_t rotation=0;}
static void panelToUI(uint16_t px,uint16_t py,uint16_t& x,uint16_t& y){
  switch(HW::rotation){
    case 1:x=py;y=HW::PW-1-px;break;
    case 2:x=HW::PW-1-px;y=HW::PH-1-py;break;
    case 3:x=HW::PH-1-py;y=px;break;
    default:x=px;y=py;break;
  }
}
static TFT_eSPI dummy;
class FreenoveSprite : public TFT_eSprite {
public:
  explicit FreenoveSprite(TFT_eSPI* t):TFT_eSprite(t){}
  void reshape(int16_t w,int16_t h){
    _iwidth=_dwidth=_bitwidth=w;_iheight=_dheight=h;
    cursor_x=cursor_y=0;_sx=_sy=0;_sw=w;_sh=h;rotation=0;
    setViewport(0,0,w,h);setPivot(w/2,h/2);
  }
};
static FreenoveSprite view(&dummy);
static bool screenOk=false,touchOk=false;
static uint8_t touchVersion=0;
static uint16_t touchX=0,touchY=0;
static uint32_t lastDraw=0;
static uint16_t* dmaFrameChunk=nullptr;
static uint32_t frameCount=0,frameErrors=0,lastFrameReport=0;
static constexpr size_t CHUNK_PX=8192;
static bool startPanel(){
  pinMode(St77922::IO_BL,OUTPUT);
  digitalWrite(St77922::IO_BL,LOW);
  if(!St77922::begin())return false;
  dmaFrameChunk=(uint16_t*)heap_caps_malloc(CHUNK_PX*2,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
  if(!dmaFrameChunk){
    Serial.println("[freenove35] upstream QSPI: no internal DMA bounce buffer");
    return false;
  }
  digitalWrite(St77922::IO_BL,HIGH);
  Serial.println("[freenove35] upstream ST77922 QSPI controller ready");
  return true;
}
static bool startTouch(){
  // Touch reset may also reset the ST77922 display/TDDI state.
  St77922Touch::begin(38,39,48,47);
  St77922::reinit();
  return St77922Touch::probe();
}
static bool pollTouch(){uint16_t px=0,py=0;if(!touchOk||!St77922Touch::read(px,py))return false;panelToUI(px,py,touchX,touchY);return true;}
static void applyPanelColors(){
  static int previousInvert=-1, previousSwap=-1;
  const int inv=Settings::inverted()?1:0;
  const int swap=Settings::rgbSwapped()?1:0;
  if(inv!=previousInvert){St77922::invert(inv!=0);previousInvert=inv;}
  if(swap!=previousSwap){
    const uint8_t madctl=swap?0x08:0x00;
    St77922::writeReg(0x36,&madctl,1);
    previousSwap=swap;
  }
}
static bool present(){
  if(!screenOk||!view.getPointer()||!dmaFrameChunk)return false;
  applyPanelColors();
  const uint16_t* src=(const uint16_t*)view.getPointer();
  // ONE whole-screen window. Upstream opens 0x2C as a register command,
  // then streams QSPI pixel chunks with 0x3C (continuation). Do not
  // substitute the 0x2C quad command used by the broken LAB firmware.
  St77922::wait();
  St77922::setWindow(0,0,HW::PW-1,HW::PH-1);
  size_t offset=0,remaining=(size_t)HW::W*HW::H;
  bool first=true;
  while(remaining){
    size_t n=remaining<CHUNK_PX?remaining:CHUNK_PX;
    St77922::wait();  // bounce memory must be reusable before rewriting
    // TFT_eSprite 16-bit storage is already high-byte-first; do not swap twice.
    if(HW::rotation==0)memcpy(dmaFrameChunk,src+offset,n*sizeof(uint16_t));
    else for(size_t i=0;i<n;i++){
      const size_t p=offset+i;uint16_t x,y;
      panelToUI(p%HW::PW,p/HW::PW,x,y);
      dmaFrameChunk[i]=src[(size_t)y*HW::W+x];
    }
    if(!St77922::queuePixels(dmaFrameChunk,n,first)){
      ++frameErrors;
      Serial.printf("[freenove35] upstream QSPI queue FAIL count=%lu\n",(unsigned long)frameErrors);
      St77922::wait();
      return false;
    }
    first=false;offset+=n;remaining-=n;
  }
  St77922::wait();
  ++frameCount;
  uint32_t now=millis();
  if(now-lastFrameReport>=5000){
    Serial.printf("[freenove35] upstream LCD frames=%lu errors=%lu heap=%lu psram=%lu\n",
      (unsigned long)frameCount,(unsigned long)frameErrors,
      (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getFreePsram());
    lastFrameReport=now;
  }
  return true;
}

// The same art, fonts, mascot, animations, button geometry, and settings
// modules as main.cpp. Only the hardware framebuffer and touch transfer are
// board-specific. Avoid a second imitation dashboard.
static DetectionEngine engine;
enum class View : uint8_t { BOOT, CLEAR, SETTINGS, LOG, RAWSCAN, DESK, FILTER, RULES, BEACON, OUTFIT, BINGO, DEX, DIARY, COLORS, IGNORE, DIAG, UNLOCK };
static View activeView=View::BOOT;
static uint32_t bootAt=0, lastTouchAt=0, unlockAt=0;
static bool petGrip=false, mascotGrip=false, mascotHeld=false, mascotMoved=false, deskGrip=false, deskUnlocked=false;
static int gripX=0,gripY=0,gripDx=0;
static uint32_t gripAt=0;
static bool scanPicker=false, rawBleMode=true;
static bool touchHeld=false, radioReady=false;
static uint32_t lastUITick=0, lastRuntimeTick=0;
static void enterClear(){
  Pet::reset();
  Settings::deskActive(false);
  if(activeView==View::RAWSCAN)engine.stopRawScan();
  scanPicker=false;
  activeView=View::CLEAR;
  uiClearInit(view);
}
static bool maybeUnlock(){
  uint8_t idx=0;
  if(Squachy::consumeOutfitUnlock(idx)){activeView=View::UNLOCK;unlockAt=millis();uiOutfitUnlockInit(view,idx);return true;}
  if(Squachy::consumePetUnlockCard()){activeView=View::UNLOCK;unlockAt=millis();uiPetUnlockInit(view);return true;}
  return false;
}
static void consumeSceneRewards(){
  if(Theme::consumeWerewolfSummon())Squachy::unlockWolfPelt();
  if(Theme::consumeToasterCatch())Squachy::unlockChromeWing();
  if(Theme::consumeEyeCatch())Squachy::unlockVoidEye();
  if(Theme::consumeLodgeKnock())Squachy::unlockParka();
  if(Theme::consumeOwlReek())Squachy::unlockShambler();
  if(Theme::consumeRedGlyph())Squachy::unlockTh3();
  if(Theme::consumeSharkCatch())Squachy::unlockShark();
  if(uint8_t said=Theme::consumeXyzzy())Squachy::magicWord(said);
  if(Theme::consumePetUnlock())Squachy::unlockPet();
  Theme::setOwlAsks(!Squachy::shamblerUnlocked());
  Theme::setRedGlyph(!Squachy::th3Unlocked());
}
static void enterDesk(){Settings::deskActive(true);activeView=View::DESK;uiDeskInit(view);}
static void enterSettings(){
  activeView=View::SETTINGS;
  uiSettingsInit(view);
}
static void enterLog(DetectionType category=DetectionType::UNKNOWN){
  activeView=View::LOG;
  uiLogInit(view);
  // uiLogInit resets category to UNKNOWN; select the roster afterwards.
  uiLogSetCategory(category);
}
static void enterRawScan(bool ble){
  rawBleMode=ble;
  activeView=View::RAWSCAN;
  if(ble)engine.startRawBleScan();
  else engine.startRawWifiScan();
  uiRawScanInit(view,ble);
}
static bool setRotation(uint8_t next){
  const uint8_t old=HW::rotation;
  next&=3;
  if(next==old)return true;
  HW::rotation=next;
  HW::W=(next&1)?HW::PH:HW::PW;
  HW::H=(next&1)?HW::PW:HW::PH;
  view.reshape(HW::W,HW::H);
  Settings::saveRotation(next);
  Squachy::trigger(Squachy::Event::ROTATED);
  return true;
}
static void handleTouch(uint16_t x,uint16_t y){
  const uint32_t now=millis();
  if((uint32_t)(now-lastTouchAt)<180)return;
  lastTouchAt=now;
  if(activeView==View::UNLOCK){if(uiOutfitUnlockDismissable(now)&&!maybeUnlock())enterClear();return;}
  if(activeView==View::BOOT){enterClear();return;}
  if(!Settings::rotationLocked()&&Theme::rotateButtonHit(x,y,HW::W)){
    setRotation((HW::rotation+1)&3);return;
  }
  if(Theme::settingsButtonHit(x,y)){
    if(activeView!=View::SETTINGS) enterSettings();
    else if(uiSettingsCurrentPage()!=SettingsPage::MAIN)
      uiSettingsOpenPage(uiSettingsParentPage());
    else enterClear();
    return;
  }
  if(activeView==View::CLEAR){
    if(!scanPicker){
      const DetectionType category=uiClearCategoryHit(x,y,HW::W,HW::H);
      if(category!=DetectionType::UNKNOWN){enterLog(category);return;}
    }
    const ButtonId hit=Theme::hitTestButtonBar(x,y,HW::W,HW::H);
    if(scanPicker){
      if(hit==ButtonId::SCAN){scanPicker=false;enterRawScan(true);return;}
      if(hit==ButtonId::LOG){scanPicker=false;enterRawScan(false);return;}
      if(hit==ButtonId::CLR){scanPicker=false;return;}
    }
    if(hit==ButtonId::LOG){Squachy::trigger(Squachy::Event::LOG_OPENED);enterLog();return;}
    if(hit==ButtonId::SCAN){scanPicker=true;return;}
    if(hit==ButtonId::CLR){enterDesk();return;}
    if(Theme::backgroundTap(x,y,now)){consumeSceneRewards();return;}
    if(!Settings::backgroundLocked()&&(x<HW::W/10||x>=HW::W-HW::W/10)){
      if(x<HW::W/10)Settings::cyclePrevBackground();
      else Settings::cycleBackground();
      return;
    }
    if(Settings::boringMode()&&y>=20&&!Settings::backgroundLocked())Settings::cycleBackground();
  }else if(activeView==View::SETTINGS){
    if(uiSettingsConfirmRow()!=SettingsRow::NONE){auto hit=uiSettingsHitConfirm(x,y,HW::W,HW::H);if(hit==SettingsConfirmTap::CONFIRM){auto row=uiSettingsConfirmRow();uiSettingsSetConfirm(SettingsRow::NONE);if(row==SettingsRow::REPLAY_INTRO){Squachy::replayIntro();enterClear();}}else if(hit==SettingsConfirmTap::CANCEL)uiSettingsSetConfirm(SettingsRow::NONE);return;}
    if(uiSettingsTapPinnedOk(x,y,HW::W,HW::H)){enterClear();return;}
    if(uiSettingsTapPinnedBack(view,x,y,HW::W,HW::H)){
      if(uiSettingsCurrentPage()==SettingsPage::MAIN)enterClear();
      else uiSettingsOpenPage(uiSettingsParentPage());
      return;
    }
    const SettingsRow choice=uiSettingsHitTest(view,x,y,HW::W,HW::H);
    Serial.printf("[settings] page=%u row=%u x=%u y=%u\n",(unsigned)uiSettingsCurrentPage(),(unsigned)choice,x,y);
    if(uiSettingsRowIsOff(choice)){Theme::showToast("BORING MODE IS ON",nullptr,Theme::CYAN);return;}
    switch(choice){
      case SettingsRow::BRIGHTNESS:Settings::adjustBrightness(uiSettingsBrightnessDeltaForTap(view,x,HW::W,HW::H));ledcWrite(0,Settings::brightness());break;
      case SettingsRow::CONFIDENCE:Settings::cycleMinConfidence();break;
      case SettingsRow::AUTO_QUIET:Settings::cycleAutoQuiet();break;
      case SettingsRow::UPDATE_CHECK:Settings::toggleUpdateCheck();break;
      case SettingsRow::TIME_ZONE:Settings::stepTimeZone(uiSettingsTimeZoneStepForTap(view,x,HW::W,HW::H));Settings::markTimeZoneChosen();break;
      case SettingsRow::SQUACHY_SIZE:Settings::cycleSquachySize();break;
      case SettingsRow::SHADES_COLOR:Squachy::cycleShadesColor();break;
      case SettingsRow::BANTER:Settings::cycleBanter();break;
      case SettingsRow::AURA:Settings::toggleAura();break;
      case SettingsRow::PET:if(!Squachy::jailed())Squachy::cyclePet();break;
      case SettingsRow::REPLAY_INTRO:uiSettingsSetConfirm(choice);break;
      case SettingsRow::SHOW_OFF:Squachy::startShowOff();enterClear();break;
      case SettingsRow::DESK_OPEN:enterDesk();break;
      case SettingsRow::DESK_BACKGROUND:if(x<HW::W/2)Settings::cyclePrevDeskBackground();else Settings::cycleDeskBackground();break;
      case SettingsRow::CLOCK_FONT:Settings::cycleClockFont();break;
      case SettingsRow::CLOCK_SIZE:Settings::cycleClockSize();break;
      case SettingsRow::CLOCK_BACKDROP:Settings::cycleClockBackdrop();break;
      case SettingsRow::CLOCK_FORMAT:Settings::toggleClock24Hour();break;
      case SettingsRow::DESK_SQUAD:Settings::toggleDeskSquad();break;
      case SettingsRow::DESK_CROWD:Settings::cycleDeskCrowd();break;
      case SettingsRow::DESK_VISIT:Settings::toggleDeskFullVisit();break;
      case SettingsRow::FONT_MENUS:case SettingsRow::FONT_TEXT:case SettingsRow::FONT_BUTTONS:
      case SettingsRow::FONT_BUBBLES:case SettingsRow::FONT_PETS:case SettingsRow::FONT_XP:
        uiSettingsFontTap(view,x,y,HW::W,HW::H);break;
      case SettingsRow::DETECTION_FILTER:activeView=View::FILTER;uiDetFilterInit(view);break;
      case SettingsRow::ALERT_RULES:activeView=View::RULES;uiAlertRulesInit(view);break;
      case SettingsRow::OUTFIT:if(!Squachy::jailed()){activeView=View::OUTFIT;uiOutfitInit(view);}break;
      case SettingsRow::BINGO:activeView=View::BINGO;uiBingoInit(view);break;
      case SettingsRow::DEX:activeView=View::DEX;uiDexInit(view);break;
      case SettingsRow::VIEW_DIARY:activeView=View::DIARY;uiDiaryInit(view);break;
      case SettingsRow::CHECK_COLORS:activeView=View::COLORS;uiColorCheckInit(view);break;
      case SettingsRow::DIAGNOSTICS:activeView=View::DIAG;uiDiagnosticsInit(view);break;
      case SettingsRow::IGNORED_DEVICES:activeView=View::IGNORE;uiIgnoreListInit(view);break;
      case SettingsRow::SNOOZE_MANAGER:uiSettingsOpenPage(SettingsPage::SNOOZES);break;
      case SettingsRow::SNOOZE_ENTRY:{
        int i=uiSettingsSnoozeIndexAt(view,x,y,HW::W,HW::H);uint8_t mac[6];uint32_t ms=0;bool always=false;
        if(i>=0&&IgnoreList::snoozeInfo(i,mac,ms,always))IgnoreList::unsnooze(mac);break;}
      case SettingsRow::WATCH_TARGET:engine.clearWatches();break;
      case SettingsRow::HUNT_TARGET:engine.clearHunts();break;
      case SettingsRow::APPEARANCE:uiSettingsOpenPage(SettingsPage::APPEARANCE);break;
      case SettingsRow::SYSTEM:uiSettingsOpenPage(SettingsPage::SYSTEM);break;
      case SettingsRow::DESK_MODE:uiSettingsOpenPage(SettingsPage::DESK);break;
      case SettingsRow::BACK:
        if(uiSettingsCurrentPage()==SettingsPage::MAIN)enterClear();
        else uiSettingsOpenPage(uiSettingsParentPage());
        break;
      case SettingsRow::THEME:Settings::cyclePalette();break;
      case SettingsRow::BACKGROUND:Settings::cycleBackground();break;
      case SettingsRow::BACKGROUND_LOCK:Settings::toggleBackgroundLocked();break;
      case SettingsRow::INVERT:Settings::toggleInvert();break;
      case SettingsRow::RGB_SWAP:Settings::toggleRgbSwap();break;
      case SettingsRow::BORING_MODE:Settings::toggleBoringMode();break;
      case SettingsRow::ROTATION_LOCK:Settings::toggleRotationLock();break;
      case SettingsRow::SHOW_WIFI_ICON:Settings::toggleWifiIcon();break;
      case SettingsRow::SHOW_SD_ICON:Settings::toggleSdIcon();break;
      case SettingsRow::PRIVACY:Settings::togglePrivacyMode();break;
      case SettingsRow::FONTS:uiSettingsOpenPage(SettingsPage::FONTS);break;
      default:
        if(choice!=SettingsRow::NONE)Theme::showToast("FREENOVE LAB","PORT IN PROGRESS",Theme::AMBER);
        break;
    }
  }else if(activeView==View::FILTER){
    if(Theme::pinnedBackHit(x,y,HW::W,HW::H)){enterSettings();return;}
    const auto hit=uiDetFilterHitTest(view,x,y,HW::W,HW::H);
    if(hit==DetectionType::IBEACON&&!Settings::typeEnabled(hit)){activeView=View::BEACON;uiBeaconWarnInit(view);}
    else if(hit!=DetectionType::COUNT)Settings::toggleType(hit);
  }else if(activeView==View::BEACON){
    const auto hit=uiBeaconWarnHitTest(view,x,y);
    if(hit!=BeaconWarnHit::NONE){if(hit==BeaconWarnHit::ENABLE&&!Settings::typeEnabled(DetectionType::IBEACON))Settings::toggleType(DetectionType::IBEACON);activeView=View::FILTER;uiDetFilterInit(view,true);}
  }else if(activeView==View::RULES){
    const auto hit=uiAlertRulesHitTest(view,x,y,HW::W,HW::H);const auto ty=uiAlertRulesSelected();
    switch(hit){
      case AlertRuleHit::BACK:if(uiAlertRulesInDetail())uiAlertRulesBackToList();else enterSettings();break;
      case AlertRuleHit::ENABLED:Settings::toggleAlertEnabled(ty);break;
      case AlertRuleHit::CONFIDENCE:Settings::cycleAlertMinConfidence(ty);break;
      case AlertRuleHit::REPEATS:Settings::cycleAlertMinRepeats(ty);break;
      case AlertRuleHit::COOLDOWN:Settings::cycleAlertCooldown(ty);break;
      case AlertRuleHit::WAKE:Settings::toggleAlertWakeScreen(ty);break;
      default:break;}
  }else if(activeView==View::IGNORE){
    if(Theme::pinnedBackHit(x,y,HW::W,HW::H)){enterSettings();return;}
    uint8_t i=uiIgnoreListHitPolicy(view,x,y,HW::W,HW::H);const uint8_t* m=IgnoreList::macAt(i);
    if(i!=0xff&&m){uint8_t mac[6];memcpy(mac,m,6);auto p=IgnoreList::policyAt(i);
      IgnoreList::setPolicy(mac,IgnoreList::typeAt(i),(IgnoreList::Policy)(((unsigned)p+1)%4));}
  }else if(activeView==View::OUTFIT){if(!uiOutfitTapArrow(x,y,HW::W,HW::H))enterClear();
  }else if(activeView==View::BINGO){if(uiBingoHitTest(view,x,y,HW::W,HW::H)==BingoTap::BACK)enterSettings();
  }else if(activeView==View::DEX){if(uiDexHitTest(view,x,y,HW::W,HW::H)==DexTap::BACK)enterSettings();
  }else if(activeView==View::DIARY){enterClear();
  }else if(activeView==View::COLORS){
    switch(uiColorCheckHitTest(x,y,HW::W,HW::H)){
      case ColorCheckTap::INVERT:Settings::toggleInvert();break;
      case ColorCheckTap::ORDER:Settings::toggleRgbSwap();break;
      case ColorCheckTap::DONE:Settings::markColorChecked();enterSettings();break;
      default:break;}
  }else if(activeView==View::DIAG){
    if(uiDiagnosticsHitBack(x,y,HW::W,HW::H))enterSettings();else {int p=uiDiagnosticsHitPage(x,y,HW::W,HW::H);if(p)uiDiagnosticsPage(p);}
  }else if(activeView==View::DESK){
    if(uiDeskHitBack(x,y,HW::W,HW::H)){enterClear();return;}
    if(uiDeskHitSettings(x,y,HW::W,HW::H)){enterSettings();uiSettingsOpenPage(SettingsPage::DESK);return;}
    if(uiDeskHitTimer(x,y,HW::W,HW::H)){uiDeskTapTimer(now);return;}
  }else if(activeView==View::RAWSCAN){
    const RawScanTap hit=uiRawScanHitTest(x,y,HW::W,HW::H);
    if(hit==RawScanTap::BACK){enterClear();return;}
    if(hit==RawScanTap::SWITCH){engine.stopRawScan();enterRawScan(!rawBleMode);return;}
    uiRawScanScroll(y>HW::H/2?1:-1);
  }else if(activeView==View::LOG){
    if(uiLogCategory()!=DetectionType::UNKNOWN){
      if(uiLogCategoryHitBack(x,y,HW::W,HW::H)){enterClear();return;}
      uiLogScroll(y>HW::H/2?1:-1);
      return;
    }
    if(y>HW::H-65){enterClear();return;}
    const LogViewTap v=uiLogHitView(x,y,HW::W,HW::H);
    if(v==LogViewTap::DEVICES)uiLogSetView(LogView::DEVICES);
    else if(v==LogViewTap::EVENTS)uiLogSetView(LogView::EVENTS);
    else uiLogScroll(y>HW::H/2?1:-1);
  }
}
static void renderOriginal(uint32_t now){
  // Source-accurate UI. All drawing goes to the PSRAM-backed TFT_eSprite;
  // the ST77922 QSPI push is the only board-specific rendering layer.
  switch(activeView){
    case View::UNLOCK:uiOutfitUnlockTick(view,now,engine);break;
    case View::BOOT:uiBootTick(view,now,HW::W,HW::H,true);break;
    case View::CLEAR:uiClearTick(view,now,engine,true,scanPicker,HW::W,HW::H);break;
    case View::FILTER:uiDetFilterTick(view,now,engine);break;
    case View::RULES:uiAlertRulesTick(view,now);break;
    case View::BEACON:uiBeaconWarnTick(view,now,engine,true);break;
    case View::OUTFIT:uiOutfitTick(view,now,engine,true);break;
    case View::BINGO:uiBingoTick(view,now,engine,true);break;
    case View::DEX:uiDexTick(view,now,engine,true);break;
    case View::DIARY:uiDiaryTick(view,now,engine);break;
    case View::COLORS:uiColorCheckTick(view,now);break;
    case View::IGNORE:uiIgnoreListTick(view,now);break;
    case View::DIAG:{
      DiagnosticsInfo d={};d.hasRaw=touchOk;d.rawTouching=touchHeld;d.touchValid=touchHeld;d.mappedX=touchX;d.mappedY=touchY;d.calSource="ST77922";d.freeHeap=ESP.getFreeHeap();d.largestBlock=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);d.boardName="FNK0104N";d.resetReason="See serial";d.usingCapTouch=true;d.bbReady=BlackBox::ready();d.bbKept=BlackBox::detectionsKept();uiDiagnosticsTick(view,now,engine,d);break;}
    case View::DESK:uiDeskTick(view,now,engine,true);break;
    case View::SETTINGS:uiSettingsTick(view,now,engine,false,true);break;
    case View::LOG:
      uiLogTick(view,now,engine,0,false,nullptr,false,nullptr,nullptr,
                false,false,false,HW::W,HW::H,true);
      break;
    case View::RAWSCAN:
      uiRawScanTick(view,now,engine,rawBleMode,
        rawBleMode?engine.rawBleScanDone():engine.rawWifiScanDone(),
        false,nullptr,false,false,false,true);
      break;
  }
  Theme::drawToast(view,now);
  if(!present())Serial.println("[freenove35] ST77922 transfer failed");
  lastDraw=now;
}
void setup(){
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  delay(200);
  Serial.println("\n[SQUACHWATCH] FNK0104N native SquachWatch UI + detection R2");
  Serial.printf("[boot] PSRAM %s (%lu MB)\n",psramFound()?"yes":"NO",
    (unsigned long)(ESP.getPsramSize()/1048576));
  screenOk=startPanel();
  touchOk=startTouch();
  Serial.printf("[hw] ST77922=%d touch 0x55=%d fw=%u\n",
                (int)screenOk,(int)touchOk,(unsigned)touchVersion);
  view.setColorDepth(16);
  view.setAttribute(PSRAM_ENABLE,true);
  if(!view.createSprite(HW::W,HW::H)){
    Serial.println("[fatal] No PSRAM-backed native 320x480 RGB565 frame");
    screenOk=false;
  }
  if(!screenOk)return;
  randomSeed(esp_random());
  Settings::load();
  ledcSetup(0,5000,8);ledcAttachPin(St77922::IO_BL,0);ledcWrite(0,Settings::brightness());
  IgnoreList::begin();
  setRotation(Settings::rotation());
  Clock::begin();
  if(BlackBox::begin()){
    BlackBox::BootRecord br={};
    br.reason=(uint8_t)esp_reset_reason();
    br.epoch=Clock::trusted()?Clock::nowEpoch():0;
    BlackBox::noteBoot(br);
    Serial.println("[events] persistent flash ring ready");
  }else Serial.println("[events] persistent flash ring FAILED");
  // No CYD SD mount; engine SD initialization is guarded for this board.
  // Onboard ESP32-S3 WiFi/BLE are started by the real detection engine.
  radioReady=engine.init();
  Serial.printf("[radio] detector=%s\n",radioReady?"OK":"FAILED");
  Bingo::begin(engine);
  Dex::begin();
  Regulars::begin();
  Squachy::setIdleProvider([](){return Notices::idleLine(engine);});
  Squachy::trigger(Squachy::Event::BOOTED,DetectionType::UNKNOWN,engine.lifetimeTotal());
  uiBootInit(view);
  bootAt=millis();
  activeView=View::BOOT;
  renderOriginal(bootAt);
}
void loop(){
  if(!screenOk){delay(500);return;}
  const uint32_t now=millis();
  if(radioReady)engine.loop();
  if(now-lastRuntimeTick>=100){
    lastRuntimeTick=now;
    Clock::tick(now);
    Bingo::tick(now);Dex::tick(now);Regulars::tick(now);
  }
  // GT911-style level handling is NOT used; ST77922 touch reports a
  // valid point while pressed. Only process the first press in a gesture.
  bool down=pollTouch();
  static bool settingsGesture=false,moved=false;
  static uint16_t startX=0,startY=0;
  static int lastY=0;
  static uint32_t downAt=0;
  if(down&&!touchHeld){
    settingsGesture=activeView==View::SETTINGS||activeView==View::FILTER||activeView==View::RULES||activeView==View::IGNORE||activeView==View::DIAG;
    if(settingsGesture){startX=touchX;startY=touchY;lastY=touchY;downAt=now;moved=false;}
    else if(activeView==View::CLEAR){
      gripX=touchX;gripY=touchY;gripDx=0;gripAt=now;mascotHeld=mascotMoved=deskUnlocked=false;
      const bool boring=Settings::boringMode();
      if(Squachy::showOffActive()){Squachy::stopShowOff();}
      else if(!boring&&Squachy::onboardingActive()&&Squachy::onboardingTapAdvance(touchX,touchY)){}
      else if(!boring&&!Squachy::onboardingActive()&&(petGrip=Pet::grab(touchX,touchY,now))){}
      else if(!scanPicker&&Theme::hitTestButtonBar(touchX,touchY,HW::W,HW::H)==ButtonId::CLR){deskGrip=true;}
      else if(Theme::backgroundTap(touchX,touchY,now)){consumeSceneRewards();}
      else if(!boring&&!scanPicker&&!Theme::settingsButtonHit(touchX,touchY)&&!Theme::rotateButtonHit(touchX,touchY,HW::W)&&uiClearCategoryHit(touchX,touchY,HW::W,HW::H)==DetectionType::UNKNOWN&&Squachy::hitTest(touchX,touchY)){mascotGrip=true;}
      else handleTouch(touchX,touchY);
    }else handleTouch(touchX,touchY);
  }
  if(down&&settingsGesture){
    const int dy=(int)touchY-lastY;
    if(abs((int)touchX-(int)startX)>16)moved=true;
    if(abs(dy)>10){moved=true;int step=dy>0?-1:1;switch(activeView){case View::SETTINGS:uiSettingsScroll(step);break;case View::FILTER:uiDetFilterScroll(step);break;case View::RULES:if(!uiAlertRulesInDetail())uiAlertRulesScroll(step);break;case View::IGNORE:uiIgnoreListScroll(step);break;case View::DIAG:uiDiagnosticsScroll(step);break;default:break;}lastY=touchY;}
  }
  if(!down&&touchHeld&&settingsGesture){
    settingsGesture=false;
    if(!moved)handleTouch(startX,startY);
  }
  if(activeView==View::CLEAR){
    if(down&&petGrip)Pet::drag(touchX,touchY,now);
    if(down&&mascotGrip){
      int dx=(int)touchX-gripX,dy=(int)touchY-gripY;gripDx=dx;
      if(!mascotHeld&&dx*dx+dy*dy>100)mascotMoved=true;
      if(mascotHeld)Squachy::grabTo(touchX,touchY);
      else if(mascotMoved)Squachy::trigger(Squachy::Event::PETTING);
      else if(now-gripAt>=600){mascotHeld=true;Squachy::trigger(Squachy::Event::HELD);}
    }
    if(down&&deskGrip&&!deskUnlocked&&now-gripAt>=4000){deskUnlocked=true;Squachy::unlockAllOutfits();}
    if(!down&&touchHeld){
      if(petGrip){Pet::release(now);petGrip=false;}
      if(mascotGrip){if(mascotHeld)Squachy::release();else if(!mascotMoved){Squachy::noteTapAt(gripX,gripY);Squachy::trigger(Squachy::Event::PETTED);}else if(now-gripAt<320&&abs(gripDx)>48)Squachy::flick(gripDx>0?1:-1);mascotGrip=false;}
      if(deskGrip){deskGrip=false;if(!deskUnlocked)enterDesk();}
    }
    consumeSceneRewards();
    if(activeView==View::CLEAR&&!down&&!Squachy::onboardingActive())maybeUnlock();
  }else{
    if(petGrip){Pet::cancel(now);petGrip=false;}
    if(mascotGrip&&mascotHeld)Squachy::release();
    mascotGrip=deskGrip=false;
  }
  if(activeView==View::UNLOCK&&now-unlockAt>12000&&!maybeUnlock())enterClear();
  touchHeld=down;
  if(activeView==View::BOOT&&now-bootAt>=3100){if(!Settings::colorChecked()){activeView=View::COLORS;uiColorCheckInit(view);}else if(Settings::deskWanted())enterDesk();else enterClear();}
  // ST77922 QSPI: a complete 307KB frame costs much more than classic CYD.
  // Cap physical push rate; all UI timing uses actual millis().
  if(now-lastUITick>=120){
    lastUITick=now;
    renderOriginal(now);
  }
  delay(4);
}
#endif // FREENOVE35_N
