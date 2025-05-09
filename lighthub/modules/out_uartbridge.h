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

#define SOURCE_PORT_A 5551
#define SOURCE_PORT_B 5552
#define MAX_PDU 64

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

  int baud;
  serialParamType serialParam;
  //uint16_t pollingInterval;
  //uint32_t timestamp;
  //aJsonObject * pollingRegisters;
  //aJsonObject * pollingIrs;
  //aJsonObject * parameters;
};

#define PDELAY 10 

class out_UARTbridge : public abstractOut {
public:

   // out_UARTbridge(Item * _item):abstractOut(_item){store = (ubPersistent *) item->getPersistent();};

    out_UARTbridge():store(NULL){};
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (ubPersistent *) item->getPersistent();} else store = NULL;};


    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    //int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    ubPersistent * store;
    bool getConfig();
//    int findRegister(int registerNum, int posInBuffer, int regType);
//    void pollModbus(aJsonObject * reg, int regType);
};
#endif
