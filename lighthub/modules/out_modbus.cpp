#ifndef MBUS_DISABLE

#include "modules/out_modbus.h"
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

struct reg_t
{
  const char verb[4];
  const uint8_t id;
};

/*
struct serial_t
{
  const char verb[4];
  const serialParamType mode;
};
*/

#define PAR_I16 1
#define PAR_I32 2
#define PAR_U16 3
#define PAR_U32 4
#define PAR_I8H 5
#define PAR_I8L 6
#define PAR_U8H 7
#define PAR_U8L 8
#define PAR_TENS 9
#define PAR_100 10


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
  { "x10", (uint8_t) PAR_TENS },
  { "100", (uint8_t) PAR_100 }
} ;
#define regSizeNum sizeof(regSize_P)/sizeof(reg_t)

/*
const serial_t serialModes_P[] PROGMEM =
{
  { "8E1", (serialParamType) SERIAL_8E1},//(uint16_t) US_MR_CHRL_8_BIT | US_MR_NBSTOP_1_BIT | UART_MR_PAR_EVEN },
  { "8N1", (serialParamType) SERIAL_8N1},
  { "8E2", (serialParamType) SERIAL_8E2},
  { "8N2", (serialParamType) SERIAL_8N2},
  { "8O1", (serialParamType) SERIAL_8O1},
  { "8O2", (serialParamType) SERIAL_8O2},
//  { "8M1", SERIAL_8M1},
//  { "8S1", SERIAL_8S1},
  { "7E1", (serialParamType) SERIAL_7E1},//(uint16_t) US_MR_CHRL_8_BIT | US_MR_NBSTOP_1_BIT | UART_MR_PAR_EVEN },
  { "7N1", (serialParamType) SERIAL_7N1},
  { "7E2", (serialParamType) SERIAL_7E2},
  { "7N2", (serialParamType) SERIAL_7N2},
  { "7O1", (serialParamType) SERIAL_7O1},
  { "7O2", (serialParamType) SERIAL_7O2}
//  { "7M1", SERIAL_7M1},
//  { "7S1", SERIAL_7S1}
} ;

#define serialModesNum sizeof(serialModes_P)/sizeof(serial_t)

serialParamType  str2SerialParam(char * str)
{ debugSerial<<str<<F(" =>");
  for(uint8_t i=0; i<serialModesNum && str;i++)
      if (strcmp_P(str, serialModes_P[i].verb) == 0)
           {

           //debugSerial<< i << F(" ") << pgm_read_word_near(&serialModes_P[i].mode)<< endl;
           if (sizeof(serialModesNum)==4)
             return pgm_read_dword_near(&serialModes_P[i].mode);
           else 
             return pgm_read_word_near(&serialModes_P[i].mode);
         }
  debugSerial<< F("Default serial mode N81 used");
  return static_cast<serialParamType> (SERIAL_8N1);
}
*/

int  str2regSize(char * str)
{
  for(uint8_t i=0; i<regSizeNum && str;i++)
      if (strcmp_P(str, regSize_P[i].verb) == 0)
           return pgm_read_byte_near(&regSize_P[i].id);
  return (int) PAR_I16;
}

