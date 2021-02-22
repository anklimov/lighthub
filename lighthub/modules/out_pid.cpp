#ifndef PID_DISABLE

#include "modules/out_pid.h"
#include "Arduino.h"
#include "options.h"
#include "utils.h"
#include "Streaming.h"
#include "item.h"
#include "main.h"

bool out_pid::getConfig()
{
  double kP=0.0;
  double kI=0.0;
  double kD=0.0;
  int direction = DIRECT;

  // Retrieve and store 
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Array) || aJson.getArraySize(item->itemArg)<2)
  {
    errorSerial<<F("PID: config failed:")<<(bool)store<<F(",")<<(bool)item<<F(",")<<(bool)item->itemArg<<F(",")<<(item->itemArg->type != aJson_Array)<<F(",")<< (aJson.getArraySize(item->itemArg)<2)<<endl;
    return false;
  }

  aJsonObject * kPIDObj = aJson.getArrayItem(item->itemArg, 0);
  if (kPIDObj->type != aJson_Array)
            {
              errorSerial<<F("Invalid PID param array.")<<endl;
              return false;
            }
   
   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   {
     case 3:
       param = aJson.getArrayItem(kPIDObj, 2);
       if (param->type == aJson_Float) kD=param->valuefloat;
     case 2:
       param = aJson.getArrayItem(kPIDObj, 1);
       if (param->type == aJson_Float) kI=param->valuefloat;
     case 1:
       param = aJson.getArrayItem(kPIDObj, 0);
       if (param->type == aJson_Float) kP=param->valuefloat;  
              if (kP<0)
                {
                    kP=-kP;
                    direction=REVERSE;
                }      
   }

   switch (item->itemVal->type)
   {
    case aJson_Int: item->itemVal->valuefloat = item->itemVal->valueint;
    break;
    case aJson_Float:
    break;
    default:

   }

   item->itemVal->type = aJson_Float;
   
  if (!store->pid)   store->pid= new PID  (&store->input, &store->output, &item->itemVal->valuefloat, kP, kI, kD, direction);

  return true;
  //store->addr=item->getArg(0);
  }


int  out_pid::Setup()
{
if (!store) store= (pidPersistent *)item->setPersistent(new pidPersistent);
if (!store)
              { errorSerial<<F("PID: Out of memory")<<endl;
                return 0;}

//store->timestamp=millis();
if (getConfig())
    {
        //item->clearFlag(ACTION_NEEDED);
        //item->clearFlag(ACTION_IN_PROCESS);
        infoSerial<<F("PID config loaded ")<< item->itemArr->name<<endl;
        store->driverStatus = CST_INITIALIZED;
        return 1;
      }
else
 {  errorSerial<<F("PID config error")<<endl;
    store->driverStatus = CST_FAILED;
    return 0;
  }

}

int  out_pid::Stop()
{
Serial.println("Modbus De-Init");
if (store) delete (store->pid());
delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
}

int  out_pid::Status()
{
if (store)
    return store->driverStatus;
return CST_UNKNOWN;
}

int out_pid::isActive()
{
return item->getVal();
}


int out_pid::Poll(short cause)
{
if ((Status() == CST_INITIALIZED) && isTimeOver(store->timestamp,millis(),store->pollingInterval))
  {
    
  store->timestamp=millis();
  debugSerial<<F("endPoll ")<< item->itemArr->name << endl;

  }

return store->pollingInterval;
};

int out_pid::getChanType()
{
   return CH_MODBUS;
}


//!Control unified Modbus item  
// Priority of selection sub-items control to:
// 1. if defined standard suffix Code inside cmd
// 2. custom textual subItem
// 3. non-standard numeric  suffix Code equal param id

int out_pid::Ctrl(itemCmd cmd,   char* subItem, bool toExecute)
{
//int chActive = item->isActive();
//bool toExecute = (chActive>0);
//itemCmd st(ST_UINT32,CMD_VOID);
int suffixCode = cmd.getSuffix();
aJsonObject *templateParamObj = NULL;
short mappedCmdVal = 0;

// trying to find parameter in template with name == subItem (NB!! standard suffixes dint working here)
if (subItem && strlen (subItem)) templateParamObj = aJson.getObjectItem(store->parameters, subItem);

if (!templateParamObj)
{
  // Trying to find template parameter where id == suffixCode
 templateParamObj = store->parameters->child;

  while (templateParamObj)
          {
            //aJsonObject *regObj = aJson.getObjectItem(paramObj, "reg");
            aJsonObject *idObj = aJson.getObjectItem(templateParamObj, "id");
            if (idObj->type==aJson_Int && idObj->valueint == suffixCode) break;

            aJsonObject *mapObj = aJson.getObjectItem(templateParamObj, "mapcmd");
            if (mapObj && (mappedCmdVal = cmd.doReverseMappingCmd(mapObj))) break;

            templateParamObj=templateParamObj->next;                       
          }
}


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
