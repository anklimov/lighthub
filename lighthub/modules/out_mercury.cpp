//Спецификация протокола тут
//https://ftp.owen.ru/CoDeSys3/04_Library/05_3.5.11.5/02_Libraries/02_vendor_protocols/merkuriy-sistema-komand-sogl-1-2021-02-02.pdf
// Использованы фрагменты кода отсюда: https://arduino.ru/forum/pesochnitsa-razdel-dlya-novichkov/arduino-mega-merkurii-2032t-rs485
#ifdef MERCURY_ENABLE

#include "modules/out_mercury.h"
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
extern uint32_t mbusSlenceTimer;



bool out_Mercury::getConfig()
{
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Array) || aJson.getArraySize(item->itemArg)<6)
  {
    errorSerial<<F("Mercury: config failed:")<<(bool)store<<F(",")<<(bool)item<<F(",")<<(bool)item->itemArg<<F(",")<<(item->itemArg->type != aJson_Array)<<F(",")<< (aJson.getArraySize(item->itemArg)<6)<<endl;
    return false;
  }
  
  aJsonObject * delayObj=aJson.getArrayItem(item->itemArg, 5);
  if (delayObj) pollingInterval = delayObj->valueint;
          else pollingInterval = 10000;

  return true;
}

void out_Mercury::initLine(bool full)
{
  int baud;
  serialParamType serialParam;

  aJsonObject * serialParamObj=aJson.getArrayItem(item->itemArg, 2);
  if (serialParamObj && serialParamObj->type == aJson_String) serialParam = str2SerialParam(serialParamObj->valuestring);
     else serialParam = SERIAL_8N1;

  aJsonObject * baudObj=aJson.getArrayItem(item->itemArg, 1);
  if (baudObj && baudObj->type == aJson_Int && baudObj->valueint) baud = baudObj->valueint;
     else baud = 9600;

    #if defined (__SAM3X8E__)
    modbusSerial.begin(baud, static_cast <USARTClass::USARTModes> (serialParam));
    #elif defined (ARDUINO_ARCH_ESP8266)
    modbusSerial.begin(baud, static_cast <SerialConfig>(serialParam));
    #elif defined (ESP32)
    if (full) modbusSerial.begin(store->baud, (serialParam),MODBUS_UART_RX_PIN,MODBUS_UART_TX_PIN);
        else  modbusSerial.updateBaudRate (baud); //Some terrible error in ESP32 core with uart reinit
    #else
    modbusSerial.begin(baud, (serialParam));
    #endif
    //debugSerial<<F("Mercury: ")<< baud << F(" <")<< serialParam<< F("> addr:")<<item->getArg(0)<<endl;
    node.begin(item->getArg(0), modbusSerial);
}



int  out_Mercury::Setup()
{
abstractOut::Setup();    
if (!store) store= (mercuryPersistent *)item->setPersistent(new mercuryPersistent);
if (!store)
              { errorSerial<<F("Mercury: Out of memory")<<endl;
                return 0;}

store->timestamp=millisNZ();
if (getConfig())
    {
        infoSerial<<F("Mercury: config loaded ")<< item->itemArr->name<<endl;
        store->driverStatus = CST_INITIALIZED;
        store->lastSuccessTS = 0;
          initLine(true);
        return 1;
      }
else
 {  errorSerial<<F("Mercury: config error")<<endl;
    store->driverStatus = CST_FAILED;
    return 0;
  }

}

int  out_Mercury::Stop()
{
debugSerial.println("Mercury: De-Init");
disconnectMercury();
delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
}

int  out_Mercury::Status()
{
if (store)
    return store->driverStatus;
return CST_UNKNOWN;
}


void out_Mercury::setStatus(short status)
{
if (store) store->driverStatus=status;
}

short out_Mercury::connectMercury()
{
if (!item || !item->itemArg || modbusBusy) return  RET_INTERROR;
uint8_t buffer[10];
uint8_t ret;  
modbusBusy=1;
initLine();

buffer[1]=1; //Login

aJsonObject * levelObj=aJson.getArrayItem(item->itemArg, 3);
if (levelObj && levelObj->type == aJson_Int) buffer[2] = levelObj->valueint;
   else errorSerial<<F("Mercury: wrong accesslevel")<<endl;
   
aJsonObject * passObj=aJson.getArrayItem(item->itemArg, 4);
if (passObj && passObj->type == aJson_String && (strlen (passObj->valuestring) == 6)) memcpy((char*) buffer+3,passObj->valuestring,6);
   else if (passObj && passObj->type == aJson_Array && (aJson.getArraySize (passObj) == 6)) 
                        {
                        //debugSerial<<F("Mercury: Pass :");
                        for(short i=0;i<6;i++) 
                              {
                              aJsonObject * digitObj=aJson.getArrayItem(passObj,i);
                              if (digitObj && digitObj->type==aJson_Int) 
                                      {
                                      buffer[3+i]=digitObj->valueint; 
                                      //debugSerial<< digitObj->valueint<< F(",");
                                      }
                              }
                        debugSerial<<endl;      
                        }
        else errorSerial<<F("Mercury: passwort must be ether String[6] or Array[6]")<<endl;


debugSerial<<F("Mercury: Connect ")<< item->itemArr->name<<F(" level:")<< buffer[2] ;//<< F(" pass:")<< passObj->valuestring;
ret=node.ModbusRawTransaction(buffer,9,4);
modbusBusy=0;
if (!ret) ret = buffer[1];
debugSerial<<F(" res:")<< ret << endl;
mbusSlenceTimer = millisNZ(); 
return ret;
}