bool out_Modbus::getConfig()
{
  // Retrieve and store template values from global modbus settings
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Array) || aJson.getArraySize(item->itemArg)<2 || !modbusObj)
  {
    errorSerial<<F("MBUS: config failed:")<<(bool)store<<F(",")<<(bool)item<<F(",")<<(bool)item->itemArg<<F(",")<<(item->itemArg->type != aJson_Array)<<F(",")<< (aJson.getArraySize(item->itemArg)<2)<<endl;
    return false;
  }
  aJsonObject * templateIdObj = aJson.getArrayItem(item->itemArg, 1);
  if (templateIdObj->type != aJson_String || !templateIdObj->valuestring)
            {
              errorSerial<<F("Invalid template.")<<endl;
              return false;
            }
  aJsonObject * templateObj = aJson.getObjectItem(modbusObj, templateIdObj->valuestring);
  if (! templateObj)
            {
              errorSerial<<F("Modbus template not found: ")<<templateIdObj->valuestring<<endl;
              return false;
            }

  aJsonObject * serialParamObj=aJson.getObjectItem(templateObj, "serial");
  if (serialParamObj && serialParamObj->type == aJson_String) store->serialParam = str2SerialParam(serialParamObj->valuestring);
     else store->serialParam = SERIAL_8N1;

  aJsonObject * baudObj=aJson.getObjectItem(templateObj, "baud");
  if (baudObj && baudObj->type == aJson_Int && baudObj->valueint) store->baud = baudObj->valueint;
     else store->baud = 9600;

    #if defined (__SAM3X8E__)
    modbusSerial.begin(store->baud, static_cast <USARTClass::USARTModes> (store->serialParam));
    #elif defined (ARDUINO_ARCH_ESP8266)
    modbusSerial.begin(store->baud, static_cast <SerialConfig>(store->serialParam));
    #elif defined (ARDUINO_ARCH_ESP32)
    modbusSerial.begin(store->baud, (store->serialParam),MODBUS_UART_RX_PIN,MODBUS_UART_TX_PIN);
    #else
    modbusSerial.begin(store->baud, (store->serialParam));
    #endif

  aJsonObject * pollObj=aJson.getObjectItem(templateObj, "poll");
  if (pollObj && pollObj->type == aJson_Object)
    {
      store->pollingRegisters=aJson.getObjectItem(pollObj, "regs");
      store->pollingIrs=aJson.getObjectItem(pollObj, "irs");
      aJsonObject * delayObj= aJson.getObjectItem(pollObj, "delay");
      if (delayObj) store->pollingInterval = delayObj->valueint;
          else store->pollingInterval = 1000;

    }
  else {store->pollingRegisters=NULL;store->pollingInterval = 1000;store->pollingIrs=NULL;}

  store->parameters=aJson.getObjectItem(templateObj, "par");
  return true;
  //store->addr=item->getArg(0);
  }


int  out_Modbus::Setup()
{
abstractOut::Setup();    
if (!store) store= (mbPersistent *)item->setPersistent(new mbPersistent);
if (!store)
              { errorSerial<<F("MBUS: Out of memory")<<endl;
                return 0;}

store->timestamp=millisNZ();
if (getConfig())
    {
        infoSerial<<F("Modbus config loaded ")<< item->itemArr->name<<endl;
        store->driverStatus = CST_INITIALIZED;
        return 1;
      }
else
 {  errorSerial<<F("Modbus config error")<<endl;
    store->driverStatus = CST_FAILED;
    return 0;
  }

}

int  out_Modbus::Stop()
{
debugSerial.println("Modbus De-Init");

delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
}

int  out_Modbus::Status()
{
if (store)
    return store->driverStatus;
return CST_UNKNOWN;
}



bool readModbus(uint16_t reg, int regType, int count)
{
uint8_t result;
switch (regType) {
    case MODBUS_HOLDING_REG_TYPE:
        result = node.readHoldingRegisters(reg, count);
        break;
    case MODBUS_COIL_REG_TYPE:
        result = node.readCoils(reg, count);
        break;
    case MODBUS_DISCRETE_REG_TYPE:
        result = node.readDiscreteInputs(reg, count);
        break;
    case MODBUS_INPUT_REG_TYPE:
        result = node.readInputRegisters(reg, count);
        break;
    default:
        debugSerial<<F("Not supported reg type\n");
 }
if (result != node.ku8MBSuccess) errorSerial<<F("MBUS: Polling error ")<<_HEX(result)<<endl;
return (result == node.ku8MBSuccess);
}



