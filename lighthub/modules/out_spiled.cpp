#ifndef SPILED_DISABLE

#include "modules/out_spiled.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "FastLED.h"
#include "item.h"

#define NUM_LEDS 15
#define DATA_PIN 4

static CRGB leds[NUM_LEDS];
static int driverStatus = CST_UNKNOWN;

int  out_SPILed::Setup()
{
Serial.println("SPI-LED Init");
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
return 1;
}

int  out_SPILed::Status()
{
return driverStatus;
}

int out_SPILed::isActive()
{

}

int out_SPILed::Poll()
{
  //FastLED.show();
return 1;
};

int out_SPILed::Ctrl(short cmd, short n, int * Parameters, boolean send, int suffixCode, char* subItem)
{
int from=0, to=NUM_LEDS-1;

if (subItem)
{ //Just single LED to control
  from=atoi(subItem);
  to=from;
}
debugSerial<<from<<F("-")<<to<<F(" cmd=")<<cmd<<endl;
  for (int i=from;i<=to;i++)
  {
//    debugSerial<<F(".");
    switch(cmd)
    {
    case CMD_ON:
    debugSerial<<F("Ch: ")<<i<<F(" White")<<endl;
    leds[i] = CRGB::White;
    break;
    case CMD_OFF:
    debugSerial<<F("Ch: ")<<i<<F(" Black")<<endl;
    leds[i] = CRGB::Black;
    break;
    case CMD_NUM:
    switch (suffixCode)
    {

  //    case S_POWER:
  //    case S_VOL:
      //leds[n].setBrightness(Parameters[0]);
  //    break;
      case S_SET:
      case S_HSV:
      debugSerial<<F("HSV: ")<<i<<F(" :")<<Parameters[0]<<Parameters[1]<<Parameters[2]<<endl;
      leds[i] = CHSV(Parameters[0],Parameters[1],Parameters[2]);
      break;
      case S_RGB:
      debugSerial<<F("RGB: ")<<i<<F(" :")<<Parameters[0]<<Parameters[1]<<Parameters[2]<<endl;
      leds[i] = CRGB(Parameters[0],Parameters[1],Parameters[2]);
      break;
  }
  }
 }
 FastLED.show();
 debugSerial<<F("Show ")<<endl;
return 1;
}

#endif
