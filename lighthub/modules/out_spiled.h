
#pragma once
#ifndef SPILED_DISABLE
#include <abstractout.h>
#include <item.h>

class out_SPILed : public abstractOut {
public:

    out_SPILed(Item * _item):abstractOut(_item){};
    int Setup() override;
    int Poll() override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(short cmd, short n=0, int * Parameters=NULL, boolean send=true, int suffixCode=0, char* subItem=NULL) override;
    int PixelCtrl(CHstore *st, short cmd, int from =0 , int to = 1024, bool show = 1, bool rgb = 0);
    int numLeds;
    int pin;
protected:

};
#endif
