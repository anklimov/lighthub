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
  if (!kPIDObj || kPIDObj->type != aJson_Array)
            {
              errorSerial<<F("Invalid PID param array.")<<endl;
              return false;
            }
   double outMin=0.;  //UNUSED
   double outMax=255.;//UNUSED
   float  dT=5.;
   uint32_t   alarmTO=PERIOD_THERMOSTAT_FAILED;

   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   { case 8: //kP,kI,kD,dT, alarmTO, alarmVal, outMin, outMax
       param = aJson.getArrayItem(kPIDObj, 7);
       if (param->type == aJson_Float) outMax=param->valuefloat;
       else if (param->type == aJson_Int) outMax=param->valueint;

     case 7: //kP,kI,kD,dT alarmTO, alarmVal, outMin
       param = aJson.getArrayItem(kPIDObj, 6);
       if (param->type == aJson_Float) outMin=param->valuefloat;
       else if (param->type == aJson_Int) outMin=param->valueint;    

     case 6: //kP,kI,kD,dT, alarmTO, alarmVal
     case 5: //kP,kI,kD,dT, alarmTO
       param = aJson.getArrayItem(kPIDObj, 4);
       if (param->type == aJson_Float) alarmTO=param->valuefloat;
       else if (param->type == aJson_Int) alarmTO=param->valueint;         

     case 4: //kP,kI,kD,dT
       param = aJson.getArrayItem(kPIDObj, 3);
       if (param->type == aJson_Float) dT=param->valuefloat;
       else if (param->type == aJson_Int) dT=param->valueint; 

     case 3: //kP,kI,kD
       param = aJson.getArrayItem(kPIDObj, 2);
       if (param->type == aJson_Float) kD=param->valuefloat;
       else if (param->type == aJson_Int) kD=param->valueint;

     case 2: //kP,kI
       param = aJson.getArrayItem(kPIDObj, 1);
       if (param->type == aJson_Float) kI=param->valuefloat;
       else if (param->type == aJson_Int) kI=param->valueint;

     case 1: //kP
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
   store->input=0.0;
   store->output=0.0;
   store->alarmTimer=millis();
   store->alarmArmed=true;
   store->alarmTimeout=alarmTO; //in sec
   alarm(true);
   
  if (!store->pid)   
      {store->pid= new PID  (&store->input, &store->output, &store->setpoint, kP, kI, kD, direction);
      if (!store->pid) return false;
      store->pid->SetMode(AUTOMATIC);
      //store->pid->SetOutputLimits(outMin,outMax);
      store->pid->SetSampleTime(dT*1000.0); 
      return true;}
  else errorSerial<<F("PID already initialized")<<endl;    

  return false;
  }


int  out_pid::Setup()
{
abstractOut::Setup();    
if (!store) store= (pidPersistent *)item->setPersistent(new pidPersistent);
if (!store)
              { errorSerial<<F("PID: Out of memory")<<endl;
                return 0;}
store->pid=NULL;
//store->timestamp=millis();
if (getConfig())
    {
        infoSerial<<F("PID config loaded ")<< item->itemArr->name<<endl;
        //item->On(); // Turn ON pid by default
  //      if (item->getCmd()) item->setFlag(SEND_COMMAND);
  //      if (item->itemVal)  item->setFlag(SEND_PARAMETERS);
        store->prevOut = -2.0;
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
debugSerial.println("PID De-Init");
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
      //double prevOut=store->output;  
      //itemCmd st;
      //st.loadItem(item);
      //short cmd = st.getCmd();
      if (item->getCmd() != CMD_OFF)
      { 
      if(store->pid->Compute() )
      {
      int alarmVal;  
      if (store->alarmArmed && (alarmVal=getAlarmVal()>=0)) store->output=alarmVal; 
      debugSerial<<F("PID ")<<item->itemArr->name<<F(" set:")<<store->setpoint<<F(" in:")<<store->input<<(" out:") << store->output <<F(" P:")<<store->pid->GetKp() <<F(" I:")<<store->pid->GetKi() <<F(" D:")<<store->pid->GetKd();
      if (store->alarmArmed) debugSerial << F(" Alarm");
      debugSerial<<endl;
      
      if ((abs(store->output-store->prevOut)>OUTPUT_TRESHOLD) && !store->alarmArmed)
          { 
            aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);
            itemCmd value((float) (store->output));// * (100./255.)));
            value.setSuffix(S_SET);
            executeCommand(oCmd,-1,value);
            store->prevOut=store->output;
          }
      }
      if(!store->alarmArmed && store->alarmTimeout && isTimeOver(store->alarmTimer,millis(),store->alarmTimeout*1000) )
        {
          store->alarmArmed=true;
          alarm(true);
        }
      }  
      }

