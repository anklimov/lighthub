#ifndef SPILED_DISABLE

#include "modules/out_spiled.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"

#ifdef ADAFRUIT_LED

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
Adafruit_NeoPixel *leds = NULL;

#else
#include "FastLED.h"
static CRGB *leds = NULL;

#endif

#define NUM_LEDS 43


void out_SPILed::getConfig()
{
  pin=item->getArg(0);
  if(pin<=0) pin=3;

  numLeds=item->getArg(1);
  if (numLeds<=0) numLeds=NUM_LEDS;

  ledsType=item->getArg(2);
  #ifdef ADAFRUIT_LED
  if (ledsType<=0) ledsType= NEO_BRG + NEO_KHZ800;
  #endif
}


int  out_SPILed::Setup()
{
abstractOut::Setup();    
getConfig();
debugSerial.println("SPI-LED Init");

if (!leds)
{
#ifdef ADAFRUIT_LED
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
leds = new Adafruit_NeoPixel(numLeds, pin, ledsType);
leds->begin();
#else
leds = new CRGB [numLeds]; //Allocate dynamic memory for LED objects
//template< CONTROLLER = TM1809, uint8_t DATA_PIN = pin, EOrder ORDER = BRG >
//FastLED.addLeds<TM1809, pin, BRG>(leds, numLeds);
FastLED.addLeds<CONTROLLER, DATA_PIN, ORDER>(leds, numLeds);
#endif
}

setStatus(CST_INITIALIZED);
return 1;
}

int  out_SPILed::Stop()
{
debugSerial.println("SPI-LED De-Init");
//FastLED.addLeds<TM1809, DATA_PIN, BRG>(leds, NUM_LEDS);
#ifdef ADAFRUIT_LED
leds->clear();
delete leds;
#else
FastLED.clear(true);
delete [] leds;
#endif
setStatus(CST_UNKNOWN);

return 1;
}



int out_SPILed::getChanType()
{

  if  ((ledsType>>4) == (ledsType>>6))
   return CH_RGB;
  else
   return CH_RGBW;
}

int out_SPILed::PixelCtrl(itemCmd cmd, char* subItem, bool show, bool authorized )
//int out_SPILed::PixelCtrl(itemCmd cmd, int from, int to, bool show)
{

int from=0, to=numLeds-1; //All LEDs on the strip by default
  // retrive LEDs range from suffix
 if (subItem)
  { //Just single LED to control todo - range
  //  debugSerial<<F("Range:")<<subItem<<endl;
    if (sscanf(subItem,"%d-%d",&from,&to) == 1) to=from;
  }
  debugSerial<<from<<F("-")<<to<<F(" cmd=")<<cmd.getCmd()<<endl;


itemCmd st(ST_RGB,CMD_VOID);

#ifdef ADAFRUIT_LED
uint32_t pixel;
#else
CRGB pixel;
#endif

  if (to>numLeds-1) to=numLeds-1;
  if (from<0) from=0;

  for (int i=from;i<=to;i++)
  {
    switch (cmd.getCmd()) {
      case CMD_ON:

      #ifdef ADAFRUIT_LED
      if (!leds->getPixelColor(i)) leds->setPixelColor(i, leds->Color(255, 255, 255));
      #else
      if (!leds[i].r && !leds[i].g &&!leds[i].b) leds[i] = CRGB::White;
      #endif
      break;

      case CMD_OFF:
       #ifdef ADAFRUIT_LED
       leds->setPixelColor(i, leds->Color(0, 0, 0));
       #else
       leds[i] = CRGB::Black;
       #endif

       break;
      default:
      st.assignFrom(cmd);

      #ifdef ADAFRUIT_LED
        leds->setPixelColor(i, leds->Color(st.param.r,st.param.g,st.param.b));
      #else
        leds[i] = CRGB(st.param.r,st.param.g,st.param.b);
      #endif
   }
  } //for



     if (show)
          {
          #ifdef ADAFRUIT_LED
          leds->show();
          #else
          FastLED.show();
          #endif
          debugSerial<<F("Show")<<endl;
          }
return 1;

}

/*
int out_SPILed::Ctrl(itemCmd cmd,   char* subItem)
{
int chActive = item->isActive();
bool toExecute = (chActive>0);
int suffixCode = cmd.getSuffix();
itemCmd st(ST_HSV);
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

int from=0, to=numLeds-1; //All LEDs on the strip by default
// retrive LEDs range from suffix
if (subItem)
{ //Just single LED to control todo - range
//  debugSerial<<F("Range:")<<subItem<<endl;
  if (sscanf(subItem,"%d-%d",&from,&to) == 1) to=from;
}
debugSerial<<from<<F("-")<<to<<F(" cmd=")<<cmd.getCmd()<<endl;


switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
case S_HSV:
          //st.Int(item->getVal()); //Restore old params
          st.loadItem(item);
          st.assignFrom(cmd);

          PixelCtrl(st,from,to,toExecute);

          if (!subItem) //Whole strip
          {
          //item->setVal(st.getInt()); //Store
          st.saveItem(item);
          if (!suffixCode)
          {
            if (chActive>0 && !st.getPercents()) item->setCmd(CMD_OFF);
            if (chActive==0 && st.getPercents()) item->setCmd(CMD_ON);
            item->SendStatus(SEND_COMMAND | FLAG_PARAMETERS | FLAG_SEND_DEFFERED);
          }
          else    item->SendStatus(FLAG_PARAMETERS | FLAG_SEND_DEFFERED);
          }
          return 1;
          //break;

case S_CMD:
      item->setCmd(cmd.getCmd());
      switch (cmd.getCmd())
          {
          case CMD_ON:
           //retrive stored values
           st.loadItem(item);

           if (subItem) // LED range, not whole strip
                      PixelCtrl(st.Cmd(CMD_ON),from,to);
           else  //whole strip
            {
            if (st.param.aslong && (st.param.v<MIN_VOLUME) ) st.Percents(INIT_VOLUME);
            //item->setVal(st.getInt());
            st.saveItem(item);

            if (st.getInt() )  //Stored smthng
            {
              item->SendStatus(SEND_COMMAND | FLAG_PARAMETERS);
              debugSerial<<F("Restored: ")<<st.param.h<<F(",")<<st.param.s<<F(",")<<st.param.v<<endl;
            }
            else
            {
              debugSerial<<st.param.aslong<<F(": No stored values - default\n");
              st.setDefault();
              //st.param.hsv_flag=1; ///tyta
              // Store
              //item->setVal(st.getInt());
              st.saveItem(item);
              item->SendStatus(SEND_COMMAND | FLAG_PARAMETERS );
            }

            PixelCtrl(st,from,to);
            }

            return 1;

            case CMD_OFF:
            if (subItem) // LED range, not whole strip
            PixelCtrl(st.Cmd(CMD_OFF),from,to);
            else
            {
              st.Percents(0);
              PixelCtrl(st,from,to);
              item->SendStatus(SEND_COMMAND);
              //  if (send) item->publishTopic(item->itemArr->name,"OFF","/cmd");
            }
            return 1;

} //switch cmd

break;
} //switch suffix
debugSerial<<F("Unknown cmd")<<endl;
return 0;
}

*/

#endif
