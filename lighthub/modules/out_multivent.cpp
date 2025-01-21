#ifndef MULTIVENT_DISABLE

#include "modules/out_multivent.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "utils.h"


void out_Multivent::getConfig()
{
 gatesObj = NULL;
  if (!item || !item->itemArg || item->itemArg->type != aJson_Object) return;
 gatesObj = item->itemArg;
 //acTemp=(float) item->getExt();
  }

int  out_Multivent::Setup()
{
abstractOut::Setup();    
//getConfig();

//Allocate objects to store persistent data in config tree
if (gatesObj)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             if (i->name && *i->name)
             {
             aJsonObject  * fanObj = aJson.getObjectItem(i, "fan");
             if (!fanObj) {aJson.addNumberToObject(i, "fan", (long int) -1);fanObj = aJson.getObjectItem(i, "fan");}

             aJsonObject  * cmdObj  = aJson.getObjectItem(i, "cmd");
             if (!cmdObj) {aJson.addNumberToObject(i, "cmd", (long int) -1);cmdObj  = aJson.getObjectItem(i, "cmd");}

             aJsonObject * outObj = aJson.getObjectItem(i, "out");
             if (!outObj) {aJson.addNumberToObject(i, "out", (long int) -1);outObj = aJson.getObjectItem(i, "out");}

             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             if (pidObj && pidObj->type == aJson_Array && aJson.getArraySize(pidObj)>=3)
             { 
             aJsonObject * setObj = aJson.getObjectItem(i, "set");
             if (!setObj) {aJson.addNumberToObject(i, "set", (float) 20.1);setObj = aJson.getObjectItem(i, "set");}
                else if (setObj->type != aJson_Float) {setObj->valuefloat = 20.0;setObj->type= aJson_Float;}

             aJsonObject  * valObj = aJson.getObjectItem(i, "val");
             if (!valObj) {aJson.addNumberToObject(i, "val", (float) 20.1);valObj = aJson.getObjectItem(i, "val");}
                else if (valObj->type != aJson_Float) {valObj->valuefloat = 20.0;valObj->type= aJson_Float;}

             aJsonObject  * poObj = aJson.getObjectItem(i, "po");
             if (!poObj) {aJson.addNumberToObject(i, "po", (float) -1.1);poObj = aJson.getObjectItem(i, "po");}
                else if (poObj->type != aJson_Float) {poObj->valuefloat = -2.0;valObj->type= aJson_Float;}   
    
              float kP = 1.0;
              float kI = 0.0;
              float kD = 0.0;
              
              int direction = DIRECT;
              aJsonObject *  param = aJson.getArrayItem(pidObj, 0);
              if (param->type == aJson_Float) kP=param->valuefloat;  
                   else if (param->type == aJson_Int) kP=param->valueint;
              if (kP<0)
                {
                    kP=-kP;
                    direction=REVERSE;
                }       
              param = aJson.getArrayItem(pidObj, 1);
              if (param->type == aJson_Float) kI=param->valuefloat;
                  else if (param->type == aJson_Int) kI=param->valueint; 

              param = aJson.getArrayItem(pidObj, 2);
              if (param->type == aJson_Float) kD=param->valuefloat;
                  else if (param->type == aJson_Int) kD=param->valueint;  

              float dT=5.0;
              if (aJson.getArraySize(pidObj)==4)
              {
              param = aJson.getArrayItem(pidObj, 3);
              if (param->type == aJson_Float) dT=param->valuefloat;
                  else if (param->type == aJson_Int) dT=param->valueint;                  
              }

              debugSerial << "VENT: X:" << (long int) &valObj->valuefloat << "-" << (long int)&poObj->valuefloat <<"="<< (long int)&setObj->valuefloat<<endl;
              pidObj->valueint = (long int) new PID  (&valObj->valuefloat, &poObj->valuefloat, &setObj->valuefloat, kP, kI, kD, direction); 
              debugSerial << "VENT: Y:" << (long int)((PID*) pidObj->valueint)->myInput << "-" << (long int)((PID*) pidObj->valueint)->myOutput <<"="<< (long int)((PID*) pidObj->valueint)->mySetpoint<<endl;

              ((PID*) pidObj->valueint)->SetMode (AUTOMATIC);
              ((PID*) pidObj->valueint)->SetSampleTime(dT*1000.0); 
              debugSerial << F ("VENT: PID P=")<<kP<<" I="<<kI<<" D="<<kD<< endl;

             } 
             } 
             i=i->next; 
          }
      debugSerial << F ("VENT: init")<< endl;
      item->setExt(0);
      setStatus(CST_INITIALIZED);
      return 1;
      }

