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
  bool limits = false;

  // Retrieve and store 
  if (!store || !item || !item->itemArg || (item->itemArg->type != aJson_Array) || aJson.getArraySize(item->itemArg)<1)
  {
    errorSerial<<F("PID: config failed:")<<(bool)store<<F(",")<<(bool)item<<F(",")<<(bool)item->itemArg<<F(",")<<(item->itemArg->type != aJson_Array)<<F(",")<< (aJson.getArraySize(item->itemArg)<2)<<endl;
    return false;
  }

  aJsonObject * kPIDObj = aJson.getArrayItem(item->itemArg, 0);
  if (!kPIDObj || kPIDObj->type != aJson_Array)
            {
              errorSerial<<F("PID: Invalid param array.")<<endl;
              return false;
            }
   double outMin=0.;  
   double outMax=255.;
   float  dT=5.;
   uint32_t   alarmTO=PERIOD_THERMOSTAT_FAILED;

   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   { case 8: //kP,kI,kD,dT, alarmTO, alarmVal, outMin, outMax
       param = aJson.getArrayItem(kPIDObj, 7);
       if (param->type == aJson_Float) {outMax=param->valuefloat;limits=true;}
       else if (param->type == aJson_Int) {outMax=param->valueint;limits=true;}

     case 7: //kP,kI,kD,dT alarmTO, alarmVal, outMin
       param = aJson.getArrayItem(kPIDObj, 6);
       if (param->type == aJson_Float) {outMin=param->valuefloat;limits=true;}
       else if (param->type == aJson_Int) {outMin=param->valueint;limits=true;}    

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
      if (limits) store->pid->SetOutputLimits(outMin,outMax);
      store->pid->SetSampleTime(dT*1000.0); 
      return true;}
  else errorSerial<<F("PID: already initialized")<<endl;    

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
        infoSerial<<F("PID: config loaded ")<< item->itemArr->name<<endl;
        //item->On(); // Turn ON pid by default
  //      if (item->getCmd()) item->setFlag(FLAG_COMMAND);
  //      if (item->itemVal)  item->setFlag(FLAG_PARAMETERS);
        store->prevOut = -2.0;
        setStatus(CST_INITIALIZED);
        return 1;
      }
else
 {  errorSerial<<F("PID: config error")<<endl;
    setStatus(CST_FAILED);
    return 0;
  }

}

int  out_pid::Stop()
{
debugSerial.print(F("PID: De-Init "));
debugSerial.println(item->itemArr->name);
if (store) delete (store->pid);
delete store;
item->setPersistent(NULL);
store = NULL;
return 1;
}


int out_pid::isActive()
{
return (item->getCmd()!=CMD_OFF);
}


