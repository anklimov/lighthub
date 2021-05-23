#ifdef UARTBRIDGE_ENABLE

#include "modules/out_uartbridge.h"
#include "Arduino.h"
#include "options.h"
#include "utils.h"
#include "Streaming.h"

#include "item.h"
#include <ModbusMaster.h>
#include "main.h"
#include <HardwareSerial.h>

extern aJsonObject *modbusObj;
extern ModbusMaster node;
extern short modbusBusy;
extern void modbusIdle(void) ;

/*
struct reg_t
{
  const char verb[4];
  const uint8_t id;
};


#define PAR_I16 1
#define PAR_I32 2
#define PAR_U16 3
#define PAR_U32 4
#define PAR_I8H 5
#define PAR_I8L 6
#define PAR_U8H 7
#define PAR_U8L 8
#define PAR_TENS 9

const reg_t regSize_P[] PROGMEM =
{
  { "i16", (uint8_t) PAR_I16 },
  { "i32", (uint8_t) PAR_I32 },
  { "u16", (uint8_t) PAR_U16 },
  { "u32", (uint8_t) PAR_U32 },
  { "i8h", (uint8_t) PAR_I8H },
  { "i8l", (uint8_t) PAR_I8L },
  { "u8h", (uint8_t) PAR_U8H },
  { "u8l", (uint8_t) PAR_U8L },
  { "x10", (uint8_t) PAR_TENS }
} ;
#define regSizeNum sizeof(regSize_P)/sizeof(reg_t)

int  str2regSize(char * str)
{
  for(uint8_t i=0; i<regSizeNum && str;i++)
      if (strcmp_P(str, regSize_P[i].verb) == 0)
           return pgm_read_byte_near(&regSize_P[i].id);
  return (int) PAR_I16;
}
*/

bool out_UARTbridge::getConfig()
{
  // Retrieve and store template values from global modbus settings
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Object))
  {
    errorSerial<<F("UARTbridge: config failed:")<<(bool)store<<F(",")<<(bool)item<<F(",")<<(bool)item->itemArg<<F(",")<<(item->itemArg->type != aJson_Object)<<F(",")<<endl;
    return false;
  }

  aJsonObject * serialParamObj=aJson.getObjectItem(item->itemArg, "serial");
  if (serialParamObj && serialParamObj->type == aJson_String) store->serialParam = str2SerialParam(serialParamObj->valuestring);
     else store->serialParam = SERIAL_8N1;

  aJsonObject * baudObj=aJson.getObjectItem(item->itemArg, "baud");
  if (baudObj && baudObj->type == aJson_Int && baudObj->valueint) store->baud = baudObj->valueint;
     else store->baud = 9600;

    #if defined (__SAM3X8E__)
    MODULE_UATRBRIDGE_UARTA.begin(store->baud, static_cast <USARTClass::USARTModes> (store->serialParam));
    MODULE_UATRBRIDGE_UARTB.begin(store->baud, static_cast <USARTClass::USARTModes> (store->serialParam));
    #elif defined (ARDUINO_ARCH_ESP8266)
    MODULE_UATRBRIDGE_UARTA.begin(store->baud, static_cast <SerialConfig>(store->serialParam));
    MODULE_UATRBRIDGE_UARTA.begin(store->baud, static_cast <SerialConfig>(store->serialParam));
    #elif defined (ARDUINO_ARCH_ESP32)
    MODULE_UATRBRIDGE_UARTA.begin(store->baud, (store->serialParam),MODULE_UATRBRIDGE_UARTA_RX_PIN,MODULE_UATRBRIDGE_UARTA_TX_PIN);
    MODULE_UATRBRIDGE_UARTB.begin(store->baud, (store->serialParam),MODULE_UATRBRIDGE_UARTB_RX_PIN,MODULE_UATRBRIDGE_UARTB_TX_PIN);
    #else
    MODULE_UATRBRIDGE_UARTA.begin(store->baud, (store->serialParam));
    MODULE_UATRBRIDGE_UARTB.begin(store->baud, (store->serialParam));
    #endif

  return true;
  }


int  out_UARTbridge::Setup()
{
abstractOut::Setup();    
if (!store) store= (ubPersistent *)item->setPersistent(new ubPersistent);
if (!store)
              { errorSerial<<F("UARTbridge: Out of memory")<<endl;
                return 0;}

//store->timestamp=millisNZ();
if (getConfig())
    {
        infoSerial<<F("UARTbridge config loaded ")<< item->itemArr->name<<endl;
        store->driverStatus = CST_INITIALIZED;
        return 1;
      }
else
 {  errorSerial<<F("UARTbridge config error")<<endl;
    store->driverStatus = CST_FAILED;
    return 0;
  }

}

int  out_UARTbridge::Stop()
{
Serial.println("UARTbridge De-Init");

delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
}

int  out_UARTbridge::Status()
{
if (store)
    return store->driverStatus;
return CST_UNKNOWN;
}

int out_UARTbridge::Poll(short cause)
{
  int chA;
  int chB;

  while (MODULE_UATRBRIDGE_UARTA.available())
          {
          chA=MODULE_UATRBRIDGE_UARTA.read();  
          MODULE_UATRBRIDGE_UARTB.write(chA);
          debugSerial<<F("<")<<_HEX(chA);
          }

    while (MODULE_UATRBRIDGE_UARTB.available())
          {
          chB=MODULE_UATRBRIDGE_UARTB.read();  
          MODULE_UATRBRIDGE_UARTA.write(chB);
          debugSerial<<F(">")<<_HEX(chB);
          }        

return 1;//store->pollingInterval;
};

int out_UARTbridge::getChanType()
{
   return CH_MODBUS;
}


//!Control unified Modbus item  
// Priority of selection sub-items control to:
// 1. if defined standard suffix Code inside cmd
// 2. custom textual subItem
// 3. non-standard numeric  suffix Code equal param id

int out_UARTbridge::Ctrl(itemCmd cmd,   char* subItem, bool toExecute)
{
//int chActive = item->isActive();
//bool toExecute = (chActive>0);
//itemCmd st(ST_UINT32,CMD_VOID);

int suffixCode = cmd.getSuffix();

//                    aJsonObject *typeObj = aJson.getObjectItem(paramObj, "type");
//                    aJsonObject *mapObj = aJson.getObjectItem(paramObj, "map");
//                    aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
//                    uint16_t data = node.getResponseBuffer(posInBuffer);



if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
debugSerial<<F("Forced execution");
case S_SET:
//case S_ESET:
          if (!cmd.isValue()) return 0;

//TODO
          return 1;
          //break;

case S_CMD:
      switch (cmd.getCmd())
          {
          case CMD_ON:

            return 1;

            case CMD_OFF:

            return 1;

            default:
            debugSerial<<F("Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd

    default:
  debugSerial<<F("Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

return 0;
}

#endif