int out_Modbus::findRegister(int registerNum, int posInBuffer, int regType)
{
  aJsonObject * paramObj = store->parameters->child;
  bool is8bit = false;
  while (paramObj)
          {
            aJsonObject *regObj=NULL;
            switch (regType) {
              case MODBUS_HOLDING_REG_TYPE: regObj = aJson.getObjectItem(paramObj, "reg");
                break;
              case MODBUS_INPUT_REG_TYPE:  regObj = aJson.getObjectItem(paramObj, "ir");
            }  

              if (regObj && regObj->valueint ==registerNum)
                    {
                    aJsonObject *typeObj = aJson.getObjectItem(paramObj, "type");
                    aJsonObject *mapObj = aJson.getObjectItem(paramObj, "map");
                    aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
                    uint16_t data = node.getResponseBuffer(posInBuffer);
                    int8_t    regType = PAR_I16;
                    uint32_t  param =0;
                    itemCmd   mappedParam;
                    char buf[16];

                    //bool isSigned=false;
                    if (typeObj && typeObj->type == aJson_String) regType=str2regSize(typeObj->valuestring);
                    switch(regType) {

                      case PAR_I16:
                      //isSigned=true;
                      param=data;
                      mappedParam.Int((int32_t)data);
                      break;

                      case PAR_U16:
                      param=data;
                      mappedParam.Int((uint32_t)data);
                      break;
                      case PAR_I32:
                      //isSigned=true;
                      param = data | (node.getResponseBuffer(posInBuffer+1)<<16);
                      mappedParam.Int((int32_t)param);
                      break;

                      case PAR_U32:
                      param = data | (node.getResponseBuffer(posInBuffer+1)<<16);
                      mappedParam.Int((uint32_t)param);
                      break;

                      case PAR_U8L:
                      //is8bit=true;
                      param = data & 0xFF;
                      mappedParam.Int((uint32_t)param);
                      break;

                      case PAR_U8H:
                      //is8bit=true;
                      param = (data & 0xFF00) >> 8;
                      mappedParam.Int((uint32_t)param);
                      break;

                      case PAR_TENS:
                      param=data;
                      mappedParam.Tens((int32_t) data);
                      break;

                      case PAR_100:
                      param=data;
                      mappedParam.Tens_raw(data * (TENS_BASE/100));
                      mappedParam.Float((int32_t) data/100.);
                    }

                    if (mapObj && (mapObj->type==aJson_Array || mapObj->type==aJson_Object))
                       mappedParam.doMapping(mapObj);
                       
                    debugSerial << F("MB got ")<<mappedParam.toString(buf,sizeof(buf))<< F(" from ")<<regType<<F(":")<<paramObj->name<<endl;             

                    if (itemParametersObj && itemParametersObj->type ==aJson_Object)
                          {
                          aJsonObject *execObj = aJson.getObjectItem(itemParametersObj,paramObj->name);
                          if (execObj) 
                                      {
                                      bool submitParam=true;  
                                      //Retrive previous data
                                      aJsonObject *lastMeasured = aJson.getObjectItem(execObj,"@S");
                                      if (lastMeasured)
                                      { 
                                                if   (lastMeasured->type == aJson_Int)
                                                  {
                                                  if   (lastMeasured->valueint == param)
                                                        submitParam=false; //supress repeating execution for same val
                                                  else  lastMeasured->valueint=param;
                                                  }
                                      }            
                                      else //No container to store value yet 
                                      {
                                        debugSerial<<F("Add @S: ")<<paramObj->name<<endl;
                                        aJson.addNumberToObject(execObj, "@S", (long) param);
                                      }                                      
                                      if (submitParam)                                       
                                      { // Compare with last submitted val
                                       aJsonObject *settedValue = aJson.getObjectItem(execObj,"@V");                      
                                       if (settedValue && (settedValue->valueint == param))
                                          {    
                                           debugSerial<<F("Ignored - equal with setted val")<<endl;
                                          }  
                                        else executeCommand(execObj, -1, mappedParam);   
                                      }
                                      }
                          }
                    if (!is8bit) return 1;
                    }
            paramObj=paramObj->next;
          }
return is8bit;
}


    void out_Modbus::pollModbus(aJsonObject * reg, int regType)
    {
    if (!reg) return;
    reg=reg->child;  
    //aJsonObject * reg = store->pollingRegisters->child;
    while (reg)
            {
            switch (reg->type)
              {
                case aJson_Int:
                {
                int registerNum = reg->valueint;
                //if (readModbus(registerNum,MODBUS_HOLDING_REG_TYPE,1)) 
                if (readModbus(registerNum,regType,1))
                  {
                    findRegister(registerNum,0,regType);
                //    data = node.getResponseBuffer(j);
                  }
                }
                break;
                case aJson_Array:
                if (aJson.getArraySize(reg)==2)
                {
                  int registerFrom=aJson.getArrayItem(reg, 0)->valueint;
                  int registerTo=aJson.getArrayItem(reg, 1)->valueint;

                  //if (readModbus(registerFrom,MODBUS_HOLDING_REG_TYPE,registerTo-registerFrom+1))
                  if (readModbus(registerFrom,regType,registerTo-registerFrom+1))
                    { debugSerial<<endl;
                      for(int i=registerFrom;i<=registerTo;i++)
                        {
                          findRegister(i,i-registerFrom,regType);
                        }
                      //data = node.getResponseBuffer(j);
                    }

                }

              }
            reg = reg->next;
            }
    }