return 1;//store->pollingInterval;
};

int out_pid::getAlarmVal()
{
  aJsonObject * kPIDObj = aJson.getArrayItem(item->itemArg, 0);
   if (!kPIDObj || kPIDObj->type != aJson_Array)
            {
              errorSerial<<F("Invalid PID param array.")<<endl;
              return -1;
            }

   int outAlarm=0;
   double kP=0.;
   
   bool alarmValDefined = false;
   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   { 
     case 7: //kP,kI,kD, alarmTO, alarmVal, outMin, outMax
     case 6: //kP,kI,kD, alarmTO, alarmVal, outMin
     case 5: //kP,kI,kD, alarmTO, alarmVal
       param = aJson.getArrayItem(kPIDObj, 4);
       alarmValDefined=true;
       if (param->type == aJson_Float) outAlarm=param->valuefloat;
       else if (param->type == aJson_Int) outAlarm=param->valueint;   
       else alarmValDefined=false; 

     case 4: //kP,kI,kD, alarmTO
     case 3: //kP,kI,kD
     case 2: //kP,kI
     case 1: //kP
       param = aJson.getArrayItem(kPIDObj, 0);
       if (param->type == aJson_Float) kP=param->valuefloat;  
       else if (param->type == aJson_Int) kP=param->valueint;
              {
              if (kP<0)
                {
                    if (!alarmValDefined) outAlarm = 0.;
                } 
               else if (!alarmValDefined) outAlarm = .255; 
              }     
   }
return outAlarm;   
}   

void  out_pid::alarm(bool state)
{

if (!item || !item->itemArg) return;
  if (state)
  {
   float outAlarm=getAlarmVal();  
   errorSerial<<item->itemArr->name<<F(" PID alarm. ")<<endl;
   if (outAlarm>=0)        
          {
          errorSerial<<F("Set out to ")<<outAlarm<<endl;
          aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);
          itemCmd value ((float)outAlarm);// * (100./255.)));
                    executeCommand(oCmd,-1,value);
          }         
  }
  else 
  {
  infoSerial<<item->itemArr->name<<F(" PID alarm: closed")<<endl;
  }
}


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

store->alarmTimer=millis();
if (store->alarmArmed)
        {
         store->alarmArmed=false; 
         alarm(false);  
         store->prevOut=-2.0;  
        }
return 1;
//break;

case S_NOTFOUND:
case S_SET:
//case S_ESET:
// Setpoint for PID
if (!cmd.isValue()) return 0;
store->setpoint=cmd.getFloat();  
debugSerial<<F("Setpoint:")<<store->setpoint<<endl;

{
 aJsonObject * itemCascadeObj = aJson.getArrayItem(item->itemArg, 2);
if (itemCascadeObj) executeCommand(itemCascadeObj,-1,cmd); 
}

//cmd.saveItem(item);
//item->SendStatus(SEND_PARAMETERS);
return 1;
//break;

case S_CMD:
      {
      aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);   
      short command = cmd.getCmd();  
      itemCmd value(ST_VOID,command);
      value.setSuffix(S_CMD);       
      switch (command)
          {
          case CMD_OFF:
            //value.Percents255(0);

          case CMD_ON:
          case CMD_HEAT:
          case CMD_COOL:
          case CMD_AUTO:
          case CMD_FAN:
          case CMD_DRY:

             executeCommand(oCmd,-1,value); 
          return 1;
          
/*
          case CMD_OFF:
          {  
            aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);
            itemCmd value((float) 0.);// * (100./255.)));
            value.setSuffix(S_SET);
            executeCommand(oCmd,-1,value);
            return 1;
          } */

            default:
            debugSerial<<F("Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd
      }
    default:
  debugSerial<<F("Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

return 0;
}

#endif
