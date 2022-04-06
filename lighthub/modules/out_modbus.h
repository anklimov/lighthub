#pragma once
#include "options.h"
#ifndef MBUS_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <utils.h>



class mbPersistent : public chPersistent  {

public:
//  int addr
  int8_t driverStatus;
  int baud;
  serialParamType serialParam;
  uint16_t pollingInterval;
  uint32_t timestamp;
  aJsonObject * pollingRegisters;
  aJsonObject * pollingIrs;
  aJsonObject * parameters;
};

#define MB_NEED_SEND  1
#define MB_SEND_ERROR 2


class out_Modbus : public abstractOut {
public:

    out_Modbus(Item * _item):abstractOut(_item){store = (mbPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;
    int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    mbPersistent * store;
    bool getConfig();
    int  findRegister(int registerNum, int posInBuffer, int regType);
    void pollModbus(aJsonObject * reg, int regType);
    void initLine();
    int  sendModbus(char * paramName, int32_t value, uint8_t regType);
};
#endif
