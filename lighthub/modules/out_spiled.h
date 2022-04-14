
#pragma once
#include "options.h"
#include "colorchannel.h"
#ifndef SPILED_DISABLE
#include <abstractout.h>
#include <item.h>

#ifdef ADAFRUIT_LED
#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif

class out_SPILed : public colorChannel {
public:

    out_SPILed(Item * _item):colorChannel(_item){getConfig();};
    int Setup() override;
    int Stop() override;
    int Status() override;
    int getChanType() override;
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL,  int suffixCode=0, char* subItem=NULL) override;
    //int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true ) override;
    //int PixelCtrl(itemCmd cmd, int from =0 , int to = 1024, bool show = 1) override;
    int numLeds;
    int8_t pin;
    int ledsType;
protected:
    void getConfig();
};
#endif
