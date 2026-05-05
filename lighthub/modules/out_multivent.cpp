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
              aJsonObject  * valObj = getCreateObject(i,"val",(float) NAN);      
              convert2float(valObj);
              aJsonObject  * poObj = getCreateObject(i,"po", (float) -2.0);    
              convert2float(poObj);
              
              int direction = DIRECT;
              float kP=getFloatFromJson(pidObj,"kP",0,1.0);     
              if (kP<0)
                {
                    kP=-kP;
                    direction=REVERSE;
                }       
              float kI=getFloatFromJson(pidObj,"kI",1,0.0);     
              float kD=getFloatFromJson(pidObj,"kD",2,0.0); 
              float dT=getFloatFromJson(pidObj,"dT",3,5.0); 

              pidObj->valueint = (long int) new PID  (&valObj->valuefloat, &poObj->valuefloat, &setObj->valuefloat, kP, kI, kD, P_ON_E,direction, 
                getFloatFromJson(i,"AlarmTime",4,120.0)*1000UL, getFloatFromJson(i,"AlarmValue",5,0.0));

              //((PID*) pidObj->valueint)->SetMode (AUTOMATIC);
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
  //debugSerial<<"VENT:active: ";
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


    void out_Multivent::stopAllzones(){
  if (!gatesObj) return;
  debugSerial << F("VENT: Stop all zones. ")<<endl;
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
                     setValToJson(i,"@preHaltcmd",cmd);
                    // setPassiveMode(i, true); 
                     fanCtrl(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD),i->name,true);
                    break;
                    }    
                  }    
       i=i->next; 
      }//while 
    }


    void out_Multivent::restoreAllzones(){
  if (!gatesObj) return;
  debugSerial << F("VENT: Restore all zones. ")<<endl;
        aJsonObject * i = gatesObj->child;  
      while (i)
          {
             if (i->name && *i->name)
             {
             int   preHaltcmd = getIntFromJson(i,"@preHaltcmd",CMD_OFF);
             if (preHaltcmd && (preHaltcmd != CMD_OFF)) //setPassiveMode(i, false); 
                 fanCtrl(itemCmd().Cmd(preHaltcmd).setSuffix(S_CMD),i->name,true);
              setValToJson(i,"@preHaltcmd",0); //reset preHaltcmd in any case
             }    
       i=i->next; 
      }//while    
    };

