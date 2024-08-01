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
uint32_t mbusSlenceTimer = 0;

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
#define PAR_COIL 11


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
 // { "bit", (uint8_t) PAR_COIL }
} ;
#define regSizeNum sizeof(regSize_P)/sizeof(reg_t)

uint16_t swap (uint16_t x) {return  ((x & 0xff) << 8) | ((x & 0xff00) >> 8);}

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
              errorSerial<<F("MBUS: Invalid template.")<<endl;
              return false;
            }
  aJsonObject * templateObj = aJson.getObjectItem(modbusObj, templateIdObj->valuestring);
  if (! templateObj)
            {
              errorSerial<<F("MBUS: Modbus template not found: ")<<templateIdObj->valuestring<<endl;
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
      store->pollingCoils=aJson.getObjectItem(pollObj, "coils");
      store->poolingDiscreteIns=aJson.getObjectItem(pollObj, "dins");
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
        infoSerial<<F("MBUS: config loaded ")<< item->itemArr->name<<endl;
        setStatus(CST_INITIALIZED);
        return 1;
      }
else
 {  errorSerial<<F("MBUS: config error")<<endl;
    setStatus(CST_FAILED);
    return 0;
  }

}

int  out_Modbus::Stop()
{
debugSerial.print("MBUS: De-Init ");
debugSerial.println(item->itemArr->name);

if (store) delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
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
        debugSerial<<F("MBUS: Not supported reg type\n");
        return false;
 }
mbusSlenceTimer = millisNZ(); 
if (result != node.ku8MBSuccess) errorSerial<<F("MBUS: Polling error ")<<_HEX(result)<<endl;
return (result == node.ku8MBSuccess);
}



