
#pragma once
#include "options.h"
//#ifndef DMX_DISABLE
#ifdef XXXX

#include <abstractout.h>
#include <item.h>

#ifdef ADAFRUIT_LED
#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif

class out_dmx : public abstractOut {
public:

    out_dmx(Item * _item):abstractOut(_item){getConfig();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, int suffixCode=0, char* subItem=NULL) override;
    int PixelCtrl(itemCmd cmd, int from =0 , int to = 1024, bool show = 1);
    int numLeds;
    int8_t pin;
    int ledsType;
protected:
    void getConfig();
};
#endif