int out_pid::Poll(short cause)
{
if (cause==POLLING_SLOW) return 0;
if (store && store->pid && (Status() == CST_INITIALIZED) && item && (item->getCmd()!=CMD_OFF))   
    {
      if (item->getCmd() != CMD_OFF && ! item->getFlag(FLAG_DISABLED))
      { 
      if(store->pid->Compute() )
      {
      float alarmVal;  
      if (store->alarmArmed && ((alarmVal=getAlarmVal())>=0.)) store->output=alarmVal; 

      debugSerial<<F("PID: ")<<item->itemArr->name<<F(" set:")<<store->setpoint<<F(" in:")<<store->input<<(" out:") << store->output <<F(" P:")<<store->pid->GetKp() <<F(" I:")<<store->pid->GetKi() <<F(" D:")<<store->pid->GetKd();
      //if (item->getFlag(FLAG_DISABLED)) debugSerial << F(" <DIS>");
      if (store->alarmArmed) debugSerial << F(" <ALM>"); 
      debugSerial<<endl;
      
      if (( (NOT_FILTER_PID_OUT || (abs(store->output-store->prevOut)>OUTPUT_TRESHOLD))  || (item->getFlag(FLAG_ACTION_NEEDED))) && !store->alarmArmed)
          { 
            aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);

            if (((store->prevOut == 0.) && (store->output>0)) || item->getFlag(FLAG_ACTION_NEEDED))
//            if ((item->getFlag(FLAG_ACTION_NEEDED)) && (store->output>0.))
              {
                executeCommand(oCmd,-1,itemCmd().Cmd(CMD_ON));
//                item->clearFlag(FLAG_ACTION_NEEDED);
              }

            item->clearFlag(FLAG_ACTION_NEEDED); 

            itemCmd value((float) (store->output));
            //value.setSuffix(S_SET);
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

float out_pid::getAlarmVal()
{
  aJsonObject * kPIDObj = aJson.getArrayItem(item->itemArg, 0);
   if (!kPIDObj || kPIDObj->type != aJson_Array)
            {
              errorSerial<<F("PID: Invalid param array.")<<endl;
              return -1.;
            }

   float outAlarm=0.;
   double kP=0.;
   
   bool alarmValDefined = false;
   aJsonObject * param;         
   switch (aJson.getArraySize(kPIDObj))
   { 
     case 8: //kP,kI,kD,dT, alarmTO, alarmVal, outMin, outMax
     case 7: //kP,kI,kD,dT, alarmTO, alarmVal, outMin
     case 6: //kP,kI,kD,dT,alarmTO, alarmVal
       param = aJson.getArrayItem(kPIDObj, 5);
       alarmValDefined=true;
       if (param->type == aJson_Float) outAlarm=param->valuefloat;
       else if (param->type == aJson_Int) outAlarm=param->valueint;   
       else alarmValDefined=false; 

     case 5: //kP,kI,kD,dT, alarmTO
     case 4: //kP,kI,kD,dT
     case 3: //kP,kI,kD
     case 2: //kP,kI
     case 1: //kP
       param = aJson.getArrayItem(kPIDObj, 0);
       if (param->type == aJson_Float) kP=param->valuefloat;  
       else if (param->type == aJson_Int) kP=param->valueint;
              {
              if (kP<0.)
                {
                    if (!alarmValDefined) outAlarm = 0.;
                } 
               else if (!alarmValDefined) outAlarm = 255.; 
              }     
   }
// debugSerial<<F("Alarm value: ")<<outAlarm<< " ";   
return outAlarm;   
}   

void  out_pid::alarm(bool state)
{

if (!item || !item->itemArg) return;
  if (state)
  {
   float outAlarm=getAlarmVal();  
   errorSerial<<item->itemArr->name<<F(" PID alarm. ")<<endl;
   if (outAlarm>=0.)        
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

int out_pid::Ctrl(itemCmd cmd,   char* subItem, bool toExecute, bool authorized)
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
debugSerial<<F("PID: ")<< item->itemArr->name <<F(" Input value:")<<store->input<<endl;

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

// Setpoint for PID
if (!cmd.isValue()) return 0;
store->setpoint=cmd.getFloat();  
debugSerial<<F("PID: ")<< item->itemArr->name <<F(" Setpoint:")<<store->setpoint<<endl;

{
 aJsonObject * itemCascadeObj = aJson.getArrayItem(item->itemArg, 2);
if (itemCascadeObj) executeCommand(itemCascadeObj,-1,cmd); 
}

//cmd.saveItem(item);
//item->SendStatus(FLAG_PARAMETERS);
return 1;
//break;

case S_CMD:
case S_CTRL:
      {
      aJsonObject * oCmd = aJson.getArrayItem(item->itemArg, 1);   
      short command = cmd.getCmd();  
      itemCmd value(ST_VOID,command);
      value.setSuffix(S_CMD);       
      switch (command)
          {
          case CMD_OFF:
             if (isNotRetainingStatus()) executeCommand(oCmd,-1,itemCmd().Cmd(CMD_DISABLE)); // Not actually disable, just inform depended systems, that no autoreg now (for pannels indication)
          executeCommand(oCmd,-1,value); 
          item->SendStatus(FLAG_FLAGS);
          return 1;  
          case CMD_ON:
          case CMD_HEAT:
          case CMD_COOL:
          case CMD_AUTO:
          case CMD_FAN:
          case CMD_DRY:
          executeCommand(oCmd,-1,itemCmd().Cmd((item->getFlag(FLAG_DISABLED))?CMD_DISABLE:CMD_ENABLE));
          executeCommand(oCmd,-1,value); 
          item->SendStatus(FLAG_FLAGS);
          return 1;

          case CMD_ENABLE:
          if (isNotRetainingStatus()) 
                {
                  item->setCmd(CMD_ON);
                  item->SendStatus(FLAG_COMMAND);
                }
          item->setFlag(FLAG_ACTION_NEEDED);
          executeCommand(oCmd,-1,value);
          if (isActive()) executeCommand(oCmd,-1,itemCmd().Cmd((CMD_ON)));   
       

          store->prevOut=-2.0;   
          return 1;

          case CMD_DISABLE:
          executeCommand(oCmd,-1,value);
          if (!isActive()) executeCommand(oCmd,-1,itemCmd().Cmd((CMD_OFF))); 
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
            debugSerial<<F("PID: Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd
      }
    default:
  debugSerial<<F("PID: Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

return 0;
}

#endif