short out_Mercury::disconnectMercury()
{
if (!item || !item->itemArg || modbusBusy) return  RET_INTERROR;
uint8_t buffer[4];
uint8_t ret;  
modbusBusy=1;
initLine();

buffer[1]=2; //Close
ret=node.ModbusRawTransaction(buffer,2,4);
modbusBusy=0;

if (!ret) ret = buffer[1];
debugSerial<<F("Mercury: Disconnect ")<< item->itemArr->name<<F(" res:")<< ret << endl;
mbusSlenceTimer = millisNZ(); 
return ret;
}

///////////////////// команды//////////////////////////////////////////////////////
//byte testConnect[] = { 0x00, 0x00 };             //тест связи
//byte Sn[]          = { 0x00, 0x08, 0x05 };       //запрос сетевого адреса
//byte Open[]        = { 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}; //открыть канал с паролеи
//byte Close[]       = { 0x00, 0x02 };             // закрыть канал (а то ждать 4 минуты)
//byte activPower[]  = { 0x00, 0x05, 0x00, 0x00 }; //суммарная энергия прямая + обратная + активная + реактивная
//byte ind[]         = { 0x00, 0x03, 0x01, 0x01, 0x00, 0x01, 0x00, 0x1F, 0x00, 0x1F, 0x00 }; //маска индикации работает только для автоматического режима
//byte Angle[]       = { 0x00, 0x08, 0x16, 0x51 }; // углы между фазами 1и2, 1и3, 2и3


////////////////////////подпрограмма чтения показаний счётчика////////////////////////////////////                     
#define M_BEGIN_YESTERDAY 0xD0 // показания на начало вчерашних суток 
#define M_BEGIN_TODAY 0x21     // показания на начало сегодняшних суток
#define M_NOW  0x00            // показания на сейчас 
//2.5.17 Запросы на чтение массивов регистров накопленной энергии
short out_Mercury::getCounters(byte when, String topic, int divisor)
{
  if (!item || !item->itemArg || modbusBusy) return  RET_INTERROR;
  modbusBusy=1;
  initLine();

  uint8_t response[19]{}; //1+4+4+4+4+2
  response[1]=0x05;
  response[2]=when;       // показания на начало вчерашних суток 0xD0
                 
  uint8_t ret=node.ModbusRawTransaction(response, 4 , sizeof(response));
  if (!ret) 

   { 
    unsigned long r = 0;
    r |= (unsigned long)response[2]<<24;
    r |= (unsigned long)response[1]<<16;
    r |= (unsigned long)response[4]<<8;
    r |= (unsigned long)response[3];

   publishTopic(item->itemArr->name, (float) r/1000,(topic+"/Active").c_str());
//Обратную (5-8) пропускаем
    r = 0;
    r |= (unsigned long)response[10]<<24;
    r |= (unsigned long)response[9]<<16;
    r |= (unsigned long)response[12]<<8;
    r |= (unsigned long)response[11];
   publishTopic(item->itemArr->name, (float) r/1000,(topic+"/Reactive").c_str());
//Обратную (13-16) пропускаем
   };

  debugSerial<<F("Mercury: getCounters ")<< item->itemArr->name << F(" res:")<< ret << endl;
  modbusBusy=0; 
  mbusSlenceTimer = millisNZ(); 
  return ret;
  }  

/////////////////////////углы, напряжения токи по фазам//////////////////////////////////////////////////
#define VOLTAGE 0x11 //0x11 напряжения пофазно, 100делитель
#define CURRENT 0x21 //0x21 токи пофазно
#define ANGLES  0x51 //0x51 углы между напряжениями фаз, 100делитель


#define M_COSF 0x31
#define M_ANGLE_DEF 0x30
#define M_PWR 0x00
short out_Mercury::getCurrentVal12(byte param, String topic, int divisor)  

{         
  if (!item || !item->itemArg || modbusBusy) return  RET_INTERROR;
  modbusBusy=1;
  initLine();    

  uint8_t response[12]{};    

  response[1]=0x08; //2.6 Запросы на чтение параметров
  response[2]=0x16; //2.6.16 Чтение вспомогательных параметров: мгновенной активной,реактивной, полной мощности,
                    // фазных и линейных, напряжений,тока, коэффициента мощности,частоты, небаланса
                    // Ответ 12 (9) двоичных байт. Два старших разряда старшего байта указывают положение вектора полной мощности и
                    // должны маскироваться (см. п. 1.5)
  response[3]=param;
                 
  uint8_t ret=node.ModbusRawTransaction(response, 4 , sizeof(response));              
  if (!ret)
  { 
        long r = 0;
        r |= (long)response[1]<<16;
        r |= (long)response[3]<<8;
        r |= (long)response[2];
        publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/A").c_str());
        
        r = 0;
        r |= (long)response[4]<<16;
        r |= (long)response[6]<<8;
        r |= (long)response[5];
        publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/B").c_str());

        r=0;
        r |= (long)response[7]<<16;
        r |= (long)response[9]<<8;
        r |= (long)response[8];
        publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/C").c_str());
  } 
debugSerial<<F("Mercury: getCurrentVal12 ")<< item->itemArr->name << F(" res:")<< ret << endl;
modbusBusy=0;    
mbusSlenceTimer = millisNZ(); 
return ret;
}

