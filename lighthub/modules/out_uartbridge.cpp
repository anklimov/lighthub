#ifdef UARTBRIDGE_ENABLE

#include "modules/out_uartbridge.h"
#include "Arduino.h"
#include "options.h"
#include "utils.h"
#include "Streaming.h"

#include "item.h"
//#include <ModbusMaster.h>
#include "main.h"
#include <HardwareSerial.h>

#include "main.h"


#include <Udp.h>
#ifndef WIFI_ENABLE
EthernetUDP udpClientA;
EthernetUDP udpClientB;
#else
WiFiUDP udpClientA;
WiFiUDP udpClientB;
#endif

IPAddress       targetIP;
uint16_t        targetPort=5555;

short halfduplex=1;
bool  udpdump=false;
uint32_t timerA = 0;
uint32_t timerB = 0;
uint16_t sizeA  = 0;
uint16_t sizeB  = 0;

//extern aJsonObject *modbusObj;
//extern ModbusMaster node;
//extern short modbusBusy;
//extern void modbusIdle(void) ;


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
    
    aJsonObject * debugIPObj=aJson.getObjectItem(item->itemArg, "ip");
    aJsonObject * debugPortObj=aJson.getObjectItem(item->itemArg, "port");
    aJsonObject * hdObj=aJson.getObjectItem(item->itemArg, "hd");

    if (debugIPObj)
          {
          _inet_aton(debugIPObj->valuestring, targetIP);
           if (udpClientA.begin(SOURCE_PORT_A) && udpClientB.begin(SOURCE_PORT_B))  udpdump=true;
           else {
                  udpClientA.stop(); udpClientB.stop(); udpdump=false; 
                  errorSerial<<F("No sockets available, udpdump disabled")<<endl;
                }
          }

    if (debugPortObj) targetPort = debugPortObj->valueint;     
    if (hdObj) halfduplex = hdObj->valueint;  

  return true;
  }


int  out_UARTbridge::Setup()
{
abstractOut::Setup();  

if (!store) store= (ubPersistent *)item->setPersistent(new ubPersistent);
if (!store)
              { errorSerial<<F("UARTbridge: Out of memory")<<endl;
                return 0;}


 sizeA=0;
 sizeB=0;
 timerA=0;
 timerB=0;           

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
 
 udpClientA.stop();
 udpClientB.stop();
 udpdump=false;

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

void flushA()
{
if (sizeA)
  {  
    if (lanStatus>=HAVE_IP_ADDRESS && udpdump)  udpClientA.endPacket();
    debugSerial<<endl;
  }
timerA=0;   
sizeA=0;
}

void flushB()
{
if (sizeB)
   {   
     if (lanStatus>=HAVE_IP_ADDRESS && udpdump)  udpClientB.endPacket();
      debugSerial<<endl;
   }
timerB=0;
sizeB=0;   
}

int out_UARTbridge::Poll(short cause)
{
  uint8_t chA;
  uint8_t chB;

 // while (MODULE_UATRBRIDGE_UARTA.available() || MODULE_UATRBRIDGE_UARTB.available())
          {

          if (MODULE_UATRBRIDGE_UARTA.available())
              {
              if (!sizeA)
                {
                 if  (lanStatus>=HAVE_IP_ADDRESS && udpdump) udpClientA.beginPacket(targetIP, targetPort);
                 if (halfduplex) flushB(); 
                }
                  
              timerA=millisNZ();
              ////if (timerB) return 1;
              chA=MODULE_UATRBRIDGE_UARTA.read();  
              MODULE_UATRBRIDGE_UARTB.write(chA);
              debugSerial<<F("<")<<((chA<16)?"0":"")<<_HEX(chA);
              if (lanStatus>=HAVE_IP_ADDRESS && udpdump) {udpClientA.write(chA);}
              sizeA++;           
              }

          if (MODULE_UATRBRIDGE_UARTB.available())
              {
              if (!sizeB)
                 { 
                  if (lanStatus>=HAVE_IP_ADDRESS && udpdump) udpClientB.beginPacket(targetIP, targetPort);  
                  if (halfduplex) flushA();
                 } 
              timerB=millisNZ();  
              ////if (timerA) return 1;
              chB=MODULE_UATRBRIDGE_UARTB.read();  
              MODULE_UATRBRIDGE_UARTA.write(chB);
              debugSerial<<F(">")<<((chB<16)?"0":"")<<_HEX(chB);
              if (lanStatus>=HAVE_IP_ADDRESS && udpdump) {udpClientB.write(chB);};
              sizeB++;
              }
          }

if ((timerA && (isTimeOver(timerA,millis(),PDELAY)) || sizeA>=MAX_PDU)) flushA();
if ((timerB && (isTimeOver(timerB,millis(),PDELAY)) || sizeB>=MAX_PDU)) flushB();


/*
if ((lanStatus>=HAVE_IP_ADDRESS) && udpMessageA.length() && (isTimeOver(timerA,millis(),PDELAY) || timerB))
{
  udp.sendTo(udpMessageA,targetIP,targetPort);
  udpMessageA.flush();
  debugSerial<<endl;
  timerA=0;   
}

if ((lanStatus>=HAVE_IP_ADDRESS) && udpMessageB.length() && (isTimeOver(timerB,millis(),PDELAY) || timerA))
{
  udp.sendTo(udpMessageB,targetIP,targetPort);
  udpMessageB.flush();
  debugSerial<<endl;
  timerB=0;   
}

  while (MODULE_UATRBRIDGE_UARTA.available() || MODULE_UATRBRIDGE_UARTB.available())
          {

          if (MODULE_UATRBRIDGE_UARTA.available())
              {
              timerA=millisNZ();
              if (timerB) return 1;
              chA=MODULE_UATRBRIDGE_UARTA.read();  
              MODULE_UATRBRIDGE_UARTB.write(chA);
              debugSerial<<F("<")<<((chA<16)?"0":"")<<_HEX(chA);
              udpMessageA.write(chA);           
              }

          if (MODULE_UATRBRIDGE_UARTB.available())
              {
              timerB=millisNZ();  
              if (timerA) return 1;
              chB=MODULE_UATRBRIDGE_UARTB.read();  
              MODULE_UATRBRIDGE_UARTA.write(chB);
              debugSerial<<F(">")<<((chB<16)?"0":"")<<_HEX(chB);
              udpMessageB.write(chB);
              }
          }
*/

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
