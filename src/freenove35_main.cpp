// Experimental SquachWatch Freenove FNK0104N (3.5-inch, ST77922).
// Original board integration; register values are hardware parameters based on
// vendor ST77922 panel initialization. This source is NOT compiled for CYD.
// v1: Wi-Fi v1.50 signatures, display, touch, RAM and battery diagnostics.
#if defined(FREENOVE35_N)
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <driver/spi_master.h>
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


namespace HW {
constexpr int W=320, H=480;
constexpr int CS=10, CLK=12, IO0=11, IO1=13, IO2=14, IO3=9, BL=41;
constexpr int SDA=38, SCL=39, TP_RST=48, TP_INT=47, TP_ADDR=0x55;
constexpr int BAT_ADC=8;
constexpr size_t DMA_BYTES=16000;
}
struct PanelCommand {uint8_t op; uint8_t data[16]; uint8_t len; uint16_t wait;};
static const PanelCommand panelInit[] = {
  {0xF1, {0x00}, 1, 0},
  {0x60, {0x00, 0x00, 0x00}, 3, 0},
  {0x65, {0x80}, 1, 0},
  {0x79, {0x06}, 1, 0},
  {0x7B, {0x00, 0x08, 0x08}, 3, 0},
  {0x80, {0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA}, 11, 0},
  {0x81, {0x26, 0x52, 0x72, 0x27}, 4, 0},
  {0x84, {0x92, 0x25}, 2, 0},
  {0x87, {0x10, 0x10, 0x58, 0x00, 0x02, 0x3A}, 6, 0},
  {0x88, {0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06}, 15, 0},
  {0x89, {0x00, 0x00, 0x00}, 3, 0},
  {0x8A, {0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19}, 11, 0},
  {0x8B, {0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E}, 9, 0},
  {0x8C, {0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01, 0xC5, 0x12, 0x09}, 13, 0},
  {0x8D, {0x0C}, 1, 0},
  {0x8E, {0x33, 0x01, 0x0C, 0x13, 0x01, 0x01}, 6, 0},
  {0xB3, {0x00, 0x30}, 2, 0},
  {0xF1, {0x00}, 1, 0},
  {0x71, {0xD0}, 1, 0},
  {0x66, {0x02, 0x3F}, 2, 0},
  {0xBE, {0x26, 0x00, 0x9D}, 3, 0},
  {0x70, {0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00, 0x00, 0x1A}, 12, 0},
  {0x90, {0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27}, 9, 0},
  {0x91, {0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27}, 9, 0},
  {0x92, {0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27}, 10, 0},
  {0x93, {0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27}, 10, 0},
  {0x94, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 6, 0},
  {0x95, {0x96, 0x16, 0x00, 0x00, 0xFF}, 5, 0},
  {0x96, {0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27, 0x00, 0x44}, 12, 0},
  {0x97, {0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27, 0x00, 0x44}, 12, 0},
  {0xBA, {0x55, 0x94, 0x2D, 0x94, 0x27}, 5, 0},
  {0x9A, {0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}, 7, 0},
  {0x9B, {0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}, 7, 0},
  {0x9C, {0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00}, 13, 0},
  {0x9D, {0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}, 8, 0},
  {0x9E, {0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}, 7, 0},
  {0xB4, {0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00, 0x1E, 0x10}, 12, 0},
  {0xB5, {0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01, 0x1E, 0x12}, 12, 0},
  {0xB6, {0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF}, 7, 0},
  {0x86, {0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C, 0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5}, 14, 0},
  {0xB7, {0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03, 0x03, 0x42, 0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D}, 16, 0},
  {0xB8, {0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02, 0x03, 0x42, 0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D}, 16, 0},
  {0xB9, {0x23, 0x23}, 2, 0},
  {0xBF, {0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B}, 6, 0},
  {0xF2, {0x00}, 1, 0},
  {0x73, {0x04, 0xDA, 0x12, 0x54, 0x47}, 5, 0},
  {0x77, {0x6B, 0x5B, 0xFD, 0xC3, 0xC5}, 5, 0},
  {0x7A, {0x15, 0x27}, 2, 0},
  {0x7B, {0x04, 0x57}, 2, 0},
  {0x7E, {0x01, 0x0E}, 2, 0},
  {0xBF, {0x36}, 1, 0},
  {0xE3, {0x40, 0x40}, 2, 0},
  {0xF0, {0x00}, 1, 0},
  {0xD0, {0x00}, 1, 0},
  {0x2A, {0x00, 0x00, 0x01, 0x3F}, 4, 0},
  {0x2B, {0x00, 0x00, 0x01, 0xDF}, 4, 0},
  {0x21, {}, 0, 0},     // INVON (panel needs inverted data)
  {0x11, {}, 0, 120},   // SLPOUT
  {0x29, {}, 0, 0},     // DISPON
  {0x2C, {}, 0, 0},
  {0x3A, {0x01}, 1, 0},     // COLMOD as the vendor sets it (renders RGB565)
  {0x36, {0x00}, 1, 0},
  {0x35, {0x01}, 1, 20},    // TEON
};
static spi_device_handle_t spiPanel=nullptr;
static uint8_t *dmaBuffer=nullptr;
static uint8_t args[24];
static bool screenOk=false,touchOk=false,scanning=false;
static TFT_eSPI dummy;
static TFT_eSprite view(&dummy);
static uint8_t page=0;
static int networks=0,matches=0;
static uint32_t lastScan=0,lastDraw=0;
static bool redraw=true;
static uint16_t touchX=0,touchY=0;
static uint32_t touches=0;
static uint8_t touchVersion=0;
static uint16_t lastBattery=0;
static char lastSeen[64]="No match recorded";