#define assign_if_positive(var, source) {int x=source; if (x>=0) {var=x;}} 

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
      int   acCmd    = getIntFromJson  (acObj,"mode");

      // global params
      int boostTreshold  = getIntFromJson  (acObj,"boost",AC_BOOST_TRESHOLD);
      int actualMode = CMD_FAN;
      if (acTemp>30.0) actualMode = CMD_HEAT;
          else if (acTemp<15.0) actualMode = CMD_COOL;
     
      if (isnan(acTemp) && acCmd) actualMode = acCmd; 

      aJsonObject * i = gatesObj->child;  
      int balance = 0;    
      bool ventRequested = false; //At least 1 ch requested FAN mode 
      bool autoRequested = false; //At least 1 ch requested AUTO mode 
      bool pidActive     = false; 
      bool pidComputed   = false;
      int  lastACfan     = -1;    
      while (i)
          {
             if (i->name && *i->name)
             {
             int     cmd = getIntFromJson  (i,"cmd");
             float   set = getFloatFromJson(i,"set");
             float   val = getFloatFromJson(i,"val");  
             int     fan = getIntFromJson(i,"fan");

             int execCmd = 0;   
             bool weakMode=false;   // kind of modes when we activating PID only if AC in active  mode
             switch (cmd)
                    {                
                    case CMD_HEATCOOL:
                    {
                       switch (acCmd)  //TODO - release heat/cool then PID released it, not just set <> val
                              {
                                case CMD_HEAT:
                                if (set>val) execCmd = CMD_HEAT;
                                if (set<val-AC_HEATCOOL_DEADBAND) execCmd = CMD_COOL;
                                break;
                                case CMD_COOL:
                                 if (set<val) execCmd = CMD_COOL;
                                 if (set>val+AC_HEATCOOL_DEADBAND) execCmd = CMD_HEAT;
                                break;
                                default:                                 
                                  if (set>val+AC_HEATCOOL_DEADBAND) execCmd = CMD_HEAT;
                                  if (set<val-AC_HEATCOOL_DEADBAND) execCmd = CMD_COOL;
                              }
                      }
                    break;
                    case CMD_FAN:
                    if (fan>0) ventRequested = true;
                    weakMode = true;
                    execCmd = cmd;
                    break;
                    case CMD_AUTO: 
                    if (fan>0) autoRequested = true;
                    weakMode = true;
                    execCmd = cmd;
                    break;
                    case CMD_COOL:
                    case CMD_HEAT:                     
                    case CMD_OFF:
                    execCmd = cmd;
                    break;
                    }   
             bool passiveMode =   getIntFromJson(i,"@pasv",0);       

             aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
             aJsonObject * poObj  = aJson.getObjectItem(i, "po");
             aJsonObject * valObj = aJson.getObjectItem(i, "val");

             if (pidObj && pidObj->valueint && poObj && poObj->type == aJson_Float && valObj && valObj->type == aJson_Float)
                        {
                        PID * p = (PID *) pidObj->valueint;   

                        /// Setup PID mode and direction based on real AC mode                        
                          switch (actualMode)   
                              { //if air hot or cold - uses temp PID and block control by /fan
                                case CMD_HEAT:
                                  if (weakMode || passiveMode)  p->SetMode(AUTOMATIC);
                                  p->SetControllerDirection(DIRECT); 
                                  break;
                                case CMD_COOL:
                                  if (weakMode || passiveMode)  p->SetMode(AUTOMATIC);
                                  p->SetControllerDirection(REVERSE); 
                                break;
                                default:
                                  if ((passiveMode || weakMode || execCmd ==CMD_OFF) && p->GetMode() == AUTOMATIC)
                                                    {
                                                        p->SetMode(MANUAL);
                                                        debugSerial<<F("VENT: PID set to MANUAL due no HEAT/COOL. zone:")<<i->name<<endl;
                                                        assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(getIntFromJson(i,"fan_in",0)).setSuffix(S_FAN),i->name,true));   
                                                    }
                              }         

                        if (!p->isOutdated() && (execCmd == CMD_HEAT || execCmd == CMD_COOL) && p->GetMode() == AUTOMATIC) pidActive = true;

                        if (p->Compute())
                             
                                {
                                  debugSerial<<F("VENT: ")
                                          <<item->itemArr->name<<"/"<<i->name
                                          <<F(" in:")<<p->GetIn()<<F(" set:")<<p->GetSet()<<F(" out:")<<p->GetOut() 
                                          <<" P:"<<p->GetKp()<<" I:"<<p->GetKi()<<" D:"<<p->GetKd()<<((p->GetDirection())?" Rev ":" Dir ")<<((p->GetMode())?"A":"M");
                                  if (p->isOutdated()) debugSerial << F(" <ALM>"); 
                                  debugSerial<<endl;

                                  if (!isnan(poObj->valuefloat))
                                  switch (execCmd)
                                  {
                                    case CMD_HEAT:
                               
                                     if (actualMode==CMD_COOL)  //close
                                          assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(0).setSuffix(S_FAN),i->name,true))
                                     else     
                                          assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true))

                                    balance+=poObj->valuefloat;
                                    pidComputed = true;

                                    break;
                                    case CMD_COOL:
                                    if (actualMode==CMD_HEAT) //close
                                          assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(0).setSuffix(S_FAN),i->name,true))
                                    else   
                                          assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true));

                                    balance-=poObj->valuefloat;
                                    pidComputed = true;
                                    break;

                                  default:
                                    switch (actualMode)
                                          {
                                            case CMD_HEAT:
                                              debugSerial<<F("VENT: HEAT PASS PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" out:")<< poObj->valuefloat <<endl;
                                              
                                               if (cmd !=CMD_OFF || passiveMode) assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true)); 
                                            break;
                                            case CMD_COOL:
                                              debugSerial<<F("VENT: COOL PASS PID: ")<<item->itemArr->name<<"/"<<i->name<<F(" out:")<< poObj->valuefloat <<endl;

                                               if (cmd !=CMD_OFF || passiveMode) assign_if_positive(lastACfan, fanCtrl(itemCmd().Percents255(poObj->valuefloat).setSuffix(S_FAN),i->name,true)); 
                                            break;
                                            case CMD_FAN: //no more hot or cold air
                                            ((PID *) pidObj->valueint)->SetMode(MANUAL);
                                          }  
                                }  
                             }  
                            else //PID not computed - maybe not in time, but we can use PID output as indicator of balance and boost if needed
                            { 
                              if (p->GetMode() == AUTOMATIC && !isnan(poObj->valuefloat))
                                {
                                  balance+=(p->GetDirection() == DIRECT) ? poObj->valuefloat : -poObj->valuefloat;
                                  //if (execCmd == CMD_HEAT) balance+=poObj->valuefloat;
                                  //else if (execCmd == CMD_COOL) balance-=poObj->valuefloat;
                                }
                            }                 
                        }
        
             } 
             i=i->next; 
          }//while

          if (pidComputed) //Active mode. PIDs in pasive mode do not triggered this flag
          {
                debugSerial<<F("VENT: Chan balance=")<<balance<<F(" treshold:")<<boostTreshold<<endl;

                if (balance>boostTreshold)  setBoost(itemCmd().Cmd(CMD_HEAT).Int(AC_BOOST_HIGH_TEMP).setSuffix(S_SET));                              
                  else  if (-balance>boostTreshold)  setBoost(itemCmd().Cmd(CMD_COOL).Int(AC_BOOST_LOW_TEMP).setSuffix(S_SET));
                          else
                          { 
                            if (abs(balance)<AC_BOOST_DEADBAND) pidActive = false; //No request from active PID = no active PID 
                          }
                  }       
           bool noFurtherModes = false; 
           bool activeMode = (actualMode==CMD_HEAT || actualMode==CMD_COOL);

           if (!pidActive) // No active mode PID - no boost needed, but maybe vent still requested 
           
                          { 
                            resetBoost();      
                            if (autoRequested && lastACfan>0) sendACcmd(itemCmd().Cmd(CMD_AUTO));
                            else if (ventRequested && lastACfan>0) sendACcmd(itemCmd().Cmd(CMD_FAN));
                                  else noFurtherModes = true; //No AUTO or FAN mode requested - so we can skip sending command to AC at all and save some energy on it
                          }

           if (lastACfan>0 || (lastACfan == 0 && (!activeMode || noFurtherModes))) sendACcmd(itemCmd().Percents255(lastACfan).setSuffix(S_FAN)); //if PID already set some fan value - send it to AC                
             
