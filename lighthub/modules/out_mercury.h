#pragma once
#include "options.h"
#ifdef MERCURY_ENABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <utils.h>
#include <Arduino.h>



class mercuryPersistent : public chPersistent  {

public:
  uint32_t timestamp;
  uint32_t lastSuccessTS;
};

#define MB_NEED_SEND  8
#define MB_SEND_ERROR 4
#define MB_SEND_ATTEMPTS 3

#define M_CONNECTING  CST_USER+0 
#define M_CONNECTED CST_USER+1 
#define M_POLLING1 CST_USER+2
#define M_POLLING2 CST_USER+3
#define M_POLLING3 CST_USER+4
#define M_POLLING4 CST_USER+5
#define M_POLLING5 CST_USER+6
#define M_POLLING6 CST_USER+7
#define M_POLLING7 CST_USER+8
#define M_POLLING8 CST_USER+9

#define RET_SUCCESS 0
#define RET_INVALID_PARAM 1
#define RET_INTERROR 2
#define RET_ACCESS_LEVEL 3
#define RET_RTC_ERR 4
#define RET_NOT_CONNECTED 3


class out_Mercury : public abstractOut {
public:

    //out_Mercury(Item * _item):abstractOut(_item){store = (mercuryPersistent *) item->getPersistent();};
    out_Mercury():store(NULL){};
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (mercuryPersistent *) item->getPersistent();} else store = NULL;};
   
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;


protected:
    mercuryPersistent * store;
    uint16_t pollingInterval;
    bool getConfig();
    void initLine(bool full = false);
    short connectMercury();
    short disconnectMercury();
    short getCurrentVal12(byte param, String topic,int divisor=1);
    short getCurrentVal15(byte param, String topic,int divisor=1);
    short getCounters  (byte when, String topic,int divisor=1);  

};
#endif