debugSerial << F ("VENT: config failed")<< endl;
return 0;

}

int  out_Multivent::Stop()
{
debugSerial << F ("VENT: De-Init") << endl;
if (gatesObj)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             if (i->name && *i->name)
             {
             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             if (pidObj && pidObj->valueint)
                        {
                        delete ((PID *) pidObj->valueint);
                        pidObj->valueint = 0;//NULL;
                        }
        
             } 
             i=i->next; 
          }
      }
setStatus(CST_UNKNOWN);
return 1;
}

int out_Multivent::Poll(short cause)
{
  if (cause == POLLING_SLOW && item->getExt() && isTimeOver(item->getExt(),millisNZ(),60000L))
  {
   item->setExt(0);
   item->setCmd((isActive())?CMD_ON:CMD_OFF); // if AC temp unknown - change state to ON or OFF instead HEAT|COOL|FAN
  }

  if (gatesObj)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             if (i->name && *i->name)
             {
             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             if (pidObj && pidObj->valueint)
                        {
                        PID * p = (PID *) pidObj->valueint;  
                        if (p->Compute())
                             { 
                              aJsonObject * outObj = aJson.getObjectItem(i,"po");
                              if (outObj && outObj->type == aJson_Float)
                                {
                                  debugSerial<<F("VENT: ")
                                          <<item->itemArr->name<<"/"<<i->name
                                          <<F(" in:")<<p->GetIn()<<F(" set:")<<p->GetSet()<<F(" out:")<<p->GetOut() 
                                       //   <<F(" in:")<<getFloatFromJson(i,"val")<<(" set:")<<getFloatFromJson(i,"set")<<F(" out:")<<outObj->valuefloat
                                          <<" P:"<<p->GetKp()<<" I:"<<p->GetKi()<<" D:"<<p->GetKd()<<((p->GetDirection())?" Rev ":" Dir ")<<((p->GetMode())?" A":" M");
                                  debugSerial<<endl;
                                  

                                  switch (item->getCmd())
                                  {
                                    case CMD_HEAT:
                                    ((PID *) pidObj->valueint)->SetControllerDirection(DIRECT);
                                    debugSerial<<F("VENT: PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" set DIRECT mode")<<endl;
                               
                                    Ctrl(itemCmd().Percents255(outObj->valuefloat).setSuffix(S_FAN),i->name);
                                    break;
                                    case CMD_COOL:
                                    //case CMD_FAN: // if PIB using for vent 
                                    //case CMD_ON: // AC temp unknown - assuming that PID used for vent
                                    ((PID *) pidObj->valueint)->SetControllerDirection(REVERSE);
                                    debugSerial<<F("VENT: PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" set REVERSE mode")<<endl;
                                    Ctrl(itemCmd().Percents255(outObj->valuefloat).setSuffix(S_FAN),i->name);
                                    break;
                                    // if FAN_ONLY (AC report room temp regularry) - not use internal PID - let be on external control via /fan
                                  }

                                }  
                             }              
                        }
        
             } 
             i=i->next; 
          }
      }
return 1;
};

int out_Multivent::getChanType()
{
   return CH_PWM;
}


