#ifndef MOTOR_DISABLE

#include "modules/out_motor.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"

static int driverStatus = CST_UNKNOWN;

void out_Motor::getConfig()
{
  inverted=false;
  pinUp=item->getArg(0);
  if (pinUp<0)
            {
              pinUp=-pinUp;
              inverted=true;
            }
  if(pinUp<=0 || pinUp>=PINS_COUNT) pinUp=32;

  pinDown=item->getArg(1);
  if (pinDown<0)
            {
              pinDown=-pinDown;
              inverted=true;
            }
 
  if (pinDown<=0 || pinDown>=PINS_COUNT) pinDown=33;

  pinFeedback=item->getArg(2);
  if (pinFeedback<0 || pinFeedback>=PINS_COUNT) pinFeedback=0;

  feedbackOpen=item->getArg(3);
  if (feedbackOpen<=0 || feedbackOpen>1024) feedbackOpen=0;

  feedbackClosed=item->getArg(4);
  if (feedbackClosed<0 || feedbackClosed>1024) feedbackClosed=1024;

  maxOnTime=item->getArg(5);
  if (maxOnTime<=0) maxOnTime=10000;
  }

#define ACTIVE (inverted)?LOW:HIGH
#define INACTIVE (inverted)?HIGH:LOW

int  out_Motor::Setup()
{
abstractOut::Setup();    
getConfig();
debugSerial.println("Motor: Init");
pinMode(pinUp,OUTPUT);
pinMode(pinDown,OUTPUT);


digitalWrite(pinUp,INACTIVE);
digitalWrite(pinDown,INACTIVE);


pinMode(pinFeedback, INPUT);
item->setExt(0);
item->clearFlag(FLAG_ACTION_NEEDED);
item->clearFlag(FLAG_ACTION_IN_PROCESS);
driverStatus = CST_INITIALIZED;
motorQuote = MOTOR_QUOTE;
return 1;
}

int  out_Motor::Stop()
{
debugSerial.println("Motor: De-Init");
digitalWrite(pinUp,INACTIVE);
digitalWrite(pinDown,INACTIVE);

item->setExt(0);
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_Motor::Status()
{
return driverStatus;
}
/*
int out_Motor::isActive()
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
int out_Motor::Poll(short cause)
{
if (cause==POLLING_SLOW) return 0;  
int curPos = -1;
int targetPos = -1;
int dif;
if (!item->getFlag(FLAG_ACTION_NEEDED)) return 0;

if (!item->getFlag(FLAG_ACTION_IN_PROCESS))
    {
    if (motorQuote)
      {
        item->setFlag(FLAG_ACTION_IN_PROCESS);
        motorQuote--;
      }
    else return 0;
    }

uint32_t motorOfftime = item->getExt();
itemCmd st;
st.loadItem(item);
targetPos = st.getPercents255();// item->getVal();

switch (item->getCmd())
{ 
  case CMD_ON:
  case CMD_XON:
  //if (targetPos<15) targetPos=100;
  
  break;

  case CMD_OFF:
  case CMD_HALT:
  targetPos = 0;
  break;
}
int fb=-1;
if (pinFeedback && isAnalogPin(pinFeedback))
{
  
curPos=map((fb=analogRead(pinFeedback)),feedbackClosed,feedbackOpen,0,255);
if (curPos<0) curPos=0;
if (curPos>255) curPos=255;
}

if (motorOfftime && isTimeOver(motorOfftime,millis(),maxOnTime))
  {dif = 0; debugSerial<<F("Motor: timeout")<<endl;}
else if (curPos>=0)
  dif=targetPos-curPos;
else
  dif=targetPos-255/2; // Have No feedback

//debugSerial<<F("Motor: in:")<<pinFeedback<<F(" Val:")<<fb<<F("/")<<curPos<<F("->")<<targetPos<<F(" delta:")<<dif<<endl;

if (dif<-POS_ERR)
{

  digitalWrite(pinDown,INACTIVE);
  if (!item->getExt())item->setExt(millisNZ());

  //
  //PINS_COUNT
  //PIN_ATTR_ANALOG
  //  uint32_t attr = g_APinDescription[pinUp].ulPinAttribute;
  //  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM) ;
#ifndef ESP32
if (digitalPinHasPWM(pinUp))
    {
//Serial.println("pinUP PWM");
  int velocity; 
  if (inverted) velocity = map(-dif, 0, 255/10, 255, 0);
  else          velocity = map(-dif, 0, 255/10, 0, 255);

  velocity = constrain (velocity, MIN_PWM, 255);

      analogWrite(pinUp,velocity);
    }

else if (digitalPinHasPWM(pinDown))
        {
  //        Serial.println("pinDown PWM fallback")
          digitalWrite(pinUp,ACTIVE);

          int velocity;
          if (inverted)
            velocity = map(-dif, 0, 255/10, 0, 255);
          else velocity = map(-dif, 0, 255/10, 255, 0);

          velocity = constrain (velocity, MIN_PWM, 255);
          analogWrite(pinDown,velocity);
        }
else
#endif
   {
  //   Serial.print(pinUp);
  //   Serial.println(" pinUP noPWM");
     digitalWrite(pinUp,ACTIVE);
   }
}
else

if (dif>POS_ERR)
{
digitalWrite(pinUp,INACTIVE);

if (!item->getExt()) item->setExt(millisNZ());
#ifndef ESP32
if (digitalPinHasPWM(pinDown))
{
  //Serial.println("pinDown PWM");
  int velocity; 
  if (inverted) velocity = map(dif, 0, 255/5, 255, 0);
  else          velocity = map(dif, 0, 255/5, 0, 255);

  velocity = constrain (velocity, MIN_PWM, 255);

  analogWrite(pinDown,velocity);
}
else
if (digitalPinHasPWM(pinUp))
{
  //Serial.println("pinUP PWM fallback");
  digitalWrite(pinDown,ACTIVE);
  
  int velocity; 
  if (inverted) velocity = map(dif, 0, 255/10, 0, 255);
  else          velocity = map(dif, 0, 255/10, 255, 0); 

  if (velocity>255) velocity=255;
  if (velocity<0) velocity=0;
  analogWrite(pinUp,velocity);
}
else
#endif
{
  //Serial.print(pinDown);
  //Serial.println(" pinDown noPWM");
 digitalWrite(pinDown,ACTIVE);
}

}
else //Target zone
{ debugSerial.println("Motor: Target");
  digitalWrite(pinUp,INACTIVE);
  digitalWrite(pinDown,INACTIVE);
  item->setExt(0);
  item->clearFlag(FLAG_ACTION_NEEDED);
  item->clearFlag(FLAG_ACTION_IN_PROCESS);
  motorQuote++;
}


return 0;
};

int out_Motor::getChanType()
{
   return CH_PWM;
}



int out_Motor::Ctrl(itemCmd cmd,   char* subItem , bool toExecute,bool authorized)
{
int suffixCode = cmd.getSuffix();
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

item->setFlag(FLAG_ACTION_NEEDED);

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
debugSerial<<F("Motor: Forced execution");
case S_SET:
//case S_ESET:
          if (!cmd.isValue()) return 0;
          if (item->getExt()) item->setExt(millisNZ()); //Extend motor time
          return 1;
          //break;

case S_CMD:
      switch (cmd.getCmd())
          {
          case CMD_ON:
            if (item->getExt()) item->setExt(millisNZ()); //Extend motor time
            return 1;

            case CMD_OFF:
              if (item->getExt()) item->setExt(millisNZ()); //Extend motor time
            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

#endif
