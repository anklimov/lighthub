#pragma once
#include "options.h"
#ifndef MULTIVENT_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"


//static int8_t motorQuote = 0;

class out_Multivent : public abstractOut {
public:

    out_Multivent(Item * _item):abstractOut(_item){getConfig();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    //int isActive() override;
    int getChanType() override;
    int getDefaultStorageType(){return ST_PERCENTS255;};
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;
protected:
    void getConfig();
    aJsonObject * gatesObj;
};
#endif