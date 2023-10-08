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
  int8_t driverStatus;
  uint32_t timestamp;
  uint32_t lastSuccessTS;
};

#define MB_NEED_SEND  8
#define MB_SEND_ERROR 4
#define MB_SEND_ATTEMPTS 3

#define M_CONNECTING  10
#define M_CONNECTED 11 
#define M_POLLING1 12
#define M_POLLING2 13
#define M_POLLING3 14
#define M_POLLING4 15
#define M_POLLING5 16
#define M_POLLING6 17
#define M_POLLING7 18
#define M_POLLING8 19

#define RET_SUCCESS 0
#define RET_INVALID_PARAM 1
#define RET_INTERROR 2
#define RET_ACCESS_LEVEL 3
#define RET_RTC_ERR 4
#define RET_NOT_CONNECTED 3


class out_Mercury : public abstractOut {
public:

    out_Mercury(Item * _item):abstractOut(_item){store = (mercuryPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    int getDefaultStorageType(){return ST_INT32;};


protected:
    mercuryPersistent * store;
    uint16_t pollingInterval;
    bool getConfig();
    void initLine(bool full = false);
    void setStatus(short);
    short connectMercury();
    short disconnectMercury();
    short getCurrentVal12(byte param, String topic,int divisor=1);
    short getCurrentVal15(byte param, String topic,int divisor=1);
    short getCounters  (byte when, String topic,int divisor=1);  

};
#endif
