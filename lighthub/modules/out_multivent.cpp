#ifndef MULTIVENT_DISABLE

#include "modules/out_multivent.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "utils.h"

void convert2float(aJsonObject * o)
{
  if (!o) return;  
  switch (o->type)
  {
    case aJson_Int:
    o->valuefloat = o->valueint;
    o->type = aJson_Float;
    break;
  }
}

void out_Multivent::getConfig()
{
 gatesObj = NULL;
 acObj = NULL;
  if (!item || !item->itemArg || item->itemArg->type != aJson_Object) return;
 gatesObj = item->itemArg;
   if (gatesObj) acObj = aJson.getObjectItem(gatesObj, "");
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
             getCreateObject(i,"fan",-1L);
             getCreateObject(i,"cmd",(long) CMD_OFF);
             getCreateObject(i,"out",-1L);

             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             if (pidObj && pidObj->type == aJson_Array && aJson.getArraySize(pidObj)>=3)
             { 
              aJsonObject  * setObj = getCreateObject(i,"set",(float) 20.0);   
              convert2float(setObj);
              aJsonObject  * valObj = getCreateObject(i,"val",(float) 20.0);      
              convert2float(valObj);
              aJsonObject  * poObj = getCreateObject(i,"po", (float) -2.0);    
              convert2float(poObj);
              
              int direction = DIRECT;
              float kP=getFloatFromJson(pidObj,0,1.0);     
              if (kP<0)
                {
                    kP=-kP;
                    direction=REVERSE;
                }       
              float kI=getFloatFromJson(pidObj,1);     
              float kD=getFloatFromJson(pidObj,2); 
              float dT=getFloatFromJson(pidObj,3,5.0); 

              pidObj->valueint = (long int) new PID  (&valObj->valuefloat, &poObj->valuefloat, &setObj->valuefloat, kP, kI, kD, direction); 

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

int out_Multivent::isActive() 
{
  debugSerial<<"VENT:active: ";
   if (gatesObj)
      {
/*
      // metrics, collected from AC  
      aJsonObject * a = aJson.getObjectItem(gatesObj, "");
      if (!a) return 0;
      float acTemp   = getFloatFromJson(a,"val",NAN);               
      int actualCmd  = getIntFromJson  (a,"mode");
      int actualMode = CMD_FAN;
      if (acTemp>30.0) actualMode = CMD_HEAT;
          else if (acTemp<15.0) actualMode = CMD_COOL;
*/
      aJsonObject * i = gatesObj->child;  
      while (i)
          {
             if (i->name && *i->name)
             {
             int   cmd = getIntFromJson(i,"cmd");
             switch (cmd)
                    {
                    case CMD_ON:  
                    case CMD_HEATCOOL:
                    case CMD_FAN:
                    case CMD_AUTO: 
                    case CMD_COOL:
                    case CMD_HEAT:   
                    case CMD_DRY:                  
                    //case CMD_OFF:
                    return 1;  
                    break;

                    }    
                  }    
       i=i->next; 
      }//while 
    } // if gatesObj
   return 0;             
}

int out_Multivent::Poll(short cause)
{
  
  if (!acObj || !gatesObj) return 0;

  if (cause == POLLING_SLOW && item->getExt() && isTimeOver(item->getExt(),millisNZ(),60000L))
  {
   item->setExt(0);
   aJsonObject * a = aJson.getObjectItem(acObj,"val");
   if (a) a->type = aJson_NULL; //invalidate in 60 sec after measure
  }

      // metrics, collected from AC  
      float acTemp   = getFloatFromJson(acObj,"val",NAN);               
      int actualCmd  = getIntFromJson  (acObj,"mode");

      // global params
      int boostTreshold  = getIntFromJson  (acObj,"boost");
      int actualMode = CMD_FAN;
      if (acTemp>30.0) actualMode = CMD_HEAT;
          else if (acTemp<15.0) actualMode = CMD_COOL;


      aJsonObject * i = gatesObj->child;  
      int balance = 0;    
      bool ventRequested = false; //At least 1 ch requested FAN mode 
      bool autoRequested = false; //At least 1 ch requested AUTO mode 
      while (i)
          {
             if (i->name && *i->name)
             {
             int   cmd = getIntFromJson(i,"cmd");
             float set = getIntFromJson(i,"set");
             float val = getIntFromJson(i,"val");  

             int execCmd = 0;      
             switch (cmd)
                    {
                    case CMD_HEATCOOL:
                    {
                    if (set>val) execCmd = CMD_HEAT;
                    if (set<val) execCmd = CMD_COOL;
                    }
                    break;
                    case CMD_FAN:
                    ventRequested = true;
                    execCmd = cmd;
                    break;
                    case CMD_AUTO: 
                    autoRequested = true;
                    execCmd = cmd;
                    break;
                    case CMD_COOL:
                    case CMD_HEAT:                     
                    case CMD_OFF:
                    //setValToJson(i,"@C",cmd);
                    execCmd = cmd;
                    break;

                    }   
             bool passiveMode =   getIntFromJson(i,"@pasv",0);       

             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             if (pidObj && pidObj->valueint)
                        {
                        PID * p = (PID *) pidObj->valueint;  
                        if (p->Compute())
                             { 
                              aJsonObject * poObj = aJson.getObjectItem(i,"po");
                              if (poObj && poObj->type == aJson_Float)
                                {
                                  debugSerial<<F("VENT: ")
                                          <<item->itemArr->name<<"/"<<i->name
                                          <<F(" in:")<<p->GetIn()<<F(" set:")<<p->GetSet()<<F(" out:")<<p->GetOut() 
                                          <<" P:"<<p->GetKp()<<" I:"<<p->GetKi()<<" D:"<<p->GetKd()<<((p->GetDirection())?" Rev ":" Dir ")<<((p->GetMode())?"A":"M");
                                  debugSerial<<endl;
                                  
                                  if (passiveMode || execCmd == CMD_AUTO)
                                  {
                                    switch (actualMode)
                                          {
                                            case CMD_HEAT:
                                              ((PID *) pidObj->valueint)->SetControllerDirection(DIRECT);
                                              debugSerial<<F("VENT: HEAT PASS PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" out:")<< poObj->valuefloat <<F(" set DIRECT mode")<<endl;
                                               if (actualCmd!=CMD_OFF) fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true);
                                               if (poObj->valuefloat == 0.0 && autoRequested) {autoRequested = false; debugSerial<<"Vent: no more heat needed for "<<i->name<<endl;}
                                            break;
                                            case CMD_COOL:
                                              ((PID *) pidObj->valueint)->SetControllerDirection(REVERSE);
                                              debugSerial<<F("VENT: COOL PASS PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" out:")<< poObj->valuefloat << F(" set REVERSE mode")<<endl;
                                               if (actualCmd!=CMD_OFF) fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true);
                                               if (poObj->valuefloat == 0.0 && autoRequested) {autoRequested = false; debugSerial<<"Vent: no more cool needed for "<<i->name<<endl;}
                                            break;
                                            //case CMD_FAN: //no more hot or cold air
                                            //if (autoRequested) debugSerial<<"VENT: Cancel automode request"<<endl; //
                                            //autoRequested = false;
                                            //todo - delete auto request if AC is idle - stuck and not move to vent job
                                                 
                                          }  
                                  }

                                  else switch (execCmd)
                                  {
                                    case CMD_HEAT:
                                    ((PID *) pidObj->valueint)->SetControllerDirection(DIRECT);
                                    debugSerial<<F("VENT: PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" set DIRECT mode")<<endl;
                               
                                     if (actualCmd==CMD_HEAT) Ctrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name);
                                     //else?

                                    balance+=poObj->valuefloat;
                                    break;
                                    case CMD_COOL:
                                    //case CMD_FAN: // if PIB using for vent 
                                    //case CMD_ON: // AC temp unknown - assuming that PID used for vent
                                    ((PID *) pidObj->valueint)->SetControllerDirection(REVERSE);
                                    debugSerial<<F("VENT: PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" set REVERSE mode")<<endl;
                                    if (actualCmd==CMD_COOL) Ctrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name);
                                    //else ?
                                    balance-=poObj->valuefloat;
                                    break;
                                    // if FAN_ONLY (AC report room temp regularry) - not use internal PID - let be on external control via /fan
                                    case CMD_FAN:
                                    // vent requested but air temp hot or cold 
                                    if (actualCmd == CMD_HEAT || actualCmd == CMD_COOL)
                                                {
                                                Ctrl(itemCmd().Percents255(0).setSuffix(S_FAN),i->name);
                                                Ctrl(itemCmd().Cmd(CMD_FREEZE),i->name);
                                                }
                                    else  Ctrl(itemCmd().Cmd(CMD_UNFREEZE),i->name);           
                                  }

                                }  
                             }              
                        }
        
             } 
             i=i->next; 
          }//while
          if (balance) debugSerial<<F("VENT: Chan balance=")<<balance<<F(" treshold:")<<boostTreshold<<endl;

          if (balance>boostTreshold) sendACcmd (CMD_HEAT);
             else  if (-balance>boostTreshold) sendACcmd (CMD_COOL); 
                else if (autoRequested) sendACcmd(CMD_AUTO);
                     else if (ventRequested) sendACcmd(CMD_FAN);
                  // else sendACcmd (CMD_OFF);
return 1;
};

int out_Multivent::sendACcmd (int cmd)
{
   if (!acObj) return 0;
   int lastCmd = getIntFromJson(acObj,"@lastCmd");
   int acCmd = getIntFromJson(acObj,"mode");
   if (lastCmd && (acCmd != lastCmd)) {
                          //debugSerial<<"VENT: AC MODE changed manually to "<<item->getCmd()<<endl; 
                          return 0;}
   if (cmd == lastCmd) {
                          //debugSerial<<"VENT: AC MODE already same"<<endl; 
                          return 0;}
   executeCommand(acObj,-1,itemCmd().Cmd(cmd).setSuffix(S_CMD));
   setValToJson(acObj,"@lastCmd",cmd);
   return 1;
}

int out_Multivent::getChanType()
{
   return CH_THERMO; /////PWM
}



void SubmitParameters(aJsonObject * callbackObj, const char * name, itemCmd value, bool doMapping){
     if (callbackObj && callbackObj->type == aJson_Object)
        {
          aJsonObject * execObj = aJson.getObjectItem(callbackObj,name);
          
          if (execObj) 
                    {
                    aJsonObject * mapObj = NULL;
                    if (doMapping) mapObj = aJson.getObjectItem(execObj, "map");  
                    executeCommand(execObj,-1,value.doReverseMapping(mapObj));  
                    } 
        }
    }

    
void out_Multivent::setPassiveMode(aJsonObject* zone, bool mode)
{
  bool passiveMode = getIntFromJson(zone,"@pasv",0);   

  if (passiveMode != mode)
  {
                  aJsonObject * cascadeObj=aJson.getObjectItem(zone, "cas");
                  
                  //aJsonObject * cmdObj=aJson.getObjectItem(zone, "cmd");
                  //Set up passive mode
                  debugSerial<<F("VENT: passive mode: ")<<mode<<endl;
                  setValToJson(zone,"@pasv",mode); 

                  if (mode)
                  {
                  if (isNotRetainingStatus()) 
                        item->SendStatusImmediate(itemCmd().Cmd(CMD_AUTO).setSuffix(S_FAN),FLAG_COMMAND,zone->name); //Send /fan->AUTO
                        
                  SubmitParameters(cascadeObj,"fan",itemCmd().Cmd(CMD_AUTO).setSuffix(S_FAN),false);
                  }
                  else 
                  {
                  aJsonObject * fanObj=aJson.getObjectItem(zone, "fan");  
                  if (!fanObj || fanObj->type!=aJson_Int) return;
                  if (isNotRetainingStatus()) 
                        item->SendStatusImmediate(itemCmd().Percents255(fanObj->valueint).setSuffix(S_FAN),FLAG_PARAMETERS,zone->name); //Send /fan->#
                        
                  SubmitParameters(cascadeObj,"fan",itemCmd().Percents255(fanObj->valueint).setSuffix(S_FAN),true);
                  }
  }
}




uint32_t out_Multivent::getFlag   (aJsonObject* zone, uint32_t flag)
{

  if (zone && (zone->type == aJson_Object))
  {
      return (uint32_t) zone->valueint & flag & FLAG_MASK;
    }
return 0;
}

void out_Multivent::setFlag   (aJsonObject* zone, uint32_t flag)
{
  if (zone && (zone->type == aJson_Object))
  {   
      zone->valueint |= flag & FLAG_MASK;   
    }

}

void out_Multivent::clearFlag (aJsonObject* zone, uint32_t flag)
{
  if (zone && (zone->type == aJson_Object))
  {   
      zone->valueint  &= CMD_MASK  | ~(flag & FLAG_MASK);    
    }
}



int out_Multivent::Ctrl(itemCmd cmd,   char* subItem , bool toExecute, bool authorized)
{
return fanCtrl(cmd,subItem, toExecute, false);
}

int out_Multivent::fanCtrl(itemCmd cmd,   char* subItem , bool toExecute, bool force)
{
if (!gatesObj || !acObj) return 0;
if (cmd.getCmd()==CMD_DISABLE || cmd.getCmd()==CMD_ENABLE) return 0;
int suffixCode = cmd.getSuffix();
debugSerial << " VENT: CTRL " << subItem << " "; cmd.debugOut();

if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

bool turnbyfan = getIntFromJson(acObj,"turnbyfan",0);   

if (!subItem)  // feedback from shared AC
{
switch (suffixCode)
    {
    case S_VAL:
            if (cmd.isValue())
            {
            debugSerial << F("VENT:")<<F("AC air temp: ")<< cmd.getFloat()<<endl;
            item->setExt(millisNZ()); //setup validity interval
            setValToJson(acObj,"val",cmd.getFloat());
            } 
    return 1;

    case S_FAN:
    return 1;

    case S_SET:
    return 1;
    
    case S_MODE:
            debugSerial << F("VENT:")<<F("AC mode: ")<< cmd.getCmd()<<endl;
            setValToJson(acObj,"mode",cmd.getCmd()); 
    return 1;   

    case S_CMD:
            debugSerial<<"VENT: Todo - handle cmd/HALT. cmd="<<cmd.getCmd()<<endl; 
    return 1;

    case S_TEMP:
            debugSerial << F("VENT:")<<F("AC air roomtemp: ")<< cmd.getFloat()<<endl;
            setValToJson(acObj,"roomtemp",cmd.getFloat());
    return 1;
    } 
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
          cmd.Percents255(10);
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

  //aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * pidObj=aJson.getObjectItem(i, "pid");
   if (fanObj && cmdObj && fanObj->type==aJson_Int && cmdObj->type==aJson_Int) 
   {

      int V            = getIntFromJson(i,"V",60);
      bool passiveMode = getIntFromJson(i,"@pasv",0);    
      int requestedV   = 0;

      if (subItem && !strcmp (i->name,subItem)) 
              {
              long sendFlags = 0;  
              switch (suffixCode)
              {
 
              case S_FAN:
               if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -1;}
               if (cmd.isValue())   
                                          {                                     
                                          if (cmd.getInt())
                                                {
                                                if (cmdObj->valueint == CMD_OFF && turnbyfan)
                                                    { 
                                                     cmd.Cmd(CMD_ON);
                                                     debugSerial<<"VENT: generating ON by fan"<<endl;
                                                        }                                              
                                                //fanObj->valueint = cmd.getInt();
                                                //sendFlags |= FLAG_PARAMETERS;
                                                }
                                           else
                                                {
                                             if (cmdObj->valueint != CMD_OFF && turnbyfan)
                                                    { 
                                                      cmd.Cmd(CMD_OFF);
                                                      debugSerial<<"VENT: Turning OFF  by fan"<<endl;                                                      
                                                      //setValToJson(i,"@precmd",cmdObj->valueint);   
                                                      //cmdObj->valueint = CMD_OFF; 
                                                      //sendFlags |= FLAG_COMMAND;
                                                        } 
                                                //fanObj->valueint = 0;
                                                //sendFlags |= FLAG_PARAMETERS;
                                                } 
                                           fanObj->valueint = cmd.getInt(); //
                                           if (!passiveMode)  sendFlags |= FLAG_PARAMETERS; //
                                           //if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,FLAG_PARAMETERS|FLAG_COMMAND,i->name);
                                          }
                else if (cmd.getCmd() == CMD_AUTO) 
                 {
                  setPassiveMode(i,true); //Setup flag
                  cmd.Cmd(CMD_OFF);
                  cmd.setSuffix(S_CMD);
                 }
                                         
             if (!cmd.isCommand()) break; // if have command in FAN suffix - continue processing
             case S_CMD:
              if (cmd.isCommand()) 
                                          {
                                          switch (cmd.getCmd())
                                          {
                                            case CMD_ON:
                                            if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -1;}
                                            if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != CMD_HALT) break;
                                            cmd.Percents255(fanObj->valueint);
                                            cmd.setSuffix(S_FAN);
                                            sendFlags |= FLAG_COMMAND | FLAG_PARAMETERS;
                                            //cmdObj->valueint = cmd.getCmd();
                                            cmdObj->valueint = getIntFromJson(i,"@precmd",CMD_FAN); 
                                            debugSerial<<"VENT: Turning ON. cmd:"<<cmdObj->valueint<<endl;
                                            cmd.Cmd(cmdObj->valueint);
                                            setPassiveMode(i,false);
                                            break;
                                            case CMD_OFF:
                                            if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -1;}
                                            if (cmdObj->valueint != CMD_OFF) setValToJson(i,"@precmd",cmdObj->valueint); //saving previous mode  
                                            cmd.Percents255(0);
                                            cmd.setSuffix(S_FAN);
                                            sendFlags |= FLAG_COMMAND; 
                                            //if (!passiveMode) sendFlags |= FLAG_PARAMETERS; 
                                            //else cmd.Cmd(CMD_AUTO);
                                            cmdObj->valueint = CMD_OFF;
                                            break;
                                            case CMD_ENABLE:
                                            if (pidObj && pidObj->valueint) ((PID *) pidObj->valueint)->SetMode(AUTOMATIC);
                                            sendFlags |= FLAG_FLAGS;
                                            setPassiveMode(i,false);
                                            break;

                                            case CMD_DISABLE:
                                            if (pidObj && pidObj->valueint) ((PID *) pidObj->valueint)->SetMode(MANUAL);
                                            sendFlags |= FLAG_FLAGS;
                                            setPassiveMode(i,false);
                                            break;

                                            case CMD_FREEZE:
                                            setFlag(i,FLAG_FREEZED);
                                            return 1;
                                            
                                            case CMD_UNFREEZE:
                                            clearFlag(i,FLAG_FREEZED);
                                            return 1;

                                            case CMD_AUTO:
                                            case CMD_HEATCOOL:
                                            case CMD_COOL:
                                            case CMD_HEAT:
                                            case CMD_FAN:
                                            case CMD_DRY:
                                            sendFlags |= FLAG_COMMAND;
                                            cmdObj->valueint = cmd.getCmd();
                                            setPassiveMode(i,false);
                                            break; 
                                            //todo - halt-rest-xon-xoff                                       
                                          }
                                          if (isNotRetainingStatus() && (cmdObj->valueint == CMD_ON) && (fanObj->valueint<20))
                                                                  {
                                                                    fanObj->valueint=30;
                                                                    cmd.Percents255(30);
                                                                    sendFlags |= FLAG_PARAMETERS;
                                                                  }
                                          }
                break;


              case S_SET:
               if (cmd.isValue())   
                                          {
                                           setValToJson(i,"set",cmd.getFloat());
                                           sendFlags |= FLAG_PARAMETERS;                               
                                          }              
                break;

              case S_VAL:
               if (cmd.isValue())   
                                          {
                                            debugSerial<<F("VENT: value ")<<cmd.getFloat()<<endl;
                                            setValToJson(i,"val",cmd.getFloat());      
                                          }
                                          return 1;
                break;  

              default:
                break;
              }  //switch
              if (isNotRetainingStatus())   //Send status separately for cmd, param, flags
                                            { 
                                            if (sendFlags & FLAG_COMMAND) item->SendStatusImmediate(cmd.setSuffix(S_CMD),FLAG_COMMAND,i->name);   
                                            if (sendFlags & FLAG_PARAMETERS ) item->SendStatusImmediate(cmd,FLAG_PARAMETERS,i->name);
                                            if (sendFlags & FLAG_FLAGS) item->SendStatusImmediate(cmd,FLAG_FLAGS,i->name);  
                                            }
              if (cascadeObj) 
                               {

                                    if (sendFlags & FLAG_COMMAND) SubmitParameters(cascadeObj,"cmd",cmd.setSuffix(S_CMD).setArgType(0),true);
                                    if (sendFlags & FLAG_PARAMETERS) 
                                    switch (cmd.getSuffix())
                                    {
                                    case S_SET:  
                                    SubmitParameters(cascadeObj,"set",cmd,true);
                                    break;
                                    case S_FAN:
                                    SubmitParameters(cascadeObj,"fan",cmd,true);
                                    break;
                                    }
                               }                        
              } // subitem

      if ((cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1) || passiveMode) 
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
executeCommand(acObj,-1,itemCmd().Percents255(fanV).setSuffix(S_FAN));
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
  bool passiveMode = getIntFromJson(i,"@pasv",0);   
 

  if (outObj && fanObj && cmdObj && outObj->type==aJson_Int && fanObj->type==aJson_Int && cmdObj->type==aJson_Int && V) 
  {
         long int out = 0;
        if (((cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1) || passiveMode) && maxRequestedV) 
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