int out_Multivent::Ctrl(itemCmd cmd,   char* subItem , bool toExecute, bool authorized)
{

if (cmd.getCmd()==CMD_DISABLE || cmd.getCmd()==CMD_ENABLE) return 0;
int suffixCode = cmd.getSuffix();
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

if (suffixCode == S_VAL && !subItem && cmd.isValue()) 
      {
        //item->setExt((long)cmd.getFloat());
        debugSerial << F("VENT:")<<F("AC air temp: ")<< cmd.getFloat()<<endl;
        item->setExt(millisNZ());
        int mode = CMD_FAN;
        int temp = cmd.getInt();
        if (temp>30) mode = CMD_HEAT;
           else if (temp<17) mode = CMD_COOL;

        if (item->getCmd() != mode)
          {
            item->setCmd(mode);
            pubAction(item->isActive());
          }   

        return 1;
      }
aJsonObject * i = NULL;

if (cmd.isCommand() && cmd.getSuffix()==S_FAN)
      switch (cmd.getCmd())
          {
          case CMD_HIGH:
          cmd.Percents255(255);
          break;

          case CMD_MED:
          cmd.Percents255(128);
          break;

          case CMD_LOW:
          cmd.setPercents(10);
          break;

} //switch cmd


if (gatesObj) i = gatesObj->child; // Pass 1 - calculate summ air value, max value etc

int activeV = 0;
int totalV  = 0;
int maxV=0;
int maxRequestedV=0;
int maxPercent=0;


while (i)
{
  aJsonObject * fanObj=aJson.getObjectItem(i, "fan");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");
  aJsonObject * cascadeObj=aJson.getObjectItem(i, "cas");

  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * pidObj=aJson.getObjectItem(i, "pid");
   if (fanObj && cmdObj && fanObj->type==aJson_Int && cmdObj->type==aJson_Int) 
   {

      int V         = getIntFromJson(i,"V",60);
      int requestedV=0;

      if (subItem && !strcmp (i->name,subItem)) 
              {

              switch (suffixCode)
              {
 
              case S_FAN:
             if (cmd.isValue())   
                                          {
                                          
                                          if (cmd.getInt())
                                                {
                                                  

                                                if (cmdObj->valueint == CMD_OFF)// || cmdObj->valueint == -1) 
                                                    { 
                                                     debugSerial<<"VENT: Turning ON"<<endl;
                                                     cmdObj->valueint = CMD_ON; 
                                                     cmd.Cmd(CMD_ON);
                                                     //if (isNotRetainingStatus()) item->SendStatusImmediate(itemCmd().Cmd(CMD_ON),FLAG_COMMAND,i->name);
                                                        } 
                                                
                                                fanObj->valueint = cmd.getInt();
                                                }
                                           else
                                                {
                                             if (cmdObj->valueint == CMD_ON)//  != CMD_OFF && cmdObj->valueint != -1)  
                                                    { debugSerial<<"VENT: Turning OFF"<<endl; 
                                                      cmdObj->valueint = CMD_OFF; 
                                                      cmd.Cmd(CMD_OFF);
                                                      //if (isNotRetainingStatus()) item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF),FLAG_COMMAND,i->name);
                                                        } 
              
                                                fanObj->valueint = 0;
                                                } 

                                           //fanObj->valueint = cmd.getInt();
                                           if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,FLAG_PARAMETERS|FLAG_COMMAND,i->name);
                                          }
                                          if (!cmd.isCommand()) break; // if have command i FAN suffix - continue processing
             case S_CMD:
              if (cmd.isCommand()) 
                                          {
                                          long sendFlags = 0;
                                          
                                    
                                          switch (cmd.getCmd())
                                          {
                                            case CMD_ON:
                                            cmd.Percents255(fanObj->valueint);
                                            cmd.setSuffix(S_FAN);
                                            sendFlags |= FLAG_COMMAND | FLAG_PARAMETERS;
                                            cmdObj->valueint = cmd.getCmd();
                                            break;
                                            case CMD_OFF:
                                            cmd.Percents255(0);
                                            cmd.setSuffix(S_FAN);
                                            sendFlags |= FLAG_COMMAND | FLAG_PARAMETERS;
                                            cmdObj->valueint = cmd.getCmd();
                                            break;
                                            case CMD_ENABLE:
                                            if (pidObj && pidObj->valueint) ((PID *) pidObj->valueint)->SetMode(AUTOMATIC);
                                            sendFlags |= FLAG_FLAGS;
                                            break;
                                            case CMD_DISABLE:
                                            if (pidObj && pidObj->valueint) ((PID *) pidObj->valueint)->SetMode(MANUAL);
                                            sendFlags |= FLAG_FLAGS;
                                            break;
                                            case CMD_AUTO:
                                            case CMD_COOL:
                                            case CMD_HEAT:
                                            case CMD_FAN:
                                            case CMD_DRY:
                                            sendFlags |= FLAG_COMMAND;
                                            cmdObj->valueint = cmd.getCmd();
                                            break; 
                                            //todo - halt-rest-xon-xoff-low-med-hi                                           
                                          }
                                          if (isNotRetainingStatus() && (cmdObj->valueint == CMD_ON) && (fanObj->valueint<20))
                                                                  {
                                                                    fanObj->valueint=30;
                                                                    cmd.Percents255(30);
                                                                    //if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,FLAG_PARAMETERS,i->name);
                                                                  }

                                          if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,sendFlags,i->name);
                                          }
                break;


              case S_SET:
               if (cmd.isValue())   
                                          {
                                           if (!setObj) {aJson.addNumberToObject(i, "set", (float) cmd.getFloat()); setObj = aJson.getObjectItem(i, "set"); }
                                               else {setObj->valuefloat = cmd.getFloat();setObj->type = aJson_Float;}
                                          //publishTopic(i->name,setObj->valuefloat,"/set");
                                          if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,FLAG_PARAMETERS,i->name);
                                          }              
                break;

              case S_VAL:
               if (cmd.isValue())   
                                          {
                                            aJsonObject  * valObj = aJson.getObjectItem(i, "val");
                                            if (!valObj) {aJson.addNumberToObject(i, "val", (float) cmd.getFloat()); setObj = aJson.getObjectItem(i, "val");}
                                                else {valObj->valuefloat = cmd.getFloat();valObj->type= aJson_Float;}
                                          }
                                          return 1;
                break;  

              default:
                break;
              }  

              if (cascadeObj) executeCommand(cascadeObj,-1,cmd);                           
              }

      if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1) 
                      { 
                      requestedV=V*fanObj->valueint;
                      activeV+=requestedV;

                            if (fanObj->valueint>maxPercent )
                                  {
                                  maxRequestedV=requestedV;
                                  maxV=V;
                                  maxPercent=fanObj->valueint;
                                  }
                      }
      totalV+=V;

   }
  i=i->next;
}

