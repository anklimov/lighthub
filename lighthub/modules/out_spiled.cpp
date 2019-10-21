#ifndef SPILED_DISABLE

#include "modules/out_spiled.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "FastLED.h"
#include "item.h"

#define NUM_LEDS 43
#define DATA_PIN 4

//static CRGB leds[NUM_LEDS];
static CRGB *leds = NULL;
static int driverStatus = CST_UNKNOWN;

int  out_SPILed::Setup()
{
Serial.println("SPI-LED Init");

leds = new CRGB [NUM_LEDS]; //Allocate dynamic memory for LED objects

FastLED.addLeds<TM1809, DATA_PIN, BRG>(leds, NUM_LEDS);
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_SPILed::Stop()
{
Serial.println("SPI-LED De-Init");
//FastLED.addLeds<TM1809, DATA_PIN, BRG>(leds, NUM_LEDS);
FastLED.clear(true);
driverStatus = CST_UNKNOWN;
delete [] leds;
return 1;
}

int  out_SPILed::Status()
{
return driverStatus;
}

int out_SPILed::isActive()
{
CHstore st;
st.aslong = item->getVal(); //Restore old params
debugSerial<< F(" val:")<<st.v<<endl;
return st.v;
}

int out_SPILed::Poll()
{
return 1;
};

int out_SPILed::getChanType()
{
  return CH_RGB;
}

int out_SPILed::PixelCtrl(CHstore *st, short cmd, int from, int to, bool show, bool rgb)
{
  //debugSerial<<F("cmd: ")<<cmd<<endl;
  CRGB pixel;
  if (!rgb)
  {
  int Saturation = map(st->s, 0, 100, 0, 255);
  int Value      = map(st->v, 0, 100, 0, 255);
  int Hue        = map(st->h, 0, 365, 0, 255);
  pixel      = CHSV(Hue, Saturation, Value);
  debugSerial<<F("hsv: ")<<st->h<<F(",")<<st->s<<F(",")<<st->v<<endl;
  }
  if (to>NUM_LEDS-1) to=NUM_LEDS-1;
  if (from<0) from=0;

  for (int i=from;i<=to;i++)
  {
    switch (cmd) {
      case CMD_ON:
      if (!leds[i].r && !leds[i].g &&!leds[i].b) leds[i] = CRGB::White;
      // FastLED.show();
      break;
      case CMD_OFF:
       //pixel     = leds[i];
       leds[i] = CRGB::Black;
       //FastLED.show();
       //leds[i] = pixel;
       break;
      default:
    if (rgb)
    {
      leds[i] = CRGB(st->r,st->g,st->b);
    }
    else
    {
      leds[i] = pixel;
    }

   }
  } //for
     if (show)
          {
          FastLED.show();
          debugSerial<<F("Show")<<endl;
          }
return 1;
}

int out_SPILed::Ctrl(short cmd, short n, int * Parameters, boolean send, int suffixCode, char* subItem)
{
int chActive = item->isActive();
bool toExecute = (chActive>0);
CHstore st;
if (cmd>0 && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

int from=0, to=NUM_LEDS-1; //All LEDs on the strip by default
// retrive LEDs range from suffix
if (subItem)
{ //Just single LED to control todo - range
//  debugSerial<<F("Range:")<<subItem<<endl;
  if (sscanf(subItem,"%d-%d",&from,&to) == 1) to=from;
}
debugSerial<<from<<F("-")<<to<<F(" cmd=")<<cmd<<endl;


switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
case S_HSV:
          st.aslong = item->getVal(); //Restore old params
          switch (n) //How many parameters are passed?
          {
            case 1:
            st.v = Parameters[0]; //Volume only
            if (!st.hsv_flag)
              {
                st.h = 0; //Filling by default
                st.s = 0;
                st.hsv_flag = 1; // Mark stored vals as HSV
              }
            break;
            case 2: // Just hue and saturation
              st.h = Parameters[0];
              st.s = Parameters[1];
              st.hsv_flag = 1;
            break;
            case 3: //complete triplet
            st.h = Parameters[0];
            st.s = Parameters[1];
            st.v = Parameters[2];
            st.hsv_flag = 1;
          }
          PixelCtrl(&st,0,from,to,toExecute);

          if (!subItem) //Whole strip
          {
          item->setVal(st.aslong); //Store
          if (toExecute)
          {
            if (chActive>0 && !st.v) item->setCmd(CMD_OFF);
            if (chActive==0 && st.v) item->setCmd(CMD_ON);
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
          }
          else    item->SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
          }
          return 1;
          //break;
case S_RGB:
  st.r = Parameters[0];
  st.g = Parameters[1];
  st.b = Parameters[2];
  st.w = 0;
  st.hsv_flag = 0;
PixelCtrl(&st,0,from,to,toExecute,true);
//item->setVal(st.aslong); //Store
if (toExecute)
{
  if (chActive>0 && !st.aslong) item->setCmd(CMD_OFF);
  if (chActive==0 && st.aslong) item->setCmd(CMD_ON);
  item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
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
           st.aslong = item->getVal();

           if (subItem) // LED range, not whole strip
                      PixelCtrl(&st,CMD_ON,from,to);
           else  //whole strip
            {
            if (st.aslong && (st.v<MIN_VOLUME)) st.v=INIT_VOLUME;
            item->setVal(st.aslong);

            if (st.aslong )  //Stored smthng
            {
              if (send) item->SendStatus(SEND_COMMAND | SEND_PARAMETERS);
              debugSerial<<F("Restored: ")<<st.h<<F(",")<<st.s<<F(",")<<st.v<<endl;
            }
            else
            {
              debugSerial<<st.aslong<<F(": No stored values - default\n");
              st.h = 100;
              st.s = 0;
              st.v = 100;
              st.hsv_flag=1;
              // Store
              item->setVal(st.aslong);
              if (send) item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            }

            PixelCtrl(&st,0,from,to);
            }

            return 1;

            case CMD_OFF:
            if (subItem) // LED range, not whole strip
            PixelCtrl(&st,CMD_OFF,from,to);
            else
            {
              st.v=0;
              PixelCtrl(&st,0,from,to);
              if (send) item->SendStatus(SEND_COMMAND);
              //  if (send) item->publishTopic(item->itemArr->name,"OFF","/cmd");
            }
            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

#endif
