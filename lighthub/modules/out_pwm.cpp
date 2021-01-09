#ifndef PWM_DISABLE
#include "modules/out_pwm.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "dmx.h"

static int driverStatus = CST_UNKNOWN;

#if defined(ARDUINO_ARCH_ESP32)
void analogWrite(int pin, int val)
{
  //TBD
}
#endif

int  out_pwm::Setup()
{
debugSerial<<F("PWM-Out Init")<<endl;
if (!item || iaddr) return 0;

switch (getChanType())
{
  case CH_RGBW:
   pinMode(getChannelAddr(3), OUTPUT);
  case CH_RGB:
   pinMode(getChannelAddr(0), OUTPUT);
   pinMode(getChannelAddr(1), OUTPUT);
   pinMode(getChannelAddr(2), OUTPUT);
   break;
  default:
   pinMode(iaddr, OUTPUT);
}
 //timer 0 for pin 13 and 4
 //timer 1 for pin 12 and 11
 //timer 2 for pin 10 and 9
 //timer 3 for pin 5 and 3 and 2
 //timer 4 for pin 8 and 7 and 6
 //prescaler = 1 ---> PWM frequency is 31000 Hz
 //prescaler = 2 ---> PWM frequency is 4000 Hz
 //prescaler = 3 ---> PWM frequency is 490 Hz (default value)
 //prescaler = 4 ---> PWM frequency is 120 Hz
 //prescaler = 5 ---> PWM frequency is 30 Hz
 //prescaler = 6 ---> PWM frequency is <20 Hz
#if defined(__AVR_ATmega2560__)
 int tval = 7;             // this is 111 in binary and is used as an eraser
 TCCR4B &= ~tval;   // this operation (AND plus NOT),  set the three bits in TCCR2B to 0
 TCCR3B &= ~tval;
 tval = 2;
 TCCR4B |= tval;
 TCCR3B |= tval;
#endif

driverStatus = CST_INITIALIZED;
return 1;
}

int  out_pwm::Stop()
{
debugSerial<<F("PWM-Out stop")<<endl;

switch (getChanType())
{
  case CH_RGBW:
   pinMode(getChannelAddr(3), INPUT);
  case CH_RGB:
   pinMode(getChannelAddr(0), INPUT);
   pinMode(getChannelAddr(1), INPUT);
   pinMode(getChannelAddr(2), INPUT);
   break;
  default:
   pinMode(iaddr, INPUT);
}
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_pwm::Status()
{
return driverStatus;
}

int out_pwm::isActive()
{
itemArgStore st;
st.aslong = item->getVal(); //Restore old params
debugSerial<< F(" val:")<<st.v<<endl;
return st.v;
}

int out_pwm::Poll(short cause)
{
return 0;
};

int out_pwm::getChanType()
{
  if (item)
  {
  switch (numArgs)
  {
    case 3:
    debugSerial<<F("RGB PWM")<<endl;
      return CH_RGB;

    case 4:
    debugSerial<<F("RGBW PWM")<<endl;
      return CH_RGBW;
    default:
    debugSerial<<item->itemType<<F(" PWM")<<endl;
      return item->itemType;
  }
 }
  return 0;
}

int out_pwm::PixelCtrl(itemCmd cmd, char* subItem, bool show)
{
if (!item || !iaddr || !show) return 0;

bool   inverse  = (item->getArg()<0);
short  cType    = getChanType();
uint8_t storageType;

switch (cmd.getCmd()){
  case CMD_OFF:
    cmd.Percents(0);
  break;
}


 switch (cType)
  {
    case CH_PWM:
          { short k;
            analogWrite(iaddr, k=cmd.getPercents255(inverse));
            debugSerial<<F("Pin:")<<iaddr<<F("=")<<k<<endl;
            return 1;
          }
    case CH_RGB:
     storageType=ST_RGB;
     break;
    case CH_RGBW:
     storageType=ST_RGBW;
     break;
    default:
     storageType=ST_PERCENTS;
   }

itemCmd st(storageType,CMD_VOID);

st.assignFrom(cmd);

switch (cType)
{
  case CH_RGBW:
  analogWrite(getChannelAddr(3), st.param.w);
  case CH_RGB:
  analogWrite(iaddr, st.param.r);
  analogWrite(getChannelAddr(1), st.param.g);
  analogWrite(getChannelAddr(2), st.param.b);
  break;
  default: ;
}

return 1;
}
#endif