
#pragma once
#include "options.h"
#ifndef RELAY_DISABLE

#include <abstractout.h>
#include <item.h>

class out_relay : public abstractOut {
public:

    out_relay(Item * _item):abstractOut(_item){ getConfig();};
    void getConfig();
    void relay(bool state);
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;

protected:
    short pin;
    bool inverted;
    uint32_t period;
};
#endif