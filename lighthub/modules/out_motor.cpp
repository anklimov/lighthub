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
  pinUp=item->getArg(0);
  if(pinUp<=0 || pinUp>=PINS_COUNT) pinUp=32;

  pinDown=item->getArg(1);
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


int  out_Motor::Setup()
{
getConfig();
Serial.println("Motor Init");
pinMode(pinUp,OUTPUT);
pinMode(pinDown,OUTPUT);
digitalWrite(pinUp,LOW);
digitalWrite(pinDown,LOW);
pinMode(pinFeedback, INPUT);
item->setExt(0);
item->clearFlag(ACTION_NEEDED);
item->clearFlag(ACTION_IN_PROCESS);
driverStatus = CST_INITIALIZED;
motorQuote = MOTOR_QUOTE;
return 1;
}

int  out_Motor::Stop()
{
Serial.println("Motor De-Init");
digitalWrite(pinUp,LOW);
digitalWrite(pinDown,LOW);
item->setExt(0);
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_Motor::Status()
{
return driverStatus;
}

int out_Motor::isActive()
{
return item->getVal();
}

int out_Motor::Poll(short cause)
{
int curPos = -1;
int targetPos = -1;
int dif;
if (!item->getFlag(ACTION_NEEDED)) return 0;

if (!item->getFlag(ACTION_IN_PROCESS))
    {
    if (motorQuote)
      {
        item->setFlag(ACTION_IN_PROCESS);
        motorQuote--;
      }
    else return 0;
    }

uint32_t motorOfftime = item->getExt();

switch (item->getCmd())
{
  case CMD_ON:
  case CMD_XON:
  targetPos = item->getVal();
  break;

  case CMD_OFF:
  case CMD_HALT:
  targetPos = 0;
  break;
}

if (pinFeedback && isAnalogPin(pinFeedback))
{
curPos=map(analogRead(pinFeedback),feedbackClosed,feedbackOpen,0,100);
if (curPos<0) curPos=0;
if (curPos>100) curPos=100;
}

if (motorOfftime && motorOfftime<millis()) //Time over
  {dif = 0; debugSerial<<F("Motor timeout")<<endl;}
else if (curPos>=0)
  dif=targetPos-curPos;
else
  dif=targetPos-50; // Have No feedback



if (dif<-POS_ERR)
{

  digitalWrite(pinDown,LOW);
  if (!item->getExt())item->setExt(millis()+maxOnTime);

  //
  //PINS_COUNT
  //PIN_ATTR_ANALOG
  //  uint32_t attr = g_APinDescription[pinUp].ulPinAttribute;
  //  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM) ;
#ifndef ESP32
if (digitalPinHasPWM(pinUp))
    {
//Serial.println("pinUP PWM");
      int velocity = map(-dif, 0, 10, 0, 255);
      if (velocity>255) velocity=255;
      if (velocity<0) velocity=0;
      analogWrite(pinUp,velocity);
    }

else if (digitalPinHasPWM(pinDown))
        {
  //        Serial.println("pinDown PWM fallback");
          digitalWrite(pinUp,HIGH);
          int velocity = map(-dif, 0, 10, 255, 0);
          if (velocity>255) velocity=255;
          if (velocity<0) velocity=0;
          analogWrite(pinDown,velocity);
        }
else
#endif
   {
  //   Serial.print(pinUp);
  //   Serial.println(" pinUP noPWM");
     digitalWrite(pinUp,HIGH);
   }
}
else

if (dif>POS_ERR)
{
digitalWrite(pinUp,LOW);

if (!item->getExt()) item->setExt(millis()+maxOnTime);
#ifndef ESP32
if (digitalPinHasPWM(pinDown))
{
  //Serial.println("pinDown PWM");
  int velocity = map(dif, 0, 10, 0, 255);
  if (velocity>255) velocity=255;
  if (velocity<0) velocity=0;
  analogWrite(pinDown,velocity);
}
else
if (digitalPinHasPWM(pinUp))
{
  //Serial.println("pinUP PWM fallback");
  digitalWrite(pinDown,HIGH);
  int velocity = map(dif, 0, 10, 255, 0);
  if (velocity>255) velocity=255;
  if (velocity<0) velocity=0;
  analogWrite(pinUp,velocity);
}
else
#endif
{
  //Serial.print(pinDown);
  //Serial.println(" pinDown noPWM");
 digitalWrite(pinDown,HIGH);
}

}
else //Target zone
{ Serial.println("Target");
  digitalWrite(pinUp,LOW);
  digitalWrite(pinDown,LOW);
  item->setExt(0);
  item->clearFlag(ACTION_NEEDED);
  item->clearFlag(ACTION_IN_PROCESS);
  motorQuote++;
}


return 0;
};

int out_Motor::getChanType()
{
   return CH_PWM;
}



int out_Motor::Ctrl(short cmd, short n, int * Parameters,  int suffixCode, char* subItem)
{
int chActive = item->isActive();
bool toExecute = (chActive>0);
long st;
if (cmd>0 && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

item->setFlag(ACTION_NEEDED);

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
debugSerial<<F("Forced execution");
case S_SET:
          if (!Parameters || n==0) return 0;
          item->setVal(st=Parameters[0]); //Store
          if (!suffixCode)
          {
            if (chActive>0 && !st) item->setCmd(CMD_OFF);
            if (chActive==0 && st) item->setCmd(CMD_ON);
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
            if (item->getExt()) item->setExt(millis()+maxOnTime); //Extend motor time
          }
          else    item->SendStatus(SEND_PARAMETERS | SEND_DEFFERED);

          return 1;
          //break;

case S_CMD:
      item->setCmd(cmd);
      switch (cmd)
          {
          case CMD_ON:
           //retrive stored values
           st = item->getVal();


            if (st && (st<MIN_VOLUME) /* && send */) st=INIT_VOLUME;
            item->setVal(st);

            if (st)  //Stored smthng
            {
              item->SendStatus(SEND_COMMAND | SEND_PARAMETERS);
              debugSerial<<F("Restored: ")<<st<<endl;
            }
            else
            {
              debugSerial<<st<<F(": No stored values - default\n");
              // Store
              st=100;
              item->setVal(st);
              item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            }
            if (item->getExt()) item->setExt(millis()+maxOnTime); //Extend motor time
            return 1;

            case CMD_OFF:
              item->SendStatus(SEND_COMMAND);
              if (item->getExt()) item->setExt(millis()+maxOnTime); //Extend motor time
            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

#endif