return 1;
};



int out_Multivent::getChanType()
{
   return CH_THERMO; 
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
                  setValToJson(zone,"fan",0); //reset fan level on passive mode ON
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
  int result =  fanCtrl(cmd,subItem, false);

  if (result>=0) 
            {
            debugSerial << F("VENT: CTRL fan level: ")<<result<<endl;  
            sendACcmd(itemCmd().Percents255(result).setSuffix(S_FAN));
            }
  else   debugSerial << F("VENT: CTRL result: ")<<(result==-1? F("OK") : F("not OK"))<<endl;          
//   else debugSerial << F("VENT: Skip turning OFF by fan")<<endl;
  return (result == -2)? 0 : 1; 
}

// Return:
// Main AC fan value (0..255) or 
// -1 unknown fan + success 
// -2 unknown fan + error (forbidden by PID or frozen by freeze flag)
int out_Multivent::fanCtrl(itemCmd cmd,   char* subItem , bool force)
{
if (!gatesObj || !acObj) return -2;
if (cmd.getCmd()==CMD_DISABLE || cmd.getCmd()==CMD_ENABLE) return -2;
int suffixCode = cmd.getSuffix();
debugSerial << "VENT: CTRL " << subItem << " "; cmd.debugOut();

if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

int turnbyfan = getIntFromJson(acObj,"turnbyfan",0);   

if (cmd.getSuffix()==S_FAN)
{
      switch (cmd.getCmd())
          {
          case CMD_HIGH:
          cmd.Percents255(255);
          cmd.Cmd(0);
          break;

          case CMD_MED:
          cmd.Percents255(128);
          cmd.Cmd(0);
          break;

          case CMD_LOW:
          cmd.Percents255(64);
          cmd.Cmd(0);
          break;

//          case CMD_OFF:
//          cmd.Percents255(0);
//          cmd.Cmd(0);
//          break;
  default: 
  if (cmd.isValue()) cmd.Percents255(cmd.getInt()); // convert to integer       
} //switch cmd
}

if (!subItem)  // feedback from shared AC
{
switch (suffixCode)
    {
    case S_VAL:
            if (cmd.isValue())
            {
            debugSerial << F("VENT: ")<<F("AC air temp: ")<< cmd.getFloat()<<endl;
            item->setExt(millisNZ()); //setup validity interval
            setValToJson(acObj,"val",cmd.getFloat());
            } 
    return -1;

    case S_FAN:
            if (cmd.isValue())
            {
            debugSerial << F("VENT: ")<<F("AC fan: ")<< cmd.getInt()<<endl;
            checkACfan(cmd.getInt());
            setValToJson(acObj,"fan",cmd.getInt());    
            } 
    return -1;

    case S_SET:
            if (cmd.isValue())
            {
            debugSerial << F("VENT: ")<<F("AC set: ")<< cmd.getFloat()<<endl;
            checkACset(cmd.getFloat());
            setValToJson(acObj,"set",cmd.getFloat()); 
            if (cmd.getFloat() == AC_BOOST_HIGH_TEMP || cmd.getFloat() == AC_BOOST_LOW_TEMP)
            {
                setFlag(acObj, FLAG_ACTION_NEEDED);
                debugSerial<<F("VENT: boost mode detected")<<endl;
            }
            }
    return -1;
    
    case S_MODE:
            if (cmd.isCommand())
            {
            debugSerial << F("VENT: ")<<F("AC mode: ")<< cmd.getCmd()<<endl;
            checkACcmd(cmd.getCmd());
            setValToJson(acObj,"mode",cmd.getCmd()); 
            }
    return -1;   

    case S_CMD:
 
            switch (cmd.getCmd())
             {
              case CMD_OFF:
              stopAllzones();
              break;
              case CMD_ON:
              case CMD_HEAT:
              case CMD_COOL:   
              case CMD_HEATCOOL:
              case CMD_DRY:
              case CMD_FAN:
              case CMD_AUTO: 
              restoreAllzones();
             }
    return -1;

    //case S_TEMP: - temp corrupted by core
    //        debugSerial << F("VENT:")<<F("AC air roomtemp: ")<< cmd.getFloat()<<endl;
    //        setValToJson(acObj,"roomtemp",cmd.getFloat());
    //return -1;
    } 
}

aJsonObject * i = NULL;

if (gatesObj) i = gatesObj->child; // Pass 1 - calculate summ air value, max value etc

int activeV = 0;
int totalV  = 0;
int maxV=0;
int maxRequestedV=0;
int maxPercent=0;
bool exitAfterSendingStatus = false;


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
               if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -2;}
               if (cmd.isValue())   
                                          {   
                                           
                                            if (!force) 
                                            {
                                               setValToJson(i,"fan_in",cmd.getInt()); //fan controlled by external source, save input value to restore after exit from passive mode
                                                debugSerial<<"VENT: external fan level stored: "<<cmd.getInt()<<endl;
                                              if (pidEnabled(pidObj))
                                                {
                                                  debugSerial<<F("VENT: FAN control disabled by PID")<<endl;
                                                  return -2;
                                                }  
                                              }
                                    
                                          if (cmd.getInt())
                                                {
                                                if (cmdObj->valueint == CMD_OFF && turnbyfan)
                                                    { 
                                                     cmd.Cmd(turnbyfan);
                                                     debugSerial<<"VENT: generating cmd by fan: "<<turnbyfan<<endl;
                                                        }                                              
                                                }
                                           else
                                                {
                                             if (cmdObj->valueint != CMD_OFF && turnbyfan)
                                                    { 
                                                      cmd.Cmd(CMD_OFF);
                                                      debugSerial<<"VENT: Turning OFF  by fan"<<endl;                                                      
                                                        } 
                                                } 
                                           fanObj->valueint = cmd.getInt(); //
                                           if (!passiveMode)  sendFlags |= FLAG_PARAMETERS; //
                                           //if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,FLAG_PARAMETERS|FLAG_COMMAND,i->name);
                                          }
                else if (cmd.getCmd() == CMD_AUTO) 
                 {
                  setPassiveMode(i,true); //Setup flag
                  passiveMode = true;
                  cmd.Cmd(CMD_OFF);
                  cmd.setSuffix(S_CMD);
                 }
                 else if (cmd.getCmd() == CMD_OFF) 
                 {
                  setPassiveMode(i,false);
                  passiveMode = false;
                 }    
                   
             if (!cmd.isCommand()) break; // if have command in FAN suffix - continue processing
             debugSerial<<"VENT: cmd in FAN suffix, process as command. cmd="<<cmd.getCmd()<<endl;
             case S_CMD:
              if (cmd.isCommand()) 
                                          {
                                          bool checkMinFanLevel = false;
                                          if (cmd.getCmd() == CMD_ON)
                                          {
                                            if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -2;}
                                            if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != CMD_HALT) break;
                                            cmd.Percents255(fanObj->valueint);
                                            cmd.setSuffix(S_FAN);
                                            sendFlags |= FLAG_COMMAND | FLAG_PARAMETERS;
                                            //cmdObj->valueint = cmd.getCmd();
                                            cmdObj->valueint = getIntFromJson(i,"@precmd",CMD_FAN); 
                                            debugSerial<<"VENT: Turning ON. cmd:"<<cmdObj->valueint<<endl;
                                            cmd.Cmd(cmdObj->valueint);
                                            setPassiveMode(i,false);
                                          }
                                          
                                          switch (cmd.getCmd())
                                          {
                                            case CMD_ON:
                                            checkMinFanLevel = true;
                                            break;
                                            case CMD_OFF:
                                            if (getFlag(i,FLAG_FREEZED)) {debugSerial<<F("VENT: zone frozen")<<endl; return -2;}
                                            if (cmdObj->valueint != CMD_OFF) setValToJson(i,"@precmd",cmdObj->valueint); //saving previous mode  
                                            cmd.Percents255(0);
                                            cmd.setSuffix(S_FAN);
                                            debugSerial<<"VENT: Turning OFF. saving cmd:"<<cmdObj->valueint<<endl;
                                            sendFlags |= FLAG_COMMAND; 
                                            //sendFlags |= FLAG_PARAMETERS; //experimental 30/03/26
                                            //
                                            if (!passiveMode) sendFlags |= FLAG_PARAMETERS; 
                                            //   else cmd.Cmd(CMD_AUTO);

                                            cmdObj->valueint = CMD_OFF;
                                            enablePid(pidObj,false);
                                            break;
                                            /*
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
                                            */

                                            case CMD_FREEZE:
                                            setFlag(i,FLAG_FREEZED);
                                            return -1;
                                            
                                            case CMD_UNFREEZE:
                                            clearFlag(i,FLAG_FREEZED);
                                            return -1;

                                            case CMD_COOL:
                                            case CMD_DRY:
                                            enablePid(pidObj,true,REVERSE);
                                            sendFlags |= FLAG_COMMAND;
                                            cmdObj->valueint = cmd.getCmd();
                                            setPassiveMode(i,false);
                                            checkMinFanLevel = true;
                                            break;

                                            case CMD_HEAT:
                                            enablePid(pidObj,true,DIRECT);
                                            case CMD_HEATCOOL:
                                            enablePid(pidObj,true);

                                            sendFlags |= FLAG_COMMAND;
                                            cmdObj->valueint = cmd.getCmd();
                                            setPassiveMode(i,false);
                                            checkMinFanLevel = true;
                                            break;

                                            case CMD_FAN:
                                            sendFlags |= FLAG_PARAMETERS; //experimental 30/03/26
                                            cmd.Percents255(fanObj->valueint);
                                            cmd.setSuffix(S_FAN);
                                            // continue
                                            case CMD_AUTO:
                                            enablePid(pidObj,false);
                                            sendFlags |= FLAG_COMMAND;
                                            cmdObj->valueint = cmd.getCmd();
                                            setPassiveMode(i,false);
                                            checkMinFanLevel = true;
                                            break; 
                                            //todo - halt-rest-xon-xoff                                       
                                          }
                                          if (isNotRetainingStatus() && checkMinFanLevel  && (fanObj->valueint<20))
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
                                           exitAfterSendingStatus = true; //if setpoint updated - send status immediately to update PID and sending status                               
                                          } 
                else return -1;                                       
                break;

              case S_VAL:
               if (cmd.isValue())   
                                          {
                                            aJsonObject * pidObj = aJson.getObjectItem(i, "pid");
                                              if (pidObj && pidObj->valueint) 
                                                  {
                                                    ((PID *) pidObj->valueint)->SetVal(cmd.getFloat());
                                                    debugSerial<<F("VENT: PID input: ")<<i->name<<F(" updated by temp: ")<< cmd.getFloat()<<endl;
                                                  }
                                          }
                                          return -1;
                break;  

              default:
                break;
              }  //switch
              if (isNotRetainingStatus())   //Send status separately for cmd, param, flags
                                            { 
                                            if (sendFlags & FLAG_COMMAND) item->SendStatusImmediate(itemCmd().Cmd(cmd).setSuffix(S_CMD),FLAG_COMMAND,i->name);   
                                            if (sendFlags & FLAG_PARAMETERS ) item->SendStatusImmediate(cmd,FLAG_PARAMETERS,i->name);
                                            if (sendFlags & FLAG_FLAGS) item->SendStatusImmediate(cmd,FLAG_FLAGS,i->name);  
                                            }
              if (cascadeObj) 
                               {

                                    if (sendFlags & FLAG_COMMAND) SubmitParameters(cascadeObj,"cmd",itemCmd().Cmd(cmd).setSuffix(S_CMD).setArgType(0),true);  
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
      if (exitAfterSendingStatus) return -1; //if setpoint updated - send status immediately to update PID and sending status

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



//if (fanV || allowToTurnOffByFan) sendACcmd(itemCmd().Percents255(fanV).setSuffix(S_FAN));
//   else debugSerial << F("VENT: Skip turning OFF by fan")<<endl;


//Move gates only if fan is actually on
if (!fanV) return 0;

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

return fanV;
}

