#ifndef RELAY_DISABLE
#include "modules/out_relay.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "dmx.h"

static int driverStatus = CST_UNKNOWN;

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
if (!item ) return 0;
pinMode(pin, OUTPUT);
item->setExt(0);
digitalWrite(pin,INACTIVE);
//if (item->getCmd()) item->setFlag(SEND_COMMAND);
//if (item->itemVal)  item->setFlag(SEND_PARAMETERS);
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_relay::Stop()
{
debugSerial<<F("Relay-Out #")<<pin<<F(" stop")<<endl;
pinMode(pin, INPUT);
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_relay::Status()
{
return driverStatus;
}

const char action_P[] PROGMEM = "/action";
const char cooling_P[] PROGMEM = "cooling";
const char heating_P[] PROGMEM = "heating";
const char drying_P[] PROGMEM = "drying";
const char idle_P[] PROGMEM = "idle";
const char fan_P[] PROGMEM = "fan";
const char off_P[] PROGMEM = "off";


void  out_relay::relay(bool state)
{
char subtopic[10];
char val[10];  
digitalWrite(pin,(state)?ACTIVE:INACTIVE);
if (period<1000) return;
debugSerial<<F("Out ")<<pin<<F(" is ")<<(state)<<endl;

strcpy_P(subtopic,action_P);
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

publishTopic(item->itemArr->name,val,subtopic);
}

bool getPinVal(uint8_t pin)
{return (0!=(*portOutputRegister( digitalPinToPort(pin) ) & digitalPinToBitMask(pin)));}

int out_relay::Poll(short cause)
{
  if (!item) return 0;

  itemCmd st;
  st.loadItem(item);
  int val = st.getPercents255();
short cmd = st.getCmd();
uint32_t timer = item->getExt(); 
  
   if (timer && isTimeOver(timer,millis(),period))
    {
      item->setExt(millisNZ());
      if (val && (getPinVal(pin) == INACTIVE)) relay(true);
    }   

    else if (timer && (getPinVal(pin) == ACTIVE) && isTimeOver(timer,millis(),period*val/255))
    {
      relay(false);
      if (!item->isActive()) item->setExt(0);
    }  


    return 0;
};


int out_relay::Ctrl(itemCmd cmd, char* subItem, bool toExecute)
{
debugSerial<<F("relayCtr: ");
cmd.debugOut();
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
                                    relay(true);
                                    }
            }
          else        
                    {     
                    item->setExt(0);
                    relay(false);              
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
                                      relay(true);
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