void out_Modbus::initLine()
{
//store->serialParam=(USARTClass::USARTModes) SERIAL_8N1;
    #if defined (__SAM3X8E__)
    modbusSerial.begin(store->baud, static_cast <USARTClass::USARTModes> (store->serialParam));
    #elif defined (ARDUINO_ARCH_ESP8266)
    modbusSerial.begin(store->baud, static_cast <SerialConfig>(store->serialParam));
    #elif defined (ESP32)
    //modbusSerial.begin(store->baud, store->serialParam);
    //delay(100);
    modbusSerial.updateBaudRate (store->baud); //Some terrible error in ESP32 core with uart reinit
    #else
    modbusSerial.begin(store->baud, (store->serialParam));
    #endif
    debugSerial<< store->baud << F("---")<< store->serialParam<<endl;
    node.begin(item->getArg(0), modbusSerial);
}

int out_Modbus::sendModbus(char * paramName, int32_t value, uint8_t regType)
{
 if (!store) return -1;
 aJsonObject * templateParamObj = aJson.getObjectItem(store->parameters, paramName);
 if (!templateParamObj) return -1; 
 aJsonObject * regObj = aJson.getObjectItem(templateParamObj, "reg");
 if (!regObj) return -2;
 int res = -1;

// int8_t    regType = PAR_I16;
// aJsonObject * typeObj = aJson.getObjectItem(templateParamObj, "type");
// if (typeObj && typeObj->type == aJson_String) regType=str2regSize(typeObj->valuestring);

                    switch(regType) {
                      case PAR_U16:
                      //res = node.writeSingleRegister(regObj->valueint,value);
                      //break;
                      case PAR_I16:  
                      case PAR_TENS:
                      case PAR_100:
                      res = node.writeSingleRegister(regObj->valueint,value);
                      break;

                      
                      break;
                      case PAR_I32:
                      case PAR_U32:
                      res = node.writeSingleRegister(regObj->valueint,value & 0xFFFF);
                      res += node.writeSingleRegister(regObj->valueint+1,value >> 16) ;
                      break;

                      case PAR_U8L:
                      res = node.writeSingleRegister(regObj->valueint,value & 0xFF);
                      
                      break;

                      case PAR_U8H:
                      res = node.writeSingleRegister(regObj->valueint,(value & 0xFFFF)>> 8);
                      break;
                    }
 debugSerial<<F("MB_SEND Res: ")<<res<<F(" ")<<paramName<<" reg:"<<regObj->valueint<<F(" val:")<<value<<F(" ival:")<<(int32_t) value<<endl;
 return ( res == 0);
}

int out_Modbus::Poll(short cause)
{
if (cause==POLLING_SLOW) return 0;  
bool lineInitialized = false;  

if (modbusBusy || (Status() != CST_INITIALIZED)) return 0;

aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj = itemParametersObj->child; 
            while (execObj && execObj->type == aJson_Object)
                          {     
                                 if ((execObj->subtype & MB_NEED_SEND) && !(execObj->subtype & MB_SEND_ERROR))
                                 {
                                 aJsonObject *outValue = aJson.getObjectItem(execObj,"@V");                      
                                      if (outValue)
                                          {    
                                                modbusBusy=1;
                                                if (!lineInitialized)
                                                    {
                                                      lineInitialized=true;  
                                                      initLine();
                                                    }
                                              switch (sendModbus(execObj->name,outValue->valueint,outValue->subtype)) 
                                              {
                                               case 1: //success
                                                 execObj->subtype&=~ MB_NEED_SEND;
                                                 break;
                                               case 0: //fault 
                                                 execObj->subtype |= MB_SEND_ERROR;  
                                                 errorSerial<<F("MBus ")<<execObj->name<<F(" send error")<<endl;
                                                 break;
                                               default: //param not found
                                                 errorSerial<<F("MBus param ")<<execObj->name<<F(" not found")<<endl;
                                                 execObj->subtype&=~ MB_NEED_SEND;
                                               }    
                                          }
                                  }   
                            execObj=execObj->next;          
                           }
           modbusBusy=0;                
           }
  

if (isTimeOver(store->timestamp,millis(),store->pollingInterval))
{

// Clean_up SEND_ERROR flag 
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj = itemParametersObj->child; 
            while (execObj && execObj->type == aJson_Object)
                          {     
                                 if (execObj->subtype & MB_SEND_ERROR) execObj->subtype&=~ MB_SEND_ERROR;
                                 execObj=execObj->next;
                          }     
           }                

