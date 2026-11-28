#pragma once
#include "state.h"
#include <TFT_eSPI.h>
#if defined(FREENOVE35_N)
#define SQW_INFO_CARD 1
namespace InfoCard {
void feed(const Detection& d,const char* title,const char* body,bool privateMode,uint32_t now);
void clear();
bool active(uint32_t now);
void draw(TFT_eSPI& t,int w,int h);
bool tapCloses(int x,int y,int w,int h);
}
#else
#define SQW_INFO_CARD 0
#endif
