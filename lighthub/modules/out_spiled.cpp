#ifndef SPILED_DISABLE

#include "modules/out_spiled.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "FastLED.h"

#define NUM_LEDS 60
#define DATA_PIN 4

CRGB leds[NUM_LEDS];
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
FastLED.addLeds<TM1809, DATA_PIN, BRG>(leds, NUM_LEDS);
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_SPILed::Status()
{
return driverStatus;
}


int out_SPILed::Poll()
{
  FastLED.show();
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
  for (n=from;n<=to;n++)
  {

    switch(cmd)
    {
    CMD_ON:
    leds[n] = CRGB::White;
    break;
    CMD_OFF:
    leds[n] = CRGB::Black;
    break;
    CMD_NUM:
    switch (suffixCode)
    {

      S_POWER:
      S_VOL:
      //leds[n].setBrightness(Parameters[0]);
      break;
      S_SET:
      S_HSV:
      leds[n] = CHSV(Parameters[0],Parameters[1],Parameters[2]);
      break;
      S_RGB:
      leds[n] = CRGB(Parameters[0],Parameters[1],Parameters[2]);
      break;
  }
  }
 }
 FastLED.show();
return 1;
}

#endif
