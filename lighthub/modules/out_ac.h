
#pragma once
#ifndef AC_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"

#define LEN_B   37
#define B_CUR_TMP   13  //Текущая температура
#define B_CMD       17  // 00-команда 7F-ответ ???
#define B_MODE      23  //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
#define B_FAN_SPD   25  //Скорость 02 - min, 01 - mid, 00 - max, 03 - auto
#define B_SWING     27  //01 - верхний и нижний предел вкл. 00 - выкл. 02 - левый/правый вкл. 03 - оба вкл
#define B_LOCK_REM  28  //80 блокировка вкл. 00 -  выкл
#define B_POWER     29  //on/off 01 - on, 00 - off (10, 11)-Компрессор??? 09 - QUIET
#define B_FRESH     31  //fresh 00 - off, 01 - on
#define B_SET_TMP   35  //Установленная температура

#define S_LOCK S_ADDITIONAL+1
#define S_QUIET S_ADDITIONAL+2
#define S_SWING S_ADDITIONAL+3
//#define S_RAW S_ADDITIONAL+4

extern void modbusIdle(void) ;

class acPersistent : public chPersistent  {
public:
  byte data[37];
  byte power;
  byte mode;
  byte inCheck;
  uint32_t timestamp;
};

class out_AC : public abstractOut {
public:

    out_AC(Item * _item):abstractOut(_item){store = (acPersistent *) item->getPersistent(); getConfig();};
    void getConfig();
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int getDefaultStorageType(){return ST_FLOAT_CELSIUS;};
    int Ctrl(itemCmd cmd,  char* subItem=NULL, bool toExecute=true) override;
    
protected:
    acPersistent * store;
    void InsertData(byte data[], size_t size);
    void SendData(byte req[], size_t size);
    uint8_t portNum;
    #if  defined (__SAM3X8E__)
    UARTClass *ACSerial;
    #else 
    HardwareSerial *ACSerial;
    #endif

};
#endif
