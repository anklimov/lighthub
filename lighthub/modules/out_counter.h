
#pragma once
#include "options.h"
#ifndef COUNTER_DISABLE

#include <abstractout.h>
#include <item.h>

class out_counter : public abstractOut {
public:
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
};
#endif
