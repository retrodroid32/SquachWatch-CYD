// SquachWatch T-Deck Plus R1 — USB-flashable hardware bring-up / Wi-Fi detector.
// Standalone from the CYD firmware until GT911/SD/GPS/keyboard hardware validation.
// Hardware mapping: official LILYGO T-Deck Plus / ESP32-S3FN16R8.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include "signatures.h"

namespace Deck {
constexpr uint8_t POWER=10, BACKLIGHT=42, LCD_CS=12, SD_CS=39, LORA_CS=9;
constexpr uint8_t I2C_SDA=18, I2C_SCL=8;
constexpr uint8_t KBD_ADDR=0x55;
constexpr uint8_t TRACK_RIGHT=2, TRACK_UP=3, TRACK_LEFT=1, TRACK_DOWN=15, TRACK_CLICK=0;
constexpr uint8_t GPS_RX=44, GPS_TX=43; // LILYGO upstream utilities.h and GPSShield example
constexpr uint8_t TOUCH_ADDRS[2]={0x5D,0x14}; // GT911 address depends on startup
}

TFT_eSPI lcd;
HardwareSerial gpsPort(1);
uint8_t touchAddress=0;
uint8_t page=0;
uint32_t lastRedraw=0, lastScan=0;
int wifiFound=0, classified=0;
bool wifiRunning=false;
uint32_t gpsSentences=0;
char gpsLine[110]={0};
uint8_t gpsLen=0;
char latestKey='-';
bool screenDirty=true;

