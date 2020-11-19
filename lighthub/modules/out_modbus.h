#pragma once
#include "options.h"
#ifndef MBUS_DISABLE
#include <abstractout.h>
#include <item.h>


class mbPersistent : public chPersistent  {

public:
//  int addr
  int8_t driverStatus;
  int baud;
  uint16_t serialParam;
  uint16_t pollingInterval;
  uint32_t timestamp;
  aJsonObject * pollingRegisters;
  aJsonObject * parameters;

};



class out_Modbus : public abstractOut {
public:

    out_Modbus(Item * _item):abstractOut(_item){store = (mbPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    mbPersistent * store;
    bool getConfig();
    int findRegister(int registerNum, int posInBuffer);
};
#endif
