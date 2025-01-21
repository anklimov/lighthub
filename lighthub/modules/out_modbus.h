#pragma once
#include "options.h"
#ifndef MBUS_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <utils.h>



class mbPersistent : public chPersistent  {

public:

  int baud;
  serialParamType serialParam;
  uint16_t pollingInterval;
  uint32_t timestamp;
  aJsonObject * pollingRegisters;
  aJsonObject * pollingIrs;
  aJsonObject * pollingCoils;
  aJsonObject * poolingDiscreteIns;
  aJsonObject * parameters;
};

#define MB_NEED_SEND  8
#define MB_SEND_ERROR 4
#define MB_SEND_ATTEMPTS 3
#define MB_VALUE_OUTDATED 1


class out_Modbus : public abstractOut {
public:

    out_Modbus(Item * _item):abstractOut(_item){store = (mbPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    //int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    mbPersistent * store;
    bool getConfig();
    itemCmd  findRegister(uint16_t registerNum, uint16_t posInBuffer, uint8_t regType, uint16_t registerFrom, uint16_t registerTo, bool doExecution = true, bool * submitParam = NULL);
    void pollModbus(aJsonObject * reg, int regType);
    void initLine();
    int  sendModbus(char * paramName, aJsonObject * outValue);
    int  sendItemCmd(aJsonObject *templateParamObj, itemCmd cmd);
    int  createLastMeasured(char * name);
    int  createLastMeasured(aJsonObject * execObj);
    aJsonObject * getLastMeasured(char * name);
    aJsonObject * getLastMeasured(aJsonObject * execObj);
};
#endif