void out_Multivent::enablePid(aJsonObject* pidObj, int enable, int direction )
 {
 if (pidObj && pidObj->valueint) 
      {
      ((PID *) pidObj->valueint)->SetMode(enable);
      if (direction != -1) ((PID *) pidObj->valueint)->SetControllerDirection(direction);
   }
 }


bool out_Multivent::pidEnabled(aJsonObject* pidObj)
 {
 return  ((pidObj && pidObj->valueint) && (((PID *) pidObj->valueint)->GetMode() ==AUTOMATIC));
 } 


 void out_Multivent::checkACcmd (int acCmd)
{
   if (!acObj) return;
   int lastCmd = getIntFromJson(acObj,"@lastCmd");
   int prevACcmd = getIntFromJson(acObj,"mode");
   int lastFan = getIntFromJson(acObj,"@lastFan",-1);

   if (!prevACcmd) {
                          debugSerial<<"VENT: AC MODE received initially "<<acCmd<<endl; 
                          setValToJson(acObj,"@lastCmd",acCmd);
                          return;
   }

  if (lastCmd && (acCmd != lastCmd) ) { 
                          
                if (!(acCmd == CMD_OFF && lastFan == 0)) 
                  {
                     debugSerial<<"VENT: AC MODE changed manually from "<<lastCmd<<" to "<<acCmd<<endl; 

                    switch (acCmd)
                    {
                      case CMD_OFF:
                      stopAllzones();
                      break;  
                      default:
                      restoreAllzones();
                      break;
                    } 

                    aJsonObject * execObj = aJson.getObjectItem(acObj, "onextcmd");
                     if (execObj) 
                    {
                    executeCommand(execObj,-1,itemCmd().Cmd(acCmd).setSuffix(S_CMD).setArgType(0).doReverseMapping(aJson.getObjectItem(execObj, "map")));
                    }                     
                }
                          setValToJson(acObj,"@lastCmd",acCmd);
                          setValToJson(acObj,"@lastFan",-1);
                          return;}

}

 void out_Multivent::checkACfan (int acFan)
{
   if (!acObj) return;
   int prevACfan = getIntFromJson(acObj,"fan");
   if (acFan != prevACfan) 
    {
      debugSerial<<"VENT: AC FAN changed, delete lastfan"<<endl; 
      setValToJson(acObj,"@lastFan",-1);
      return;
    }
}

 void out_Multivent::checkACset (int acSet)
{
   if (!acObj) return;
   int lastSet = getIntFromJson(acObj,"@lastSet",-1);
   //int prevACset = getIntFromJson(acObj,"set");

    if (lastSet == -1) {
                          debugSerial<<"VENT: AC SET received initially "<<acSet<<endl; 
                          setValToJson(acObj,"@lastSet",acSet);
                          return;
    }       

  if (lastSet && (acSet != lastSet)) {
                          debugSerial<<"VENT: AC SET changed manually from "<<lastSet<<" to "<<acSet<<endl; 
                          aJsonObject * execObj = aJson.getObjectItem(acObj, "onextcmd");
                            if (execObj) 
                            {
                            executeCommand(execObj,-1,itemCmd().Int(acSet).setSuffix(S_SET).doReverseMapping(aJson.getObjectItem(execObj, "map")));
                            }  
                          setValToJson(acObj,"@lastSet",acSet);
                          return;}

}



 int out_Multivent::sendACcmd (itemCmd cmd)
{
   if (!acObj) return 0;
   int supress = getIntFromJson(acObj,"supress",0xF);
   
  
   //int acCmd = getIntFromJson(acObj,"mode");
   
   //if (lastCmd && (acCmd != lastCmd)) {
   //                       //debugSerial<<"VENT: AC MODE changed manually to "<<item->getCmd()<<endl; 
   //                       return 0;}
   
   if (cmd.isCommand())
   {
          int lastCmd = getIntFromJson(acObj,"@lastCmd");
          int overrideCmd = getIntFromJson(acObj,"overridecmd");
          if (overrideCmd && cmd.getCmd() != CMD_OFF) cmd.Cmd(overrideCmd); //if override cmd exist - use it instead of requested. But allow to turn off by original cmd

          if (cmd.getCmd() == lastCmd && (supress & AC_SUPPRESS_CMD)) 
                                  {
                                  //debugSerial<<"VENT: AC MODE already "<<lastCmd<<endl; 
                                  }
          else 
          {        
          debugSerial<<"VENT: sendACcmd. cmd="; cmd.debugOut(); debugSerial<<endl;                     
          executeCommand(acObj,-1,itemCmd().Cmd(cmd).setSuffix(S_CMD).setArgType(0));
          setValToJson(acObj,"@lastCmd",cmd.getCmd());
          setValToJson(acObj,"@lastFan",-1); //reset last fan to avoid suppression after mode change
          }
   }
  if (cmd.isValue())
  {
    int lastFan = getIntFromJson(acObj,"@lastFan",-1);
    int lastSet = getFloatFromJson(acObj,"@lastSet");
    int prevACcmd = getIntFromJson(acObj,"mode",-1);
    bool isActiveNow = (prevACcmd != CMD_OFF && prevACcmd != -1) || (lastFan>0);

    switch (cmd.getSuffix())
    {
    case S_FAN:
    if (!(supress & AC_SUPPRESS_FAN) || lastFan != cmd.getInt()) {
        debugSerial<<"VENT: sendACcmd. fan="; cmd.debugOut(); debugSerial<<endl;  
        executeCommand(acObj,-1,cmd.Cmd(0));
    }
    setValToJson(acObj,"@lastFan",cmd.getInt());

    if (cmd.getInt() == 0) 
        {
          /////setValToJson(acObj,"@lastCmd",CMD_OFF);
          if (isActiveNow) setValToJson(acObj,"@preCmd",prevACcmd);
        }
     else if (cmd.getInt() > 0 && !isActiveNow) 
        {
          int preCmd  = getIntFromJson(acObj,"@preCmd",CMD_FAN);
          setValToJson(acObj,"@lastCmd",preCmd);
        } 
    break;
    case S_SET:
    if (!(supress & AC_SUPPRESS_SET) || lastSet != cmd.getFloat()) {
        debugSerial<<"VENT: sendACcmd. set="; cmd.debugOut(); debugSerial<<endl;
        executeCommand(acObj,-1,cmd.Cmd(0));
    }
    setValToJson(acObj,"@lastSet",cmd.getFloat());
    break;
    default:
    executeCommand(acObj,-1,cmd.Cmd(0));
    break;
    }

  }
   return 1;
}

