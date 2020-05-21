
#pragma once
#include "options.h"
#ifndef SPILED_DISABLE
#include <abstractout.h>
#include <item.h>
#ifdef ADAFRUIT_LED

#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif

class out_SPILed : public abstractOut {
public:

    out_SPILed(Item * _item):abstractOut(_item){getConfig();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(short cmd, short n=0, int * Parameters=NULL,  int suffixCode=0, char* subItem=NULL) override;
    int PixelCtrl(itemStore *st, short cmd, int from =0 , int to = 1024, bool show = 1, bool rgb = 0);
    int numLeds;
    int8_t pin;
    int ledsType;
protected:
    void getConfig();
};
#endif
