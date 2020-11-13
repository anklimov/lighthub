//#ifndef DMX_DISABLE
#ifdef XXXX
#include "modules/out_dmx.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"

static int driverStatus = CST_UNKNOWN;




int  out_dmx::Setup()
{

debugSerial<<F("DMX-Out Init")<<endl;
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_SPILed::Stop()
{
debugSerial<<F("DMX-Out stop")<<endl;
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_SPILed::Status()
{
return driverStatus;
}

int out_SPILed::isActive()
{
itemStore st;
st.aslong = item->getVal(); //Restore old params
debugSerial<< F(" val:")<<st.v<<endl;
return st.v;
}

int out_SPILed::Poll(short cause)
{
return 0;
};

int out_SPILed::getChanType()
{
  if  ((ledsType>>4) == (ledsType>>6))
   return CH_RGB;
  else
   return CH_RGBW;
}

int out_SPILed::PixelCtrl(itemCmd cmd, int from, int to, bool show)
{
itemCmd st(ST_RGB);

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

int out_SPILed::Ctrl(itemCmd cmd,  int suffixCode, char* subItem)
{
int chActive = item->isActive();
bool toExecute = (chActive>0);
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
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
          }
          else    item->SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
          }
          return 1;
          //break;
case S_HUE:
     st.setH(uint16_t);
     break;

case S_SAT:
     st.setS(uint8_t);
     break;

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
            if (st.param.aslong && (st.param.v<MIN_VOLUME) /* && send */) st.Percents(INIT_VOLUME);
            //item->setVal(st.getInt());
            st.saveItem(item);

            if (st.getInt() )  //Stored smthng
            {
              item->SendStatus(SEND_COMMAND | SEND_PARAMETERS);
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
              item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            }

            PixelCtrl(st,from,to);
            }

            return 1;

            case CMD_OFF:
            if (subItem) // LED range, not whole strip
            PixelCtrl(st.Cmd(CMD_OFF));
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

#endif
