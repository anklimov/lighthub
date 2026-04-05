#pragma once
#include "options.h"
#ifndef MULTIVENT_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <PID_v1.h>

#define AC_BOOST_HIGH_TEMP 30
#define AC_PRESET_TEMP 21
#define AC_BOOST_LOW_TEMP 17
#define AC_BOOST_TRESHOLD 230
#define AC_BOOST_DEADBAND 10

#define AC_SUPPRESS_FAN 1
#define AC_SUPPRESS_SET 2
#define AC_SUPPRESS_CMD 4


class out_Multivent : public abstractOut {
public:

    //out_Multivent(Item * _item):abstractOut(_item){getConfig();};
    //out_Multivent(){};
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};    
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int isActive() override;
    int getChanType() override;
    //int getDefaultStorageType(){return ST_PERCENTS255;};
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    int fanCtrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool force = false);
protected:
    void getConfig();
    int  sendACcmd (itemCmd cmd);
    void checkACcmd (int acCmd);
    void checkACfan (int acFan);
    void checkACset (int acSet);
    void setPassiveMode(aJsonObject* zone, bool mode);
    uint32_t getFlag   (aJsonObject* zone, uint32_t flag);
    void setFlag   (aJsonObject* zone, uint32_t flag);
    void clearFlag (aJsonObject* zone, uint32_t flag);
    void enablePid(aJsonObject* zone, int enable, int direction = -1);
    bool pidEnabled(aJsonObject* pidObj);
    void setBoost(itemCmd);
    void resetBoost();
    void notifyState(itemCmd state);


    aJsonObject * gatesObj;
    aJsonObject * acObj;
    //float acTemp;
};
#endif