// if some polling configured
if (store->pollingRegisters || store->pollingIrs)
  {
    debugSerial<<F("Poll ")<< item->itemArr->name << endl;
    modbusBusy=1;

    if (!lineInitialized)
    {
    lineInitialized=true;  
    initLine();
    }

    pollModbus(store->pollingRegisters,MODBUS_HOLDING_REG_TYPE);
    pollModbus(store->pollingIrs,MODBUS_INPUT_REG_TYPE);
    debugSerial<<F("endPoll ")<< item->itemArr->name << endl;

  //Non blocking waiting to release line
  uint32_t time = millis();
  while (!isTimeOver(time,millis(),50))
             modbusIdle();

  modbusBusy =0;
  }
 store->timestamp=millisNZ();  
}

return store->pollingInterval;
};

int out_Modbus::getChanType()
{
   return CH_MBUS;
}


//!Control unified Modbus item  
// Priority of selection sub-items control to:
// 1. if defined standard suffix Code inside cmd
// 2. custom textual subItem
// 3. non-standard numeric  suffix Code equal param id

int out_Modbus::Ctrl(itemCmd cmd,   char* subItem, bool toExecute)
{
int suffixCode = cmd.getSuffix();
aJsonObject *templateParamObj = NULL;
short mappedCmdVal = 0;

char * suffixStr = subItem;

// trying to find parameter in template with name == subItem (NB!! standard suffixes dint working here)
if (subItem && strlen (subItem) && store) templateParamObj = aJson.getObjectItem(store->parameters, subItem);

if (!templateParamObj && store)
{
  // Fallback - Trying to find template parameter where id == suffixCode
 templateParamObj = store->parameters->child;

  while (templateParamObj)
          {
            //aJsonObject *regObj = aJson.getObjectItem(paramObj, "reg");
            aJsonObject *idObj = aJson.getObjectItem(templateParamObj, "id");
            if (idObj->type==aJson_Int && idObj->valueint == suffixCode) 
                            {
                            suffixStr=templateParamObj->name;  
                            break;
                            }

         //   aJsonObject *mapObj = aJson.getObjectItem(templateParamObj, "mapcmd");
         //   if (mapObj && (mappedCmdVal = cmd.doReverseMappingCmd(mapObj))) break;

            templateParamObj=templateParamObj->next;                       
          }
}


if (templateParamObj)
{
// We have find template for suffix or suffixCode
long  Value = 0;
int8_t    regType = PAR_I16;
aJsonObject * typeObj = aJson.getObjectItem(templateParamObj, "type");

                    if (typeObj && typeObj->type == aJson_String) regType=str2regSize(typeObj->valuestring);
                    switch(regType) {
                      case PAR_I16:
                      case PAR_I32:
                      case PAR_U32:
                      case PAR_U8L:
                      case PAR_U8H:
                        Value=cmd.getInt();
                      break;
                      case PAR_TENS:
                        Value=cmd.getTens();
                      break;  
                      case PAR_100:  
                        Value=cmd.getTens_raw()*(100/TENS_BASE);
                    }

debugSerial<<F("MB suffix:")<<suffixStr<< F(" Val: ")<<Value<<endl;
aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj =  aJson.getObjectItem(itemParametersObj,suffixStr); 
            if (execObj && execObj->type == aJson_Object)
                          {   
                              aJsonObject *polledValue = aJson.getObjectItem(execObj,"@S");                      
                                      if (polledValue && polledValue->type == aJson_Int && (polledValue->valueint == Value))
                                          {    
                                           debugSerial<<F("Ignored - not changed")<<endl;
                                          }

                                      else 
                                      { //Schedule update
                                        execObj->subtype |= MB_NEED_SEND;

                                        aJsonObject *outValue = aJson.getObjectItem(execObj,"@V");                      
                                        if (outValue)
                                            {    
                                            outValue->valueint=Value;
                                            outValue->subtype =regType; 
                                            polledValue->valueint=Value; //to pevent suppressing to change back to previously polled value if this occurs before next polling
                                            }
                                        else //No container to store value yet 
                                            {
                                              debugSerial<<F("Add @V: ")<<execObj->name<<endl;
                                              aJson.addNumberToObject(execObj, "@V", Value);
                                              outValue = aJson.getObjectItem(execObj,"@V");  
                                              if (outValue) outValue->subtype =regType; 
                                            }
                                      }          
                           }  
           }
  


}
else errorSerial<<F("No template for ")<<subItem<<F("/")<<suffixCode<<endl;


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