if (!totalV) return 0;

int fanV=activeV/totalV;
debugSerial << F("VENT: Total V:")<<totalV<<F(" active V:")<<activeV/255<< F(" fan%:")<<fanV<< F(" Max req:")<<maxRequestedV/255 <<F(" from ")<<maxV<<F(" m3")<< endl;

//executeCommand(aJson.getObjectItem(gatesObj, ""),-1,itemCmd().Percents255(fanV).Cmd((fanV)?CMD_ON:CMD_OFF));
executeCommand(aJson.getObjectItem(gatesObj, ""),-1,itemCmd().Percents255(fanV).setSuffix(S_FAN));
/*
if (fanV)
  executeCommand(aJson.getObjectItem(gatesObj, ""),-1,itemCmd().Percents255(fanV).Cmd(CMD_ON));
else
  executeCommand(aJson.getObjectItem(gatesObj, ""),-1,itemCmd().Percents255(fanV)); */

//Move gates only if fan is actually on
if (!fanV) return 1;

i=NULL;
if (gatesObj) i = gatesObj->child; //Pass 2: re-distribute airflow

while (i)
{

  int V  = getIntFromJson(i,"V",60);


  aJsonObject * outObj=aJson.getObjectItem(i, "out"); 
  aJsonObject * fanObj=aJson.getObjectItem(i, "fan");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");

  if (outObj && fanObj && cmdObj && outObj->type==aJson_Int && fanObj->type==aJson_Int && cmdObj->type==aJson_Int && V) 
  {
         long int out = 0;
        if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1 && maxRequestedV) 
          {
            int requestedV=V*fanObj->valueint;
            out = (( long)requestedV*255L)/(( long)V)*( long)maxV/( long)maxRequestedV;
            debugSerial<<F("VENT: ")<<i->name<<F(" Req:")<<requestedV/255<<F(" Out:")<<out<<endl;
          } 

         
            if ((out != outObj->valueint))
                {
                  //report out
                  executeCommand(i,-1,itemCmd().Percents255(out)); 
                  outObj->valueint=out;
                }
  }        
  i=i->next;
}

return 1;
}

#endif