static bool i2cRead16(uint8_t addr, uint16_t reg, uint8_t* out, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)reg);
    if (Wire.endTransmission(false)!=0) return false;
    if (Wire.requestFrom(addr,len)!=(int)len) {
        while(Wire.available()) Wire.read();
        return false;
    }
    for(uint8_t i=0;i<len;++i) out[i]=(uint8_t)Wire.read();
    return true;
}
static bool i2cWrite16(uint8_t addr,uint16_t reg,uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)(reg>>8));
    Wire.write((uint8_t)reg);
    Wire.write(value);
    return Wire.endTransmission()==0;
}
static bool getTouch(uint16_t& x,uint16_t& y) {
    if(!touchAddress) return false;
    uint8_t status=0;
    if(!i2cRead16(touchAddress,0x814E,&status,1)) return false;
    if(!(status&0x80)) return false;
    const uint8_t fingers=status&0x0F;
    bool valid=false;
    if(fingers>=1&&fingers<=5) {
        uint8_t data[4]={};
        if(i2cRead16(touchAddress,0x8150,data,4)) {
            x=(uint16_t)data[0]|((uint16_t)data[1]<<8);
            y=(uint16_t)data[2]|((uint16_t)data[3]<<8);
            valid=(x<320&&y<240);
        }
    }
    i2cWrite16(touchAddress,0x814E,0);
    return valid;
}
static void backlightOn() {
    // Match the LILYGO UnitTest::setBrightness(16) power-on sequence:
    // LOW >=3ms resets the 16-step pulse chip; the first HIGH selects
    // maximum brightness. Pulsing another 16 times is NOT the same.
    pinMode(Deck::BACKLIGHT,OUTPUT);
    digitalWrite(Deck::BACKLIGHT,LOW);
    delay(3);
    digitalWrite(Deck::BACKLIGHT,HIGH);
    delayMicroseconds(30);
}
static void newPage(int step) {
    page=(uint8_t)((page+4+step)%4);
    screenDirty=true;
}
static void draw() {
    lcd.fillScreen(TFT_BLACK);
    lcd.fillRect(0,0,320,29,TFT_DARKCYAN);
    lcd.setTextColor(TFT_WHITE,TFT_DARKCYAN);
    lcd.setTextSize(2);
    lcd.drawString("SQUACHWATCH",8,6,2);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_CYAN,TFT_BLACK);
    const char* title[]={"T-DECK PLUS LAB R1","WI-FI DETECTIONS","ONBOARD GPS","HARDWARE STATUS"};
    lcd.drawString(title[page],8,35,2);
    lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    char line[90];
    switch(page) {
      case 0:
        lcd.drawString("EXPERIMENTAL BOARD BRING-UP",8,63,2);
        lcd.drawString("Wi-Fi signature scan enabled",8,89,2);
        lcd.drawString("BLE/LoRa/SD logging not yet ported",8,112,2);
        lcd.drawString("Use trackball / touch to navigate",8,139,2);
        lcd.drawString("Do not flash CYD firmware here",8,164,2);
        break;
      case 1:
        snprintf(line,sizeof line,"Networks: %d   Classified: %d",wifiFound,classified);
        lcd.drawString(line,8,68,2);
        lcd.drawString(wifiRunning?"Scanning nearby Wi-Fi...":"Next scan every 20 seconds",8,99,2);
        lcd.drawString("Matches print on USB serial",8,130,2);
        break;
      case 2:
        snprintf(line,sizeof line,"NMEA messages: %lu",(unsigned long)gpsSentences);
        lcd.drawString(line,8,67,2);
        lcd.drawString("GPS UART 9600 RX=44 TX=43",8,97,2);
        if(gpsSentences) {
           lcd.setTextSize(1);
           lcd.drawString(gpsLine,4,137,1);
        } else lcd.drawString("Waiting for onboard GPS...",8,135,2);
        break;
      case 3:
        snprintf(line,sizeof line,"Heap: %lu / largest %lu",
                 (unsigned long)ESP.getFreeHeap(),
                 (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        lcd.drawString(line,8,64,2);
        snprintf(line,sizeof line,"PSRAM: %s (%lu MB)",psramFound()?"OK":"NOT FOUND",
                 (unsigned long)(ESP.getPsramSize()/1048576));
        lcd.drawString(line,8,88,2);
        snprintf(line,sizeof line,"GT911 I2C: %s",touchAddress?"FOUND":"NOT FOUND");
        lcd.drawString(line,8,112,2);
        snprintf(line,sizeof line,"KEYBOARD: I2C 0x55   Last: %c",latestKey);
        lcd.drawString(line,8,136,2);
        break;
    }
    lcd.drawFastHLine(0,206,320,TFT_DARKGREY);
    lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
    lcd.drawString("< PREV       trackball / touch       NEXT >",9,218,2);
}
static void startWifiScan() {
    if(wifiRunning) return;
    WiFi.scanDelete();
    int rc=WiFi.scanNetworks(true,false,false,150);
    wifiRunning=(rc==WIFI_SCAN_RUNNING || rc>=0);
    lastScan=millis();
}
static void pollWifi() {
    if(wifiRunning) {
        int n=WiFi.scanComplete();
        if(n==WIFI_SCAN_RUNNING) return;
        wifiRunning=false;
        wifiFound=n>0?n:0;
        classified=0;
        for(int i=0;i<wifiFound;++i) {
            uint8_t mac[6]={};
            String bssid=WiFi.BSSIDstr(i);
            unsigned int m[6]={};
            if(sscanf(bssid.c_str(),"%02x:%02x:%02x:%02x:%02x:%02x",
                      &m[0],&m[1],&m[2],&m[3],&m[4],&m[5])==6)
                for(int j=0;j<6;++j)mac[j]=(uint8_t)m[j];
            Confidence conf=Confidence::LOW_CONF;
            DetectionType detected=lookupSsid(WiFi.SSID(i).c_str());
            if(detected==DetectionType::UNKNOWN) detected=lookupOui(mac,&conf);
            if(detected!=DetectionType::UNKNOWN) {
                ++classified;
                Serial.printf("[DETECT] %-16s RSSI=%d SSID=%s BSSID=%s type=%s\n",
                              detectionTypeName(detected),WiFi.RSSI(i),
                              WiFi.SSID(i).c_str(),bssid.c_str(),
                              ssidVendorName(WiFi.SSID(i).c_str())?ssidVendorName(WiFi.SSID(i).c_str()):"OUI");
            }
        }
        Serial.printf("[wifi] networks=%d signature_hits=%d\n",wifiFound,classified);
        WiFi.scanDelete();
        screenDirty=true;
    }
    if(!wifiRunning && millis()-lastScan>20000) startWifiScan();
}
static void pollGps() {
    while(gpsPort.available()) {
        char c=(char)gpsPort.read();
        if(c=='\n') {
            gpsLine[gpsLen]=0;
            if(gpsLen>6&&gpsLine[0]=='$') ++gpsSentences;
            gpsLen=0;
            if(page==2) screenDirty=true;
        } else if(c!='\r') {
            if(gpsLen<sizeof(gpsLine)-1) gpsLine[gpsLen++]=c;
            else gpsLen=0;
        }
    }
}
static void pollControls() {
    static uint32_t lastInput=0;
    if(millis()-lastInput<230) return;
    if(digitalRead(Deck::TRACK_LEFT)==LOW || digitalRead(Deck::TRACK_UP)==LOW) {
       newPage(-1);lastInput=millis();return;
    }
    if(digitalRead(Deck::TRACK_RIGHT)==LOW || digitalRead(Deck::TRACK_DOWN)==LOW) {
       newPage(1);lastInput=millis();return;
    }
    uint16_t x,y;
    if(getTouch(x,y) && y>190) {
       newPage(x<160?-1:1);lastInput=millis();return;
    }
    if(Wire.requestFrom((uint8_t)Deck::KBD_ADDR,(uint8_t)1)==1) {
       uint8_t key=Wire.read();
       if(key>0&&key<127) {
          latestKey=(char)key;screenDirty=true;
          if(key=='a'||key=='h'||key==',')newPage(-1);
          if(key=='d'||key=='l'||key=='.')newPage(1);
          lastInput=millis();
       }
    }
}
void setup() {
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    delay(200);
    Serial.println("\n[SQUACHWATCH] T-Deck Plus experimental hardware R1");
    // Isolate all other SPI CS lines before initializing the display.
    pinMode(Deck::POWER,OUTPUT); digitalWrite(Deck::POWER,HIGH);
    for(uint8_t cs:{Deck::LCD_CS,Deck::SD_CS,Deck::LORA_CS}) {
        pinMode(cs,OUTPUT); digitalWrite(cs,HIGH);
    }
    // LILYGO Keyboard_T_Deck_Master.ino delays 500ms after power enable.
    delay(500);
    lcd.init();
    lcd.setRotation(1);
    lcd.invertDisplay(true);
    backlightOn();
    Wire.begin(Deck::I2C_SDA,Deck::I2C_SCL,400000);
    for(uint8_t addr:Deck::TOUCH_ADDRS) {
        Wire.beginTransmission(addr);
        if(Wire.endTransmission()==0) {touchAddress=addr;break;}
    }
    // Trackball center is GPIO0 (BOOT), keep input-only. Vendor map G01..G04.
    for(uint8_t p:{Deck::TRACK_RIGHT,Deck::TRACK_UP,Deck::TRACK_LEFT,Deck::TRACK_DOWN,Deck::TRACK_CLICK})
        pinMode(p,INPUT_PULLUP);
    // Upstream keyboard backlight default brightness command 0x02.
    Wire.beginTransmission(Deck::KBD_ADDR);
    Wire.write(0x02);
    Wire.write(127);
    Wire.endTransmission();
    gpsPort.begin(9600,SERIAL_8N1,Deck::GPS_RX,Deck::GPS_TX);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false,false);
    Serial.printf("[hw] PSRAM=%s size=%lu touch=0x%02x heap=%lu\n",
                  psramFound()?"yes":"no",(unsigned long)ESP.getPsramSize(),
                  touchAddress,(unsigned long)ESP.getFreeHeap());
    draw();
    startWifiScan();
}
void loop() {
    pollGps();
    pollWifi();
    pollControls();
    if(screenDirty || millis()-lastRedraw>1500) {
       screenDirty=false; lastRedraw=millis();draw();
    }
    delay(5);
}
