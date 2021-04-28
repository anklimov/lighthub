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

//Expand Argument storage to 3
for (int i = aJson.getArraySize(item->itemArg); i < 2; i++)
            aJson.addItemToArray(item->itemArg, aJson.createItem( (long int) 0));

//Allocate objects to store persistent data in config tree
if (gatesObj)
      {
      aJsonObject * i = gatesObj->child;            
      while (i)
          {
             aJsonObject  * setObj = aJson.getObjectItem(i, "set");
             if (!setObj) aJson.addNumberToObject(i, "set", (long int) 0);

             aJsonObject  * cmdObj  = aJson.getObjectItem(i, "cmd");
             if (!setObj) aJson.addNumberToObject(i, "cmd", (long int) 0);

             aJsonObject * outObj = aJson.getObjectItem(i, "out");
             if (!setObj) aJson.addNumberToObject(i, "out", (long int) 0);

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
aJsonObject * gateObj = NULL;
int suffixCode = cmd.getSuffix();
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

//if (subItem && gatesObj)
//{
//gateObj = aJson.getObjectItem(gatesObj,subItem);
//}

aJsonObject * i = NULL;
if (gatesObj) i = gatesObj->child;

int activeV = 0;
int totalV = 0;
int gateV = 0;
char * gateEmit = NULL;
int gateCmd = 0;

while (i)
{
  int V  =aJson.getObjectItem(i,"V")->valueint;
  aJsonObject * setObj=aJson.getObjectItem(i, "set");
  aJsonObject * cmdObj=aJson.getObjectItem(i, "cmd");

  if (subItem && !strcmp (i->name,subItem)) 
          {
          gateObj=i;
          gateV=V;
          gateEmit=aJson.getObjectItem(i,"emit")->valuestring;
          gateCmd=cmdObj->valueint;
          if (cmdObj && cmd.isCommand()) cmdObj->valueint = cmd.getCmd();
          if (setObj && cmd.isValue())   setObj->valueint = cmd.getPercents255();
          }

   if (cmdObj && cmdObj->valueint != CMD_OFF) activeV+=setObj->valueint*V;
   totalV+=V;

  i=i->next;
}

int fanV=activeV*255/totalV;

//RECALC gates
//APPLY every gate
//SUB current gate

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
