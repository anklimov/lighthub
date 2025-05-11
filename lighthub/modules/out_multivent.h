#pragma once
#include "options.h"
#ifndef MULTIVENT_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <PID_v1.h>


//static int8_t motorQuote = 0;

class out_Multivent : public abstractOut {
public:

    //out_Multivent(Item * _item):abstractOut(_item){getConfig();};
    //out_Multivent(){};
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};    
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    //int isActive() override;
    int getChanType() override;
    //int getDefaultStorageType(){return ST_PERCENTS255;};
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
protected:
    void getConfig();
    int  sendACcmd (int cmd);
    aJsonObject * gatesObj;
    //float acTemp;
};
#endif
