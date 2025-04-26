
#pragma once
#include "options.h"
#ifndef RELAY_DISABLE

#include <abstractout.h>
#include <item.h>

class out_relay : public abstractOut {
public:
    void link(Item * _item){abstractOut::link(_item); if (_item) getConfig();};
    void getConfig();
    void relay(bool state);
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

protected:
    short pin;
    bool inverted;
    uint32_t period;
};
#endif
