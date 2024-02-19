#ifndef RELAY_DISABLE
#include "modules/out_relay.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "dmx.h"
#include "utils.h"

void out_relay::getConfig()
{
  inverted=false;

  if (!item) return;
  pin=item->getArg(0);
  if (pin<0)
            {
              pin=-pin;
              inverted=true;
            }
  if(pin==0 || pin>=PINS_COUNT) pin=32;

  period = item->getFloatArg(1)*1000.0; 
  if (!period)   period = 5000UL;
          
}

#define ACTIVE (inverted)?LOW:HIGH
#define INACTIVE (inverted)?HIGH:LOW


int  out_relay::Setup()
{
abstractOut::Setup();    

debugSerial<<F("Relay-Out #")<<pin<<F(" init")<<endl;
if (isProtectedPin(pin)) {errorSerial<<F("pin disabled")<<endl;return 0;}
pinMode(pin, OUTPUT);
digitalWrite(pin,INACTIVE);
if (item) item->setExt(0);
//if (item->getCmd()) item->setFlag(FLAG_COMMAND);
//if (item->itemVal)  item->setFlag(FLAG_PARAMETERS);
setStatus(CST_INITIALIZED);
if (item && (item->isActive()>0))  ///????
    {
    item->setExt(millisNZ());
    }
return 1;
}

int  out_relay::Stop()
{
debugSerial<<F("Relay-Out #")<<pin<<F(" stop")<<endl;
pinMode(pin, INPUT);
setStatus(CST_UNKNOWN);
return 1;
}

void  out_relay::relay(bool state)
{
char subtopic[10]="/";
char val[10];  
digitalWrite(pin,(state)?ACTIVE:INACTIVE);
if (period<1000) return;
debugSerial<<F("Out ")<<pin<<F(" is ")<<(state)<<endl;

strcat_P(subtopic,action_P);
short cmd=item->getCmd();
if (state) 
 switch(cmd)
 {
  case CMD_COOL:
     strcpy_P(val,cooling_P);
     break;
  //case CMD_AUTO:
  //case CMD_HEAT:
  //case CMD_ON:
  //   
  //   break;
  case CMD_DRY:
     strcpy_P(val,drying_P);
     break;
  case CMD_FAN:
      strcpy_P(val,fan_P);
     break;
  default:
     strcpy_P(val,heating_P);
  }
   else //turned off
       if (cmd==CMD_OFF)  strcpy_P(val,off_P);
          else strcpy_P(val,idle_P);

debugSerial << F("pub action ") << publishTopic(item->itemArr->name,val,subtopic)<<F(":")<<item->itemArr->name<<subtopic<<F("=>")<<val<<endl;
}


int out_relay::Poll(short cause)
{
  if (cause==POLLING_SLOW) return 0;
  if (!item) return 0;
  itemCmd st;
  st.loadItem(item);
  int val = st.getPercents255();
short cmd = st.getCmd();
uint32_t timer = item->getExt(); 

bool needToOff = isTimeOver(timer,millis(),period*val/255);
  
   if (timer && isTimeOver(timer,millis(),period))
    {
      item->setExt(millisNZ());
      if (val && (getPinVal(pin) == INACTIVE)) relay(true);
    }   

    else if (timer && (getPinVal(pin) == ACTIVE) && needToOff)
    {
      relay(false);
      if (!item->isActive()) item->setExt(0);
    }  

    else if (timer && val && (getPinVal(pin) == INACTIVE) && !needToOff)
      relay(true);


    return 0;
};


int out_relay::Ctrl(itemCmd cmd, char* subItem, bool toExecute,bool authorized)
{
debugSerial<<F("relayCtr: ");
cmd.debugOut();
if ((subItem && !strcmp_P(subItem,action_P)) || !item) return 0;

if (isProtectedPin(pin)) {return 0;}

int suffixCode;
if (cmd.isCommand()) suffixCode = S_CMD;
   else suffixCode = cmd.getSuffix();

switch(suffixCode)
{ 
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
          if (toExecute)
          {
          if (cmd.getPercents255()) 
            {
             if (!item->getExt()) 
                                    {
                                    item->setExt(millisNZ());
                                    //relay(true);
                                    }
            }
          else        
                    {     
                    item->setExt(0);
                    relay(false);              
                    } 
          } else //not execute
            {
              switch (item->getCmd())
              {
                case CMD_AUTO:
                case CMD_ON:
                case CMD_COOL:
                case CMD_DRY:
                case CMD_HEAT:
                if (cmd.getPercents255() && !item->getExt()) item->setExt(millisNZ());
              }
            }
          return 1;
case S_CMD:

      switch (cmd.getCmd())
          {
          case CMD_ON:
          case CMD_HEAT:
          case CMD_COOL:
          case CMD_AUTO:
          case CMD_FAN:
          case CMD_DRY:
             if (!item->getExt())  
                                      {
                                      item->setExt(millisNZ());
                                      //relay(true);
                                      }
            return 1;

            case CMD_OFF:
             item->setExt(0);
             relay(false);
            return 1;

            default:
            debugSerial<<F("Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd

    default:
  debugSerial<<F("Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

return 0;

}

int out_relay::getChanType()
{
   return CH_PWM;
}
#endif