static bool panelCommand(uint8_t cmd,const uint8_t *data=nullptr,size_t size=0){
  if(!spiPanel||size>sizeof(args)) return false;
  spi_transaction_ext_t tx={};
  tx.base.flags=SPI_TRANS_VARIABLE_CMD|SPI_TRANS_VARIABLE_ADDR;
  tx.command_bits=8; tx.address_bits=24;
  tx.base.cmd=0x02; tx.base.addr=(uint32_t)cmd<<8;
  tx.base.length=size*8;
  if(size){memcpy(args,data,size);tx.base.tx_buffer=args;}
  return spi_device_polling_transmit(spiPanel,reinterpret_cast<spi_transaction_t*>(&tx))==ESP_OK;
}
static bool panelPixels(const uint8_t *data,size_t bytes){
  spi_transaction_ext_t tx={};
  tx.base.flags=SPI_TRANS_MODE_QIO|SPI_TRANS_VARIABLE_CMD|SPI_TRANS_VARIABLE_ADDR;
  tx.command_bits=8;tx.address_bits=24;
  // Every band sets a NEW 2A/2B address window. It must start with
  // RAMWR (0x2C), not RAMWRC (0x3C), which is legal only for continuing
  // an already active window. 0x3C caused the previous frozen/noisy LCD.
  tx.base.cmd=0x32;tx.base.addr=0x2C00;
  tx.base.length=bytes*8;tx.base.tx_buffer=data;
  return spi_device_polling_transmit(spiPanel,reinterpret_cast<spi_transaction_t*>(&tx))==ESP_OK;
}
static bool window(int x0,int y0,int x1,int y1){
  uint8_t col[4]={uint8_t(x0>>8),uint8_t(x0),uint8_t(x1>>8),uint8_t(x1)};
  uint8_t row[4]={uint8_t(y0>>8),uint8_t(y0),uint8_t(y1>>8),uint8_t(y1)};
  return panelCommand(0x2A,col,4)&&panelCommand(0x2B,row,4);
}
static bool startPanel(){
  // Freenove 3.5 ST77922 QSPI: D0..3 11/13/14/9, SCK12, CS10.
  pinMode(HW::BL,OUTPUT);digitalWrite(HW::BL,LOW);
  dmaBuffer=(uint8_t*)heap_caps_malloc(HW::DMA_BYTES,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
  if(!dmaBuffer)return false;
  spi_bus_config_t bus={};
  bus.data0_io_num=HW::IO0;bus.data1_io_num=HW::IO1;
  bus.data2_io_num=HW::IO2;bus.data3_io_num=HW::IO3;
  bus.sclk_io_num=HW::CLK;bus.max_transfer_sz=HW::DMA_BYTES;
  bus.flags=SPICOMMON_BUSFLAG_MASTER|SPICOMMON_BUSFLAG_QUAD;
  if(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO)!=ESP_OK)return false;
  spi_device_interface_config_t device={};
  device.mode=0;device.clock_speed_hz=40000000;
  device.spics_io_num=HW::CS;device.queue_size=1;device.flags=SPI_DEVICE_HALFDUPLEX;
  if(spi_bus_add_device(SPI2_HOST,&device,&spiPanel)!=ESP_OK)return false;
  for(const auto& c:panelInit){
    if(!panelCommand(c.op,c.data,c.len))return false;
    if(c.wait)delay(c.wait);
  }
  // The Freenove 3.5 has a native 320x480 portrait pixel map; this
  // diagnostic R1 draws in native orientation (no axis-swap support).
  digitalWrite(HW::BL,HIGH);
  Serial.printf("[freenove35] ST77922 QSPI clock=%uHz write=RAMWR 0x2C width=%d height=%d\n",
     40000000,HW::W,HW::H);
  return true;
}
static uint32_t frameCount=0, frameErrors=0;
static uint32_t lastFrameReport=0;
static bool present(){
  if(!screenOk||!view.getPointer())return false;
  const uint16_t *pixels=(const uint16_t*)view.getPointer();
  const int rowsPer=HW::DMA_BYTES/(HW::W*2);
  for(int y=0;y<HW::H;y+=rowsPer){
    int count=min(rowsPer,HW::H-y);
    // ESP32 is little-endian, panel expects big-endian RGB565.
    for(int i=0;i<count*HW::W;++i){
      uint16_t c=pixels[(size_t)y*HW::W+i];
      dmaBuffer[i*2]=uint8_t(c>>8);
      dmaBuffer[i*2+1]=uint8_t(c);
    }
    if(!window(0,y,HW::W-1,y+count-1)||
       !panelPixels(dmaBuffer,count*HW::W*2)){
      frameErrors++;
      if(frameErrors<=5 || (frameErrors%100)==0)
        Serial.printf("[freenove35] QSPI band write FAIL y=%d rows=%d errors=%lu\n",
          y,count,(unsigned long)frameErrors);
      return false;
    }
    delay(0);
  }
  ++frameCount;
  uint32_t now=millis();
  if((uint32_t)(now-lastFrameReport)>=5000){
    Serial.printf("[freenove35] LCD frames=%lu errors=%lu heap=%lu psram=%lu\n",
      (unsigned long)frameCount,(unsigned long)frameErrors,
      (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getFreePsram());
    lastFrameReport=now;
  }
  return true;
}
static bool readRegister(uint16_t reg,uint8_t *dest,uint8_t size){
  Wire.beginTransmission(HW::TP_ADDR);
  Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));
  if(Wire.endTransmission(false)!=0)return false;
  if(Wire.requestFrom((uint8_t)HW::TP_ADDR,size)!=(int)size)return false;
  for(int i=0;i<size;++i)dest[i]=Wire.read();
  return true;
}
static bool startTouch(){
  Wire.begin(HW::SDA,HW::SCL,400000);
  pinMode(HW::TP_INT,INPUT);
  pinMode(HW::TP_RST,OUTPUT);
  digitalWrite(HW::TP_RST,LOW);delay(100);
  digitalWrite(HW::TP_RST,HIGH);delay(100);
  uint8_t info[5]={};
  return readRegister(0x0000,&touchVersion,1)&&readRegister(0x0005,info,5);
}
static bool pollTouch(){
  if(!touchOk)return false;
  uint8_t flag=0,count=0,data[7]={};
  if(!readRegister(0x0010,&flag,1)||!(flag&0x08))return false;
  if(!readRegister(0x0009,&count,1)||count==0)return false;
  if(!readRegister(0x0014,data,7)||!(data[0]&0x80))return false;
  uint16_t x=((uint16_t)(data[0]&0x3F)<<8)|data[1];
  uint16_t y=((uint16_t)data[2]<<8)|data[3];
  if(x>=HW::W||y>=HW::H)return false;
  touchX=x;touchY=y;++touches;
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
