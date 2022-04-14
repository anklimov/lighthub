
#pragma once
#include "options.h"
#ifndef PWM_DISABLE

#include <abstractout.h>
#include <item.h>
#include "colorchannel.h"

class out_pwm : public colorChannel {
public:

    out_pwm(Item * _item):colorChannel(_item){};
    int Setup() override;
    int Stop() override;
    int Status() override;
    
    int getChanType() override;
    //int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true ) override;

protected:
    short numChannels;
};
#endif
