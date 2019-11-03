#ifndef MOTOR_DISABLE

#include "modules/out_motor.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"

static int driverStatus = CST_UNKNOWN;

void out_Motor::getConfig()
{
  pinUp=item->getArg(0);
  if(pinUp<=0 || pinFeedback>=PINS_COUNT) pinUp=32;

  pinDown=item->getArg(1);
  if (pinDown<=0 || pinFeedback>=PINS_COUNT) pinDown=33;

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
digitalWrite(pinUp,LOW);
digitalWrite(pinDown,LOW);
pinMode(pinFeedback, INPUT);
item->setExt(0);
item->clearFlag(ACTION_NEEDED);
driverStatus = CST_INITIALIZED;
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

if (pinFeedback && (g_APinDescription[pinFeedback].ulPinAttribute & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG)
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

if (digitalPinHasPWM(pinUp))
    {
      int velocity = map(-dif, 0, 10, 0, 255);
      if (velocity>255) velocity=255;
      analogWrite(pinUp,velocity);
    }
else
   {
     digitalWrite(pinUp,HIGH);
   }
}
else

if (dif>POS_ERR)
{
digitalWrite(pinUp,LOW);

if (!item->getExt()) item->setExt(millis()+maxOnTime);
if (digitalPinHasPWM(pinDown))
{
  int velocity = map(dif, 0, 10, 0, 255);
  if (velocity>255) velocity=255;
  analogWrite(pinDown,velocity);
}
else
{
 digitalWrite(pinDown,HIGH);
}

}
else //Target zone
{
  digitalWrite(pinUp,LOW);
  digitalWrite(pinDown,LOW);
  item->setExt(0);
  item->clearFlag(ACTION_NEEDED);

}


return 0;
};

int out_Motor::getChanType()
{
   return CH_PWM;
}



int out_Motor::Ctrl(short cmd, short n, int * Parameters, boolean send, int suffixCode, char* subItem)
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
          if (toExecute)
          {
            if (chActive>0 && !st) item->setCmd(CMD_OFF);
            if (chActive==0 && st) item->setCmd(CMD_ON);
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
            item->setExt(millis()+maxOnTime); //Extend motor time
          }
          else    item->SendStatus(SEND_PARAMETERS | SEND_DEFFERED);

          return 1;
          //break;

case S_CMD:
      item->setCmd(cmd);
      switch (cmd)
          {
          case CMD_ON:

            /*
            if (chActive>0 && send)
                               {
                                 SendStatus(SEND_COMMAND);
                                 return 1;
                               }
            */
           //retrive stored values
           st = item->getVal();


            if (st && (st<MIN_VOLUME)) st=INIT_VOLUME;
            item->setVal(st);

            if (st)  //Stored smthng
            {
              if (send) item->SendStatus(SEND_COMMAND | SEND_PARAMETERS);
              debugSerial<<F("Restored: ")<<st<<endl;
            }
            else
            {
              debugSerial<<st<<F(": No stored values - default\n");
              // Store
              st=100;
              item->setVal(st);
              if (send) item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            }
            item->setExt(millis()+maxOnTime); //Extend motor time
            return 1;

            case CMD_OFF:
              if (send) item->SendStatus(SEND_COMMAND);
              item->setExt(millis()+maxOnTime); //Extend motor time
            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

#endif