void out_Multivent::setBoost(itemCmd cmd)
  {
   if (!acObj || getFlag(acObj,FLAG_ACTION_NEEDED)) return;
   debugSerial<<"VENT: boost on"<<endl;
   int acSetTemp = getIntFromJson(acObj,"set",AC_PRESET_TEMP);  
   setValToJson(acObj,"@preset",acSetTemp);
   sendACcmd(cmd);
   setFlag(acObj,FLAG_ACTION_NEEDED);
  }

void out_Multivent::resetBoost()
  {
  if (!acObj) return;  
  //int acSetTemp = getIntFromJson(acObj,"set");
  if (!getFlag(acObj,FLAG_ACTION_NEEDED)) return;  
  debugSerial<<"VENT: boost off"<<endl;
  int preTemp = getIntFromJson(acObj,"@preset",AC_PRESET_TEMP); 
  if (preTemp<=AC_BOOST_LOW_TEMP) preTemp = AC_PRESET_TEMP;
  if (preTemp>=AC_BOOST_HIGH_TEMP) preTemp = AC_PRESET_TEMP;
  sendACcmd(itemCmd().Cmd(0).Int(preTemp).setSuffix(S_SET));
  clearFlag(acObj,FLAG_ACTION_NEEDED);
  }

void  out_Multivent::notifyState(itemCmd state)
{
char val[16];  
state.toString(val,sizeof(val),FLAG_COMMAND);
publishTopic(item->itemArr->name,val,"/$state");
}

#endif
