
#pragma once
#include "options.h"
#ifndef COUNTER_DISABLE

#include <abstractout.h>
#include <item.h>

class out_counter : public abstractOut {
public:

    out_counter(Item * _item):abstractOut(_item){ getConfig();};
    void getConfig();
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;

protected:
    //float impulse;
    uint32_t impulse; 
    uint32_t period;
};
#endif
