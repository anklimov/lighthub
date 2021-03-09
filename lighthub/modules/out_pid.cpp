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
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Array) || aJson.getArraySize(item->itemArg)<1)
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
   double outMin=0.;
   double outMax=100.;
   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   {   case 5:
       param = aJson.getArrayItem(kPIDObj, 4);
       if (param->type == aJson_Float) outMax=param->valuefloat;
       else if (param->type == aJson_Int) outMax=param->valueint;
     case 4:
       param = aJson.getArrayItem(kPIDObj, 3);
       if (param->type == aJson_Float) outMin=param->valuefloat;
       else if (param->type == aJson_Int) outMin=param->valueint;
     case 3:
       param = aJson.getArrayItem(kPIDObj, 2);
       if (param->type == aJson_Float) kD=param->valuefloat;
       else if (param->type == aJson_Int) kD=param->valueint;
     case 2:
       param = aJson.getArrayItem(kPIDObj, 1);
       if (param->type == aJson_Float) kI=param->valuefloat;
       else if (param->type == aJson_Int) kI=param->valueint;
     case 1:
       param = aJson.getArrayItem(kPIDObj, 0);
       if (param->type == aJson_Float) kP=param->valuefloat;  
       else if (param->type == aJson_Int) kP=param->valueint;
              if (kP<0)
                {
                    kP=-kP;
                    direction=REVERSE;
                }      
   }

   switch (item->itemVal->type)
   {
    case aJson_Int: 
    store->setpoint = item->itemVal->valueint;
    break;
    case aJson_Float:
    store->setpoint = item->itemVal->valuefloat;
    break;
    default:
    store->setpoint = 20.;
   }

   
  if (!store->pid)   
  
      {store->pid= new PID  (&store->input, &store->output, &store->setpoint, kP, kI, kD, direction);
      if (!store->pid) return false;
      store->pid->SetMode(AUTOMATIC);
      store->pid->SetOutputLimits(outMin,outMax);
     
      return true;}
  else errorSerial<<F("PID already initialized")<<endl;    

  return false;
  }


int  out_pid::Setup()
{
if (!store) store= (pidPersistent *)item->setPersistent(new pidPersistent);
if (!store)
              { errorSerial<<F("PID: Out of memory")<<endl;
                return 0;}
store->pid=NULL;
//store->timestamp=millis();
if (getConfig())
    {
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
Serial.println("PID De-Init");
if (store) delete (store->pid);
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
return (item->getCmd()!=CMD_OFF);
}


int out_pid::Poll(short cause)
{
if (store && store->pid && (Status() == CST_INITIALIZED) && item && (item->getCmd()!=CMD_OFF))   
      {
      double prevOut=store->output;  
      store->pid->Compute();
      if (abs(store->output-prevOut)>OUTPUT_TRESHOLD)
          { 
            aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);
            itemCmd value((float) store->output);
            executeCommand(oCmd,-1,value);
          }

      }

return 1;//store->pollingInterval;
};


int out_pid::getChanType()
{
   return CH_THERMO;
}


//!Control unified PID controller item  
// /set suffix - to setup setpoint
// /val suffix - to put value into controller
// accept ON and OFF commands

int out_pid::Ctrl(itemCmd cmd,   char* subItem, bool toExecute)
{
if (!store || !store->pid || (Status() != CST_INITIALIZED)) return 0;    


int suffixCode = cmd.getSuffix();

if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

switch(suffixCode)
{
case S_VAL:
// Input value for PID
if (!cmd.isValue()) return 0;
store->input=cmd.getFloat();
debugSerial<<F("Input value:")<<store->input<<endl;
return 1;
//break;

case S_NOTFOUND:
case S_SET:
// Setpoint for PID
if (!cmd.isValue()) return 0;
store->setpoint=cmd.getFloat();  
debugSerial<<F("Setpoint:")<<store->setpoint<<endl;
//cmd.saveItem(item);
//item->SendStatus(SEND_PARAMETERS);
return 1;
//break;

case S_CMD:
      switch (cmd.getCmd())
          {
          case CMD_ON:
          case CMD_OFF:
            item->setCmd(cmd.getCmd());
            item->SendStatus(SEND_COMMAND);
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
