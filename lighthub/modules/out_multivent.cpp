#ifndef MULTIVENT_DISABLE

#include "modules/out_multivent.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"

static int driverStatus = CST_UNKNOWN;

void out_Multivent::getConfig()
{
 gatesObj = NULL;
  if (!item || !item->itemArg || item->itemArg->type != aJson_Object) return;
 gatesObj = item->itemArg;
  }

int  out_Multivent::Setup()
{
abstractOut::Setup();    
//getConfig();

//Expand Argument storage to 2
//for (int i = aJson.getArraySize(item->itemArg); i < 2; i++)
//            aJson.addItemToArray(item->itemArg, aJson.createItem( (long int) 0));

//Allocate objects to store persistent data in config tree
if (gatesObj /*&& aJson.getArraySize(item->itemArg)>=2*/)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             if (i->name && *i->name)
             {
             aJsonObject  * setObj = aJson.getObjectItem(i, "set");
             if (!setObj) aJson.addNumberToObject(i, "set", (long int) -1);

             aJsonObject  * cmdObj  = aJson.getObjectItem(i, "cmd");
             if (!cmdObj) aJson.addNumberToObject(i, "cmd", (long int) -1);

             aJsonObject * outObj = aJson.getObjectItem(i, "out");
             if (!outObj) aJson.addNumberToObject(i, "out", (long int) -1);
             } 
             i=i->next; 
          }
      debugSerial << F ("MultiVent init")<< endl;
      driverStatus = CST_INITIALIZED;
      return 1;
      }

debugSerial << F ("MultiVent config failed")<< endl;
return 0;

}

int  out_Multivent::Stop()
{
debugSerial << F ("Multivent De-Init") << endl;
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_Multivent::Status()
{
return driverStatus;
}
/*
int out_Multivent::isActive()
{
itemCmd st;  
switch (item->getCmd())
{ 
  case CMD_OFF:
  case CMD_HALT:
  return 0;
  break;
  default:
st.loadItem(item);
return st.getPercents255();
}  
}
*/
int out_Multivent::Poll(short cause)
{
return 0;
};

int out_Multivent::getChanType()
{
   return CH_PWM;
}



int out_Multivent::Ctrl(itemCmd cmd,   char* subItem , bool toExecute)
{

int suffixCode = cmd.getSuffix();
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

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

  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");
  aJsonObject * cascadeObj=aJson.getObjectItem(i, "cas");
   if (setObj && cmdObj && setObj->type==aJson_Int && cmdObj->type==aJson_Int) 
   {

      int V         =aJson.getObjectItem(i,"V")->valueint;
      int requestedV=0;

      if (subItem && !strcmp (i->name,subItem)) 
              {
              if (cmdObj && cmd.isCommand()) 
                                          {
                                          cmdObj->valueint = cmd.getCmd();
                                          //publishTopic(i->name,cmdObj->valueint,"/set");
                                          switch (cmd.getCmd())
                                          {
                                            case CMD_ON:
                                            cmd.Percents255(setObj->valueint);
                                            break;
                                            case CMD_OFF:
                                            cmd.Percents255(0);
                                          }
                                          if (isNotRetainingStatus() && (cmdObj->valueint == CMD_ON) && (setObj->valueint<20))
                                                                  {
                                                                    setObj->valueint=30;
                                                                    cmd.Percents255(30);
                                                                    //if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,SEND_PARAMETERS,i->name);
                                                                  }

                                          if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,SEND_COMMAND|SEND_PARAMETERS,i->name);
                                          }

              else if (setObj && cmdObj && suffixCode == S_FAN && cmd.isValue())   
                                          {
                                          if (cmd.getInt())
                                                {

                                                if (cmdObj->valueint == CMD_OFF || cmdObj->valueint == -1) 
                                                    { 
                                                     debugSerial<<"Turning ON"<<endl;
                                                     cmdObj->valueint = CMD_ON; 
                                                     cmd.Cmd(CMD_ON);
                                                     //if (isNotRetainingStatus()) item->SendStatusImmediate(itemCmd().Cmd(CMD_ON),SEND_COMMAND,i->name);
                                                        } 
                                                
                                                setObj->valueint = cmd.getInt();
                                                }
                                           else
                                                {
                                             if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1)  
                                                    { debugSerial<<"Turning OFF"<<endl; 
                                                      cmdObj->valueint = CMD_OFF; 
                                                      cmd.Cmd(CMD_OFF);
                                                      //if (isNotRetainingStatus()) item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF),SEND_COMMAND,i->name);
                                                        } 
              
                                                setObj->valueint = 0;
                                                }

                                           if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,SEND_PARAMETERS|SEND_COMMAND,i->name);
                                          }

              else if (setObj && cmd.isValue())   
                                          {
                                          setObj->valueint = cmd.getPercents255();
                                          //publishTopic(i->name,setObj->valueint,"/set");
                                          if (isNotRetainingStatus()) item->SendStatusImmediate(cmd,SEND_PARAMETERS,i->name);
                                          }
              if (cascadeObj) executeCommand(cascadeObj,-1,cmd);                           
              }

      if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1) 
                      { 
                      requestedV=V*setObj->valueint;
                      activeV+=requestedV;

                            if (setObj->valueint>maxPercent )
                                  {
                                  maxRequestedV=requestedV;
                                  maxV=V;
                                  maxPercent=setObj->valueint;
                                  }
                      }
      totalV+=V;

   }
  i=i->next;
}

if (!totalV) return 0;

int fanV=activeV/totalV;
debugSerial << F("Total V:")<<totalV<<F(" active V:")<<activeV/255<< F(" fan%:")<<fanV<< F(" Max request:")<<maxRequestedV/255 <<F(" from ")<<maxV<<F(" m3")<< endl;

executeCommand(aJson.getObjectItem(gatesObj, ""),-1,itemCmd().Percents255(fanV).Cmd((fanV)?CMD_ON:CMD_OFF));

i=NULL;
if (gatesObj) i = gatesObj->child; //Pass 2: re-distribute airflow

while (i)
{
  int V  =aJson.getObjectItem(i,"V")->valueint;

  aJsonObject * outObj=aJson.getObjectItem(i, "out"); 
  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");

  if (outObj && setObj && cmdObj && outObj->type==aJson_Int && setObj->type==aJson_Int && cmdObj->type==aJson_Int && V) 
  {
         long int out = 0;
        if (cmdObj->valueint != CMD_OFF && cmdObj->valueint != -1 && maxRequestedV) 
          {
            int requestedV=V*setObj->valueint;
            out = (( long)requestedV*255L)/(( long)V)*( long)maxV/( long)maxRequestedV;
            debugSerial<<i->name<<(" Req:")<<requestedV/255<<F(" Out:")<<out<<endl;
          } 

          // executeCommand(i,-1,itemCmd().Percents255(out));
            if (out != outObj->valueint)
                {
                  //report out
                  executeCommand(i,-1,itemCmd().Percents255(out));
                  outObj->valueint=out;
                }
  }        
  i=i->next;
}

return 1;

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
debugSerial<<F("Forced execution");
case S_SET:
          if (!cmd.isValue()) return 0;

          return 1;
          //break;

case S_CMD:
      //item->setCmd(cmd.getCmd());
      switch (cmd.getCmd())
          {
          case CMD_ON:

            return 1;

            case CMD_OFF:

            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

#endif
