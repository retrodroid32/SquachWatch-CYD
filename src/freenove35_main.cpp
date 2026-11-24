// Experimental SquachWatch Freenove FNK0104N (3.5-inch, ST77922).
// Original board integration; register values are hardware parameters based on
// vendor ST77922 panel initialization. This source is NOT compiled for CYD.
// v1: Wi-Fi v1.50 signatures, display, touch, RAM and battery diagnostics.
#if defined(FREENOVE35_N)
#include <Arduino.h>
#include <Wire.h>
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
#include "notices.h"
#include "bingo.h"
#include "dex.h"
#include "regulars.h"


// Board hardware is the exact SquachWatch upstream ST77922 implementation,
// not the earlier experimental SPI command/animation copy.
// 320x480 panel is native portrait. Reuse 2.8-CYD app's screen renderers.
namespace HW {constexpr int W=320,H=480;}
static TFT_eSPI dummy;
static TFT_eSprite view(&dummy);
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
static bool pollTouch(){return touchOk&&St77922Touch::read(touchX,touchY);}
static bool present(){
  if(!screenOk||!view.getPointer()||!dmaFrameChunk)return false;
  const uint16_t* src=(const uint16_t*)view.getPointer();
  // ONE whole-screen window. Upstream opens 0x2C as a register command,
  // then streams QSPI pixel chunks with 0x3C (continuation). Do not
  // substitute the 0x2C quad command used by the broken LAB firmware.
  St77922::wait();
  St77922::setWindow(0,0,HW::W-1,HW::H-1);
  size_t offset=0,remaining=(size_t)HW::W*HW::H;
  bool first=true;
  while(remaining){
    size_t n=remaining<CHUNK_PX?remaining:CHUNK_PX;
    St77922::wait();  // bounce memory must be reusable before rewriting
    for(size_t i=0;i<n;i++)dmaFrameChunk[i]=St77922::wire(src[offset+i]);
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
enum class View : uint8_t { BOOT, CLEAR, SETTINGS, LOG, RAWSCAN };
static View activeView=View::BOOT;
static uint32_t bootAt=0, lastTouchAt=0;
static bool scanPicker=false, rawBleMode=true;
static bool touchHeld=false, radioReady=false;
static uint32_t lastUITick=0, lastRuntimeTick=0;
static void enterClear(){
  if(activeView==View::RAWSCAN)engine.stopRawScan();
  scanPicker=false;
  activeView=View::CLEAR;
  uiClearInit(view);
}
static void enterSettings(){
  activeView=View::SETTINGS;
  uiSettingsInit(view);
}
static void enterLog(){
  activeView=View::LOG;
  uiLogSetCategory(DetectionType::UNKNOWN);
  uiLogInit(view);
}
static void enterRawScan(bool ble){
  rawBleMode=ble;
  activeView=View::RAWSCAN;
  if(ble)engine.startRawBleScan();
  else engine.startRawWifiScan();
  uiRawScanInit(view,ble);
}
static void handleTouch(uint16_t x,uint16_t y){
  const uint32_t now=millis();
  if((uint32_t)(now-lastTouchAt)<180)return;
  lastTouchAt=now;
  if(activeView==View::BOOT){enterClear();return;}
  if(Theme::settingsButtonHit(x,y)){
    if(activeView!=View::SETTINGS) enterSettings();
    else if(uiSettingsCurrentPage()!=SettingsPage::MAIN)
      uiSettingsOpenPage(uiSettingsParentPage());
    else enterClear();
    return;
  }
  if(activeView==View::CLEAR){
    const ButtonId hit=Theme::hitTestButtonBar(x,y,HW::W,HW::H);
    if(hit==ButtonId::LOG){Squachy::trigger(Squachy::Event::LOG_OPENED);enterLog();return;}
    if(scanPicker){
      if(hit==ButtonId::SCAN){scanPicker=false;enterRawScan(true);return;}
      if(hit==ButtonId::LOG){scanPicker=false;enterRawScan(false);return;}
      if(hit==ButtonId::CLR){scanPicker=false;return;}
    }
    if(hit==ButtonId::SCAN){scanPicker=true;return;}
    if(hit==ButtonId::CLR){enterLog();return;}
    if(!Settings::backgroundLocked()&&(x<HW::W/10||x>=HW::W-HW::W/10)){
      if(x<HW::W/10)Settings::cyclePrevBackground();
      else Settings::cycleBackground();
      return;
    }
    if(Squachy::hitTest(x,y)){Squachy::trigger(Squachy::Event::PETTED);return;}
  }else if(activeView==View::SETTINGS){
    if(uiSettingsTapPinnedBack(view,x,y,HW::W,HW::H)){
      if(uiSettingsCurrentPage()==SettingsPage::MAIN)enterClear();
      else uiSettingsOpenPage(uiSettingsParentPage());
      return;
    }
    const SettingsRow choice=uiSettingsHitTest(view,x,y,HW::W,HW::H);
    switch(choice){
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
  }else if(activeView==View::RAWSCAN){
    const RawScanTap hit=uiRawScanHitTest(x,y,HW::W,HW::H);
    if(hit==RawScanTap::BACK){enterClear();return;}
    if(hit==RawScanTap::SWITCH){engine.stopRawScan();enterRawScan(!rawBleMode);return;}
    uiRawScanScroll(y>HW::H/2?1:-1);
  }else if(activeView==View::LOG){
    if(uiLogCategoryHitBack(x,y,HW::W,HW::H)){enterClear();return;}
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
    case View::BOOT:uiBootTick(view,now,HW::W,HW::H,true);break;
    case View::CLEAR:uiClearTick(view,now,engine,true,scanPicker,HW::W,HW::H);break;
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
  if(!present())Serial.println("[freenove35] ST77922 transfer failed");
  lastDraw=now;
}
void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[SQUACHWATCH] FNK0104N native SquachWatch UI + detection R2");
  Serial.printf("[boot] PSRAM %s (%lu MB)\n",psramFound()?"yes":"NO",
    (unsigned long)(ESP.getPsramSize()/1048576));
  screenOk=startPanel();
  touchOk=startTouch();
  Serial.printf("[hw] ST77922=%d touch 0x55=%d fw=%u\n",
                (int)screenOk,(int)touchOk,(unsigned)touchVersion);
  view.setColorDepth(16);
  if(!view.createSprite(HW::W,HW::H)){
    Serial.println("[fatal] No PSRAM-backed native 320x480 RGB565 frame");
    screenOk=false;
  }
  if(!screenOk)return;
  randomSeed(esp_random());
  Settings::load();
  Clock::begin();
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
  if(down&&!touchHeld)handleTouch(touchX,touchY);
  touchHeld=down;
  if(activeView==View::BOOT&&now-bootAt>=3100)enterClear();
  // ST77922 QSPI: a complete 307KB frame costs much more than classic CYD.
  // Cap physical push rate; all UI timing uses actual millis().
  if(now-lastUITick>=120){
    lastUITick=now;
    renderOriginal(now);
  }
  delay(4);
}
#endif // FREENOVE35_N
