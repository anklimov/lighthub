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
  if (!item || !item->itemArg || !item->itemArg->type != aJson_Array) return;
 gatesObj = aJson.getArrayItem(item->itemArg,1);
  }

int  out_Multivent::Setup()
{
abstractOut::Setup();    
//getConfig();

//Expand Argument storage to 2
for (int i = aJson.getArraySize(item->itemArg); i < 2; i++)
            aJson.addItemToArray(item->itemArg, aJson.createItem( (long int) 0));

//Allocate objects to store persistent data in config tree
if (gatesObj)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             aJsonObject  * setObj = aJson.getObjectItem(i, "set");
             if (!setObj) aJson.addNumberToObject(i, "set", (long int) -1);

             aJsonObject  * cmdObj  = aJson.getObjectItem(i, "cmd");
             if (!setObj) aJson.addNumberToObject(i, "cmd", (long int) -1);

             aJsonObject * outObj = aJson.getObjectItem(i, "out");
             if (!setObj) aJson.addNumberToObject(i, "out", (long int) -1);

             i=i->next; 
          }
      }

debugSerial << F ("MultiVent init")<< endl;
driverStatus = CST_INITIALIZED;
return 1;
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
aJsonObject * currentGateObj = NULL;

if (gatesObj) i = gatesObj->child; // Pass 1 - calculate summ air value, max value etc

int activeV = 0;
int totalV  = 0;
int gateV   = 0;
//char * gateEmit = NULL;
int gateCmd = 0;
int maxV=0;

while (i)
{
  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");
   if (!setObj || !cmdObj || setObj->type!=aJson_Int || cmdObj->type!=aJson_Int) continue;

  int V         =aJson.getObjectItem(i,"V")->valueint;
  int requestedV=0;

  if (subItem && !strcmp (i->name,subItem)) 
          {
          currentGateObj=i;
          gateV=V;
  //        gateEmit=aJson.getObjectItem(i,"emit")->valuestring;
          
          gateCmd=cmdObj->valueint;
          if (cmdObj && cmd.isCommand()) 
                                      {
                                      cmdObj->valueint = cmd.getCmd();
                                      //publishTopic(i->name,cmdObj->valueint,"/set");
                                      item->SendStatusImmediate(SEND_COMMAND,i->name);
                                      }
          if (setObj && cmd.isValue())   
                                      {
                                      setObj->valueint = cmd.getPercents255();
                                      //publishTopic(i->name,setObj->valueint,"/set");
                                      item->SendStatusImmediate(SEND_PARAMETERS,i->name);
                                      }
          }

   if (cmdObj->valueint != CMD_OFF) 
                  { 
                  requestedV=V*setObj->valueint;
                  activeV+=requestedV;
                  }
   totalV+=V;
  // numberGates++;

   if(requestedV>maxV) maxV=requestedV;

  i=i->next;
}

int fanV=activeV/totalV;
debugSerial << F("Total V:")<<totalV<<F(" active V:")<<activeV<< F(" %:")<<fanV<< endl;

//uint8_t nGate=0;
i=NULL;
if (gatesObj) i = gatesObj->child; //Pass 2: re-distribute airflow

while (i)
{
//  nGate++;
  int V  =aJson.getObjectItem(i,"V")->valueint;

  aJsonObject * outObj=aJson.getObjectItem(i, "out"); 
  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");

  if (!outObj || !setObj || !cmdObj || outObj->type!=aJson_Int || setObj->type!=aJson_Int || cmdObj->type!=aJson_Int) continue;
  int out = 0;
  if (cmdObj->valueint != CMD_OFF) 
    {
      int requestedV=V*setObj->valueint;
      int out = requestedV*255/maxV;
      debugSerial<<i->name,(" Req:")<<requestedV<<F( Out:)<<out<<endl;
    } 
      if (out != outObj->valueint)
          {
            //report out
            executeCommand(i,-1,itemCmd().Percents255(out));
            outObj->valueint=out;
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
      item->setCmd(cmd.getCmd());
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