itemCmd out_Modbus::findRegister(uint16_t registerNum, uint16_t posInBuffer, uint8_t regType, uint16_t registerFrom, uint16_t registerTo, bool doExecution, bool * submitParam)
{
  aJsonObject * paramObj = store->parameters->child;
  
  bool tmpSubmitParam;  
  if (!submitParam) submitParam=&tmpSubmitParam;  
  *submitParam=true;

  //bool is8bit = false;
  while (paramObj)
          { 
            int8_t       parType = PAR_I16;
            aJsonObject *regObj=NULL;
            switch (regType) {
              case MODBUS_HOLDING_REG_TYPE: 
                regObj = aJson.getObjectItem(paramObj, "reg");
                break;
              case MODBUS_INPUT_REG_TYPE:  
                regObj = aJson.getObjectItem(paramObj, "ir");
                break;
              case MODBUS_COIL_REG_TYPE:  
                regObj = aJson.getObjectItem(paramObj, "coil");  
                parType = PAR_COIL;
                break;
              case MODBUS_DISCRETE_REG_TYPE:  
                regObj = aJson.getObjectItem(paramObj, "din");  
                parType = PAR_COIL;  
            }  

              if (regObj && regObj->valueint ==registerNum)
                    {
                    aJsonObject *idObj = aJson.getObjectItem(paramObj, "id");                 
                    aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
                    uint16_t data = node.getResponseBuffer(posInBuffer);
                    bool executeWithoutCheck=false; //Afler recurrent check, all dublicatess and suppressing checked by recurrent 
                    bool submitRecurrentOut = false; //false if recurrent check find duplicates 
                    char buf[16];

                    uint32_t  param =0;
                    itemCmd   mappedParam;
                    aJsonObject *typeObj = aJson.getObjectItem(paramObj, "type");
                    aJsonObject *mapObj = aJson.getObjectItem(paramObj, "map");

                    if (typeObj && typeObj->type == aJson_String) parType=str2regSize(typeObj->valuestring);
                    switch(parType) {

                      case PAR_I16:
                      //isSigned=true;
                      param=data;
                      mappedParam.Int((int32_t)(int16_t)data);
                      break;

                      case PAR_U16:
                      param=data;
                      mappedParam.Int((uint32_t)data);
                      break;

                      case PAR_I32:
                      //isSigned=true;
                      param = swap(data ) | swap(node.getResponseBuffer(posInBuffer+1)<<16);
                      mappedParam.Int((int32_t)param);
                      break;

                      case PAR_U32:
                      param = swap(data )| swap (node.getResponseBuffer(posInBuffer+1)<<16 );
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
                      mappedParam.Tens((int16_t) data);
                      break;

                      case PAR_100:
                      param=data;
                      mappedParam.Tens_raw((int16_t) data * (TENS_BASE/100));
                      mappedParam.Float((int32_t) (int16_t) data/100.);
                      break;

                      case PAR_COIL:
                      param = (node.getResponseBuffer(posInBuffer/16) >> (posInBuffer % 16)) & 1;
                      mappedParam.Int((uint32_t)param);
                    }
                       
                    traceSerial << F("MBUSD:  got ")<<mappedParam.toString(buf,sizeof(buf))<< F(" from type ")<<parType<<F(":")<<paramObj->name<<endl;             

                  if (mapObj && (mapObj->type==aJson_Array || mapObj->type==aJson_Object))
                    {
                       mappedParam=mappedParam.doReverseMapping(mapObj);
                       if (!mappedParam.isCommand() && !mappedParam.isValue())  //Not mapped
                          {  
                          aJsonObject *defMappingObj;
                          defMappingObj = aJson.getObjectItem(mapObj, "def");
                          if (defMappingObj)
                             {
                              switch (defMappingObj->type)
                               {
                                case aJson_Int: //register/coil/.. number
                                  traceSerial<<F("Searching reg#")<<defMappingObj->valueint<<endl; 
                                  if ((defMappingObj->valueint>= registerFrom) && (defMappingObj->valueint<=registerTo))  
                                      {   
                                      mappedParam = findRegister(defMappingObj->valueint,defMappingObj->valueint-registerFrom,regType,registerFrom,registerTo,false,&submitRecurrentOut);
                                      executeWithoutCheck=true;
                                      }
                                  else 
                                  {
                                  debugSerial<<F("def reg# ")<<defMappingObj->valueint<<F(" out of range buffer, fetching")<<endl;    
                                  ////to prevent CORRUPTION if using same buffer
                                  uint16_t localBuffer;
                                  node.setResponseBuffer(&localBuffer,1);
                                  if (readModbus(defMappingObj->valueint,regType,1))
                                    {
                                      mappedParam = findRegister(defMappingObj->valueint,0,regType,defMappingObj->valueint,defMappingObj->valueint,false,&submitRecurrentOut);
                                      executeWithoutCheck=true;
                                    }
                                  node.setDefaultResponseBuffer();                                   
                                  }

                                break;
                                case aJson_String: // parameter name
                                 traceSerial<<F("Searching reg: ")<<defMappingObj->valuestring<<endl; 
                                 if (itemParametersObj && itemParametersObj->type ==aJson_Object) 
                                    {      
                                      //Searching item param for nested mapping 
                                      aJsonObject *itemParObj = aJson.getObjectItem(itemParametersObj,defMappingObj->valuestring);
                                      if (itemParObj) 
                                          {
                                            //Retrive previous data
                                            aJsonObject *lastMeasured = aJson.getObjectItem(itemParObj,"@S");
                                            if (lastMeasured && lastMeasured->type ==aJson_Int)
                                            {
                                            traceSerial<<F("LastKnown value: ")<<lastMeasured->valueint<<endl; 
                                            //Searching template param for nested mapping
                                            aJsonObject * templateParObj = aJson.getObjectItem(store->parameters,defMappingObj->valuestring);
                                              if (templateParObj)
                                                 {
                                                  int8_t nestedParType = PAR_I16;
                                                  
                                                  aJsonObject * nestedTypeObj = aJson.getObjectItem(templateParObj, "type");
                                                  if (nestedTypeObj && nestedTypeObj->type == aJson_String) parType=str2regSize(nestedTypeObj->valuestring);

                                                  switch(nestedParType) {
                                                      case PAR_I16:
                                                      case PAR_I32:
                                                      mappedParam.Int((int32_t)lastMeasured->valueint);
                                                      break;

                                                      case PAR_U32:
                                                      case PAR_U16:
                                                      case PAR_U8L:
                                                      case PAR_U8H:
                                                      case PAR_COIL:
                                                      mappedParam.Int((uint32_t)lastMeasured->valueint);
                                                      break;

                                                      case PAR_TENS:
                                                      mappedParam.Tens((int16_t) data);
                                                      break;

                                                      case PAR_100:
                                                      mappedParam.Tens_raw((int16_t) lastMeasured->valueint * (TENS_BASE/100));
                                                      mappedParam.Float((int32_t) (int16_t) lastMeasured->valueint/100.);
                                                      break;
                                                      default: errorSerial<<F("Invalid regtype")<<endl;
                                                    }

                                                  aJsonObject * nestedMapObj = aJson.getObjectItem(templateParObj, "map");  
                                                  if (nestedMapObj && (nestedMapObj->type==aJson_Array || nestedMapObj->type==aJson_Object)) mappedParam=mappedParam.doReverseMapping(nestedMapObj);
                                                  traceSerial << F("MBUSD: NestedMapped:")<<mappedParam.toString(buf,sizeof(buf))<<endl; 

                                                  if (!(lastMeasured->subtype & MB_VALUE_OUTDATED))
                                                      {
                                                          executeWithoutCheck=true;
                                                          submitRecurrentOut=true;
                                                          lastMeasured->subtype|= MB_VALUE_OUTDATED;
                                                      }

                                                 } 
                                            }
                                          }
                                    }
                                break;
                               }
                             }                          
             }  
                        else   
                           traceSerial << F("MBUSD: Mapped:")<<mappedParam.toString(buf,sizeof(buf))<<endl; 
                    } //mapping       

                    if (doExecution && idObj && idObj->type==aJson_Int) 
                       switch (idObj->valueint)
                        {
                          case S_CMD: 
                          mappedParam.saveItem(item,FLAG_COMMAND);
                            break;
                          case S_SET:
                          mappedParam.saveItem(item,FLAG_PARAMETERS);
                        }

                
                    if (itemParametersObj && itemParametersObj->type ==aJson_Object)
                          {
                          aJsonObject *execObj = aJson.getObjectItem(itemParametersObj,paramObj->name);
                          if (execObj) 
                                      {
                                      aJsonObject * markObj = execObj;
                                      if (execObj->type == aJson_Array) markObj = execObj->child;  
                                      //Retrive previous data
                                      aJsonObject *lastMeasured = aJson.getObjectItem(markObj,"@S");
                                      if (lastMeasured)
                                      { 
                                                if   (lastMeasured->type == aJson_Int)
                                                  {
                                                  if   (lastMeasured->valueint == param)
                                                        *submitParam=false; //supress repeating execution for same val
                                                  else  
                                                        {
                                                        lastMeasured->valueint=param;
                                                        traceSerial<<"MBUS: Stored "<<param<<" to @S of "<<paramObj->name<<endl;
                                                        lastMeasured->subtype&=~MB_VALUE_OUTDATED;
                                                        }
                                                  }
                                      }            
                                      else //No container to store value yet 
                                      {
                                        debugSerial<<F("MBUS: Add @S: ")<<paramObj->name<<endl;
                                        aJson.addNumberToObject(markObj, "@S", (long) param);
                                      }  


                                       if (executeWithoutCheck)
                                            {
                                            
                                            if (doExecution && (submitRecurrentOut || *submitParam)) 
                                                  {
                                                  executeCommand(execObj, -1, mappedParam); 
                                                  *submitParam=true; //if requrrent check has submit smth - report it.  
                                                  }
                                                
                                            return mappedParam;  
                                            }

                                      if (*submitParam)                                       
                                      {   
                                      // Compare with last submitted val (if @V NOT marked as NULL in config)
                                       aJsonObject *settedValue = aJson.getObjectItem(markObj,"@V");                      
                                       if (settedValue && settedValue->type==aJson_Int && (settedValue->valueint == param))
                                          {    
                                           traceSerial<<F("MBUSD: Ignored - equal with setted val")<<endl;
                                           *submitParam=false;
                                          }  
                                        else 
                                          {
                                          if (doExecution) executeCommand(execObj, -1, mappedParam);
                                          // if param updated by device and no new value queued to send - update @V to avoid "Ignored - equal with setted val" 
                                          if (settedValue && !(execObj->subtype & MB_NEED_SEND)) 
                                              settedValue->valueint=param;
                                          }
                                      }
                                      }
                          }
                    //if (submitRecurrentOut) *submitParam=true; //if requrrent check has submit smth - report it.      
                    return mappedParam;
                    }
            paramObj=paramObj->next;
          }
return itemCmd();
}


    void out_Modbus::pollModbus(aJsonObject * reg, int regType)
    {
    if (!reg) return;
    reg=reg->child;  
    while (reg)
            {
            switch (reg->type)
              {
                case aJson_Int:
                {
                int registerNum = reg->valueint; 
                if (readModbus(registerNum,regType,1))
                  {
                    findRegister(registerNum,0,regType,registerNum,registerNum);
                  }
                }
                break;
                case aJson_Array:
                if (aJson.getArraySize(reg)==2)
                {
                  int registerFrom=aJson.getArrayItem(reg, 0)->valueint;
                  int registerTo=aJson.getArrayItem(reg, 1)->valueint;

                  if (readModbus(registerFrom,regType,registerTo-registerFrom+1))
                    { traceSerial<<endl;
                      for(int i=registerFrom;i<=registerTo;i++)
                        {
                          findRegister(i,i-registerFrom,regType,registerFrom,registerTo);
                        }
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
    //debugSerial<< store->baud << F("---")<< store->serialParam<<endl;
    node.begin(item->getArg(0), modbusSerial);
}

int out_Modbus::sendModbus(char * paramName, aJsonObject * outValue)
{
 if (!store) {errorSerial<<F(" internal send error - no store")<<endl; return -1;}

 aJsonObject * templateParamObj = aJson.getObjectItem(store->parameters, paramName);
 if (!templateParamObj) {errorSerial<<F(" internal send error - no template")<<endl; return -1;}

 aJsonObject * regObj = aJson.getObjectItem(templateParamObj, "reg");

 if (!regObj)
    {  
    regObj = aJson.getObjectItem(templateParamObj, "coil");
    if (!regObj) {errorSerial<<F(" internal send error - no reg/coil")<<endl; return -2;}
       else outValue->subtype = PAR_COIL;
    } 
 if (regObj->type != aJson_Int) {errorSerial<<F(" Reg/coil must be int")<<endl; return -2;}   

 aJsonObject * prefetchObj = aJson.getObjectItem(templateParamObj, "prefetch");
 aJsonObject *lastMeasured =  NULL;

 int res = -1;

if (prefetchObj && (prefetchObj->type == aJson_Boolean) && prefetchObj->valuebool) 
                                      {
                                                      int modbusRegType =  (outValue->subtype == PAR_COIL) ? MODBUS_COIL_REG_TYPE:MODBUS_HOLDING_REG_TYPE;
                                                      debugSerial<<F("\nMBUS: prefetching ")<<paramName<<F(" #") <<regObj->valueint << " type:" << modbusRegType << " "; 

                                                      /// to prevent CORRUPTIOIN if using same buffer
                                                      uint16_t localBuffer;
                                                      node.setResponseBuffer(&localBuffer,1);

                                                      bool successRead = readModbus(regObj->valueint,modbusRegType,1);
                                                      

                                                      if (successRead)
                                                        {
                                                          #ifdef PREFETCH_EXEC_IMMEDIALELLY
                                                          // option to execute if changed immediatelly
                                                          bool submited = false;
                                                          findRegister(regObj->valueint,0,modbusRegType,regObj->valueint,regObj->valueint,true,&submited);
                                                          node.setDefaultResponseBuffer();

                                                          if (submited)
                                                                {
                                                                  debugSerial << F("MBUS:")<<paramName<< (" val changed. Write cancelled")<<endl;
                                                                  return -3;
                                                                }
                                                           else   debugSerial << F("MBUS:")<<paramName<< F(" val not changed. Continue")<<endl;  
                                                           
                                                          
                                                          #else
                                                          // Option to skip writing if change, let next polling take of change           
                                                          aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
                                                          if (itemParametersObj && itemParametersObj->type ==aJson_Object)
                                                                                    {
                                                                                    aJsonObject *execObj = aJson.getObjectItem(itemParametersObj,paramName);
                                                                                    if (execObj) 
                                                                                                {
                                                                                                aJsonObject * markObj = execObj;
                                                                                                if (execObj->type == aJson_Array) markObj = execObj->child;  
                                                                                                //Retrive previous data
                                                                                                lastMeasured = aJson.getObjectItem(markObj,"@S");
                                                                                                if (lastMeasured)
                                                                                                { 
                                                                                                          if   (lastMeasured->type == aJson_Int)
                                                                                                            {
                                                                                                            traceSerial<<F(" Last:")<<lastMeasured->valueint<< F(" Now:") << localBuffer<<endl;

                                                                                                            if   (lastMeasured->valueint != localBuffer)
                                                                                                                  {
                                                                                                                    debugSerial << F("MBUS:")<<paramName<< F(" val changed.")<<endl;
                                                                                                                    node.setDefaultResponseBuffer();
                                                                                                                    return -3;
                                                                                                                  }
                                                                                                            else  
                                                                                                                  {
                                                                                                                  debugSerial << F("MBUS:")<<paramName<< F(" val not changed. Continue")<<endl;  
                                                                                                                  }
                                                                                                            }
                                                                                                }            
                                                                                                }
                                                                                    }
                                                        #endif  
                                                       }
                                                       node.setDefaultResponseBuffer();
                                                      

                                      }


                    switch(outValue->subtype) {
                      case PAR_U16:
                      case PAR_I16:  
                      case PAR_TENS:
                      case PAR_100:
                      res = node.writeSingleRegister(regObj->valueint,outValue->valueint);
                      break;
            
                      break;
                      case PAR_I32:
                      case PAR_U32:
                      res = node.writeSingleRegister(regObj->valueint,swap(outValue->valueint & 0xFFFF));
                      res += node.writeSingleRegister(regObj->valueint+1,swap(outValue->valueint >> 16)) ;
                      break;

                      case PAR_U8L:
                      case PAR_I8L:
                      res = node.writeSingleRegister(regObj->valueint,outValue->valueint & 0xFF);                 
                      break;

                      case PAR_U8H:
                      case PAR_I8H:
                      res = node.writeSingleRegister(regObj->valueint,(outValue->valueint & 0xFFFF)>> 8);
                      break;
                      case PAR_COIL:
                      res = node.writeSingleCoil (regObj->valueint,outValue->valueint);
                      break;
                    }
 mbusSlenceTimer = millisNZ();                   
 debugSerial<<F("MBUS res: ")<<res<<F(" ")<<paramName<<" reg:"<<regObj->valueint<<F(" val:")<<outValue->valueint<<endl;

//If wrote - suppress action on poll 
if ((res ==0) && (outValue->type == aJson_Int) && lastMeasured && (lastMeasured->type == aJson_Int)) lastMeasured->valueint = outValue->valueint;


 return ( res == 0);
}

int out_Modbus::Poll(short cause)
{
if (cause==POLLING_SLOW) return 0;  
bool lineInitialized = false;  

if (modbusBusy || (Status() != CST_INITIALIZED) || ( mbusSlenceTimer && !isTimeOver(mbusSlenceTimer,millis(),100))) return 0;

aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj = itemParametersObj->child; 
            bool onceSendOk=false;
            while (execObj && ((execObj->type == aJson_Object) || (execObj->type == aJson_Array)) && !onceSendOk)
                          {     
                             
                                 if ((execObj->subtype & MB_NEED_SEND) && !(execObj->subtype & MB_SEND_ERROR))
                                 {

                                aJsonObject * markObj = execObj;
                                if (execObj->type == aJson_Array) markObj = execObj->child; 

                                 aJsonObject *outValue = aJson.getObjectItem(markObj,"@V");                      
                                      if (outValue)
                                          {    
                                                modbusBusy=1;
                                                if (!lineInitialized)
                                                    {
                                                      lineInitialized=true;  
                                                      initLine();
                                                    }

                                              int sendRes;
                                              int savedValue;
                                              do
                                                  {
                                                  savedValue = outValue->valueint;  
                                                  debugSerial<<"MBUS: SEND "<<item->itemArr->name<<" ";   
                                                  sendRes = sendModbus(execObj->name,outValue);
                                                  }
                                              while (savedValue != outValue->valueint); //repeat sending if target value changed while we're waited for mbus responce

                                              switch (sendRes) 
                                              {
                                               case 1: //success
                                                 execObj->subtype&=~ MB_NEED_SEND;
                                                 onceSendOk=true;
                                                 if (outValue->type == aJson_Int) 
                                                 ///return 1; //relax
                                                 
                                                 break;
                                               case 0: //fault 
                                                 execObj->subtype |= MB_SEND_ERROR;  
                                                 errorSerial<<F("MBUS:  ")<<execObj->name<<F(" send error. ");
                                                 if ((execObj->subtype & 3) != MB_SEND_ATTEMPTS) execObj->subtype++;
                                                 errorSerial<<"Attempt: "<< (execObj->subtype & 3) <<endl;
                                                 break;
                                               case -3:
                                                 errorSerial<<F("MBUS:  param ")<<execObj->name<<F(" sending cancelled")<<endl;
                                                 //outValue->valueint=
                                                 execObj->subtype&=~ MB_NEED_SEND; 
                                                 break;                                              
                                               default: //param not found
                                                 errorSerial<<F("MBUS:  param ")<<execObj->name<<F(" not found")<<endl;
                                                 execObj->subtype&=~ MB_NEED_SEND;
                                               }    
                                          }
                                  }   
                            execObj=execObj->next;          
                           }
           modbusBusy=0;                
           }
  

if (isTimeOver(store->timestamp,millis(),store->pollingInterval) && ( !mbusSlenceTimer || isTimeOver(mbusSlenceTimer,millis(),100)))
{

// Clean_up FLAG_SEND_ERROR flag 
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj = itemParametersObj->child; 
            while (execObj && ((execObj->type == aJson_Object) || (execObj->type == aJson_Array)))
                          {     
                                 if (execObj->subtype & MB_SEND_ERROR) execObj->subtype&=~ MB_SEND_ERROR;
                                 if ((execObj->subtype & 0x3) >= MB_SEND_ATTEMPTS) 
                                                                        {
                                                                        //execObj->subtype&=~ MB_NEED_SEND;
                                                                        //Clean ERROR, NEED, Attempts
                                                                        errorSerial<<"MBUS: send failed "<<item->itemArr->name<<"/"<<execObj->name<<endl;
                                                                        execObj->subtype = 0;
                                                                        }

                                 execObj=execObj->next;
                          }     
           }                

// if some polling configured
if (store->pollingRegisters || store->pollingIrs || store->pollingCoils || store->poolingDiscreteIns)
  {
    traceSerial<<F("MBUSD: Poll ")<< item->itemArr->name << endl;
    modbusBusy=1;

    if (!lineInitialized)
    {
    lineInitialized=true;  
    initLine();
    }

    pollModbus(store->pollingRegisters,MODBUS_HOLDING_REG_TYPE);
    pollModbus(store->pollingIrs,MODBUS_INPUT_REG_TYPE);
    pollModbus(store->pollingCoils,MODBUS_COIL_REG_TYPE);  
    pollModbus(store->poolingDiscreteIns ,MODBUS_DISCRETE_REG_TYPE);  
    traceSerial<<F("MBUSD: endPoll ")<< item->itemArr->name << endl;

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

int out_Modbus::sendItemCmd(aJsonObject *templateParamObj, itemCmd cmd)
{
if (templateParamObj)
{
int suffixCode = cmd.getSuffix(); 
//bool isCommand = cmd.isCommand();
if (!suffixCode) return 0;

char *suffixStr =templateParamObj->name;   
// We have find template for suffix or suffixCode
itemCmd  cmdValue =  itemCmd(ST_VOID,CMD_VOID);
long Value = 0;
int8_t    regType = PAR_I16;
aJsonObject * typeObj = aJson.getObjectItem(templateParamObj, "type");
aJsonObject * mapObj = aJson.getObjectItem(templateParamObj, "map");


                    if (typeObj && typeObj->type == aJson_String) regType=str2regSize(typeObj->valuestring);
                    switch(regType) {
                      case PAR_I16:
                      case PAR_U16:
                      case PAR_I32:
                      case PAR_U32:
                      case PAR_U8L:
                      case PAR_U8H: 
                      case PAR_I8H:
                      case PAR_I8L:
                     
                        cmdValue=cmd.doMapping(mapObj);
                        if (!cmdValue.isValue())return 0;
                        Value=cmdValue.getInt();
                      break;
                      case PAR_TENS:
                        cmdValue=cmd.doMapping(mapObj);
                        if (!cmdValue.isValue())return 0;
                        Value=cmdValue.getTens();
                      break;  
                      case PAR_100:  
                        cmdValue=cmd.doMapping(mapObj);
                        if (!cmdValue.isValue())return 0;
                        Value=cmdValue.getTens_raw()*(100/TENS_BASE);
                    }

traceSerial<<F("MBUSD: suffix:")<<suffixStr<< F(" Val: ")<<Value<<endl;
aJsonObject * itemParametersObj = aJson.getArrayItem(item->itemArg, 2);
if (itemParametersObj && itemParametersObj->type ==aJson_Object)
           {
            aJsonObject *execObj =  aJson.getObjectItem(itemParametersObj,suffixStr); 
            if (execObj && ((execObj->type == aJson_Object) || (execObj->type == aJson_Array)))
                          {   

                                      
                                      aJsonObject * markObj = execObj;
                                      if (execObj->type == aJson_Array) markObj = execObj->child;

                                       //Schedule update
                                        execObj->subtype |= MB_NEED_SEND;

                                        aJsonObject *outValue = aJson.getObjectItem(markObj,"@V");                      
                                        if (outValue) // Existant. Preserve original @type
                                            {    
                                            outValue->valueint=Value;
                                            outValue->subtype =regType & 0xF; 
                                            }
                                        else //No container to store value yet 
                                        // If no  @V in config - creating with INT type - normal behavior - no supress in-to-out 
                                            {
                                              debugSerial<<F("Add @V: ")<<execObj->name<<endl;
                                              aJson.addNumberToObject(markObj, "@V", Value);
                                              outValue = aJson.getObjectItem(markObj,"@V");  
                                              if (outValue) outValue->subtype =regType & 0xF; 
                                            }
/* Conflict with pre-fetching
                                        aJsonObject *polledValue = aJson.getObjectItem(markObj,"@S"); 
                                        if (polledValue && outValue->type == aJson_Int) 
                                                                {
                                                                traceSerial<<"MBUS: not Stored "<<Value<<" to @S of "<<item->itemArr->name<<":"<<templateParamObj->name<<endl;  
                                                                polledValue->valueint=Value; //to pevent suppressing to change back to previously polled value if this occurs before next polling
                                                                polledValue->subtype&=~MB_VALUE_OUTDATED;
                                                                }
     */                                                           

                                                
                           }  
           }
return 1;  
}
else return 0;
}

//!Control unified Modbus item  
// Priority of selection sub-items control to:
// 1. if defined standard suffix Code inside cmd
// 2. custom textual subItem
// 3. non-standard numeric  suffix Code equal param id

int out_Modbus::Ctrl(itemCmd cmd,   char* subItem, bool toExecute,bool authorized)
{
if (!store) return -1;

int          suffixCode = cmd.getSuffix();
aJsonObject *templateParamObj = NULL;
int          res = -1;

// trying to find parameter in template with name == subItem (NB!! standard suffixes dint working here)
if (subItem && strlen (subItem) && store && store->parameters) 
      
      {
      templateParamObj = aJson.getObjectItem(store->parameters, subItem);
      res= sendItemCmd(templateParamObj,cmd);
      }
else

// No subitem, trying to find suffix with root item  - (Trying to find template parameter where id == suffixCode)
      {
      if (store && store->parameters) templateParamObj = store->parameters->child;
      bool suffixFinded = false;
        while (templateParamObj)
                {
                  aJsonObject *idObj = aJson.getObjectItem(templateParamObj, "id");
                  if (idObj && idObj->type==aJson_Int && idObj->valueint == suffixCode) 
                                  {
                                    res= sendItemCmd(templateParamObj,cmd);
                                    suffixFinded = true;
                                  }
                  templateParamObj=templateParamObj->next;                       
                }
         if (!suffixFinded) errorSerial<<F("MBUS: No template for ")<<subItem<<F(" or suffix ")<<suffixCode<<endl;                       
      } 
return res;          
}


#endif