//cosSN(); //0x31 углы между напряжениями фазы и током COSf  
/////////////////////////COSf по фазам//////////////////////////////////////////////////
short out_Mercury::getCurrentVal15(byte param, String topic, int divisor)  
{       
  if (!item || !item->itemArg || modbusBusy) return  RET_INTERROR;
            
  modbusBusy=1;
  initLine();

  uint8_t response[15]{};     

  response[1]=0x08;
  response[2]=0x16;
  response[3]=param;

  uint8_t ret=node.ModbusRawTransaction(response, 4 , sizeof(response));  

  if (!ret)
  {
  long r = 0;
  r |= (long)response[1]<<16;
  r |= (long)response[3]<<8;
  r |= (long)response[2];

  publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/sum").c_str());  

  r = 0;
  r |= (long)response[4]<<16;
  r |= (long)response[6]<<8;
  r |= (long)response[5];

  publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/A").c_str());  

  r = 0;
  r |= (long)response[7]<<16;
  r |= (long)response[9]<<8;
  r |= (long)response[8];

  publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/B").c_str());    

  r=0;
  r |= (long)response[10]<<16;
  r |= (long)response[12]<<8;
  r |= (long)response[11];
   
  publishTopic(item->itemArr->name, (float) r/divisor,(topic+"/C").c_str());  
  }
 debugSerial<<F("Mercury: getCurrentVal15 ")<< item->itemArr->name << F(" res:")<< ret << endl; 
 modbusBusy=0;   
 mbusSlenceTimer = millisNZ(); 
 return ret;
}

int out_Mercury::Poll(short cause)
{
//bool lineInitialized = false;    
if (cause==POLLING_SLOW) return 0;  
if (modbusBusy || ( mbusSlenceTimer && !isTimeOver(mbusSlenceTimer,millis(),100))) return 0;
if (store->driverStatus == CST_FAILED) return 0;
if (!getConfig()) return 0;

 switch (Status())
   {
    case CST_INITIALIZED:
    if (isTimeOver(store->timestamp,millis(),10000UL)) setStatus(M_CONNECTING);
    break;

    case M_CONNECTING:
       switch  (connectMercury()) 
        {
          case RET_SUCCESS: setStatus(M_CONNECTED);
          store->lastSuccessTS=millisNZ();
          break;

          case RET_NOT_CONNECTED: setStatus(CST_FAILED);
          break;

          default:
          setStatus(CST_INITIALIZED);   

        }
        
       break;
     
    case M_CONNECTED:
       if (isTimeOver(store->lastSuccessTS,millis(),240000UL)) setStatus(CST_INITIALIZED);
       if (isTimeOver(store->timestamp,millis(),pollingInterval)) setStatus(M_POLLING1);
       break;

    case M_POLLING1:   
           switch (getCurrentVal12(VOLTAGE,"/voltage",100))
           {
           case RET_SUCCESS: setStatus(M_POLLING2);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;

    case M_POLLING2:   
           switch (getCurrentVal12(CURRENT,"/current",1000))
           {
           case RET_SUCCESS: setStatus(M_POLLING3);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;

    case M_POLLING3:   
           switch (getCurrentVal12(ANGLES,"/angles",100))
           {
           case RET_SUCCESS: setStatus(M_POLLING4);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;

     case M_POLLING4:   
           switch (getCounters(M_NOW,"/counters",1000))
           {
           case RET_SUCCESS: setStatus(M_POLLING5);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;

     case M_POLLING5:   
           switch (getCurrentVal12(M_PWR+1,"/power",100))
           {
           case RET_SUCCESS: setStatus(M_POLLING6);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;
     case M_POLLING6:   
           switch (getCurrentVal12(M_COSF,"/cosf",1000))
           {
           case RET_SUCCESS: setStatus(M_CONNECTED);
           store->timestamp=millisNZ();
           store->lastSuccessTS=millisNZ();
           break;

           case RET_NOT_CONNECTED: setStatus(M_CONNECTING);
           break;

           default:
           setStatus(CST_INITIALIZED);   
           }      
     break;     
   }

return pollingInterval;
};

int out_Mercury::getChanType()
{
   return CH_MBUS;
}



//!Control unified  item  
int out_Mercury::Ctrl(itemCmd cmd,   char* subItem, bool toExecute,bool authorized)
{
if (!store) return -1;

int          suffixCode = cmd.getSuffix();
int          res = -1;


return res;          
}


#endif
