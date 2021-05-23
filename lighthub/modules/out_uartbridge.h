#pragma once
#include "options.h"
#ifdef UARTBRIDGE_ENABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include "utils.h"

#if defined(ESP32)
#define serialParamType uint32_t
#else
#define serialParamType uint16_t
#endif

#ifndef MODULE_UATRBRIDGE_UARTA
#define MODULE_UATRBRIDGE_UARTA Serial1
#endif

#ifndef MODULE_UATRBRIDGE_UARTB
#define MODULE_UATRBRIDGE_UARTB Serial2
#endif

#ifndef MODULE_UATRBRIDGE_UARTA_RX_PIN
#define MODULE_UATRBRIDGE_UARTA_RX_PIN 15
#endif

#ifndef MODULE_UATRBRIDGE_UARTA_TX_PIN
#define MODULE_UATRBRIDGE_UARTA_TX_PIN 2
#endif

#ifndef MODULE_UATRBRIDGE_UARTB_RX_PIN
#define MODULE_UATRBRIDGE_UARTB_RX_PIN -1
#endif

#ifndef MODULE_UATRBRIDGE_UARTB_TX_PIN
#define MODULE_UATRBRIDGE_UARTB_TX_PIN -1
#endif

class ubPersistent : public chPersistent  {

public:
//  int addr
  int8_t driverStatus;
  int baud;
  serialParamType serialParam;
  //uint16_t pollingInterval;
  //uint32_t timestamp;
  //aJsonObject * pollingRegisters;
  //aJsonObject * pollingIrs;
  //aJsonObject * parameters;
};



class out_UARTbridge : public abstractOut {
public:

    out_UARTbridge(Item * _item):abstractOut(_item){store = (ubPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true) override;
    int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    ubPersistent * store;
    bool getConfig();
//    int findRegister(int registerNum, int posInBuffer, int regType);
//    void pollModbus(aJsonObject * reg, int regType);
};
#endif
