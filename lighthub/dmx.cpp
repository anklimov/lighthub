/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Homepage: http://lazyhome.ru
GIT:      https://github.com/anklimov/lighthub
e-mail    anklimov@gmail.com

*/
#include "dmx.h"
//#include <DmxSimple.h>
//#include <DMXSerial.h>
#include "options.h"
#include "item.h"
#include "main.h"

#ifdef _dmxin
#if defined(ARDUINO_ARCH_AVR)
#include <DMXSerial.h>
#endif
#endif

#if defined(ESP8266) ||  defined(ARDUINO_ARCH_ESP32)
#ifndef DMX_DISABLE
DMXESPSerial dmxout;
#endif
#endif

uint8_t * DMXin = NULL;

#ifdef DMX_SMOOTH
volatile uint8_t * DMXinterimBuf = NULL;
volatile uint16_t DMXOUT_Channels=0;
volatile uint32_t checkTimestamp=0L;
#endif

#if defined(_dmxin)
volatile uint32_t D_State=0;
volatile unsigned long D_checkT=0;
uint8_t DMXINChannels=0;
#endif

#ifdef _artnet
#include <Artnet.h>
Artnet *artnet = NULL;
uint16_t artnetMinCh=1;
uint16_t artnetMaxCh=512;
#endif


extern aJsonObject *items;
extern aJsonObject *dmxArr;



itemCmd rgb2hsv(itemCmd in)
{
    itemCmd         out;
    out.setArgType(ST_HSV255);

    double      min, max, delta;
    
    double inr=in.param.r/255;
    double ing=in.param.g/255;
    double inb=in.param.b/255;
    double inw=in.param.w/255; 

    min = inr < ing ? inr : ing;
    min = min  < inb ? min  : inb;

    max = inr > ing ? inr : ing;
    max = max  > inb ? max  : inb;
    max = max  > inw ? max  : inw;

    out.param.v = max*255;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.param.s = 0;
        out.param.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.param.s = (delta / max)*100;                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.param.s = 0;
        out.param.h = 0;                            // its now undefined
        return out;
    }
    double outh;
    if( inr >= max )                           // > is bogus, just keeps compilor happy
        outh = ( ing - inb ) / delta;        // between yellow & magenta
    else
    if( ing >= max )
        outh = 2.0 + ( inb - inr ) / delta;  // between cyan & yellow
    else
        outh = 4.0 + ( inr - ing ) / delta;  // between magenta & cyan

    outh *= 60.0;                              // degrees

    if( outh < 0.0 )
        outh += 360.0;
    out.param.h=outh;
    return out;
}


int itemCtrl2(char* name,int r,int g, int b, int w)
{
  if (!items) return 0;
  aJsonObject *itemArr= aJson.getObjectItem(items, name);

       if (itemArr && (itemArr->type==aJson_Array))
       {
        short itemtype = aJson.getArrayItem(itemArr,0)->valueint;
        short itemaddr = aJson.getArrayItem(itemArr,1)->valueint;
       switch (itemtype){
#ifdef _dmxout
       case CH_DIMMER: //Dimmed light

       DmxWrite(itemaddr, w);
       break;


       case CH_RGBW: //Colour RGBW
        DmxWrite(itemaddr+3, w);

       case CH_RGB: // RGB
      {
        DmxWrite(itemaddr,   r);
        DmxWrite(itemaddr+1, g);
        DmxWrite(itemaddr+2, b);

         break; }
#endif
        case CH_GROUP: //Group
        aJsonObject *groupArr= aJson.getArrayItem(itemArr, 1);
        if (groupArr && (groupArr->type==aJson_Array))
        { aJsonObject *i =groupArr->child;
          while (i)
            {
            if (i->type == aJson_String) itemCtrl2(i->valuestring,r,g,b,w);
              i=i->next;}
        }
       } //itemtype
     //  break;
          } //if have correct array
return 1;
  }


void DMXImmediateUpdate(short tch,short r, short g, short b, short w) {
//Here only safe re-interable code for quick passthrow between DMX IN and DMX OUT
       if (dmxArr && (dmxArr->type==aJson_Array))

        {
        aJsonObject *DMXch =   aJson.getArrayItem(dmxArr,tch);
        char* itemname = NULL;
        if (DMXch->type == aJson_String) itemname=DMXch->valuestring;
        if (itemname) itemCtrl2(itemname,r,g,b,w);
        }
}

void DMXSemiImmediateUpdate(short tch,short r, short g, short b, short w)
{
//Here any code for  passthrow between DMX IN and DMX OUT in idle state
       if (dmxArr && (dmxArr->type==aJson_Array))

        {
        aJsonObject *DMXch =   aJson.getArrayItem(dmxArr,tch);
        char* itemname = NULL;
        if (DMXch->type == aJson_String) itemname=DMXch->valuestring;
         if (itemname)
          {
            Item it(itemname);
            if (!r && !g && !b && !w) it.Ctrl(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD));
               else 
                  {
                    /*
                  CRGB rgb;
                  rgb.r = r;
                  rgb.g = g;
                  rgb.b = b;    
                  CHSV hsv = rgb2hsv_approximate(rgb);  
                  it.Ctrl(itemCmd().HSV255(hsv.h,hsv.s,hsv.v).setSuffix(S_SET)); */
                  it.Ctrl(itemCmd().RGBW(r,g,b,w).setSuffix(S_SET));
                  //it.Ctrl(rgb2hsv(itemCmd().RGBW(r,g,b,w)).setSuffix(S_SET));
                  it.Ctrl(itemCmd().Cmd(CMD_ON).setSuffix(S_CMD));
                  }
          }
        }
}

void DMXput(void)
{
if (!DMXin) return;  
for (short tch=0; tch<=3 ; tch++)
    {
    short base = tch*4;
    DMXImmediateUpdate(tch,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
    }
    
};

extern volatile uint8_t timerHandlerBusy;

#if defined(_dmxin)
volatile int DMXinDoublecheck=0;
#endif

// INVOKED BY INTERRUPTS - MUST BE SAFE CODE
void DMXUpdate(void)
{
#if defined(_dmxin)
if(!DMXin) return;
#if defined(__SAM3X8E__)
if (dmxin.getRxLength()<DMXINChannels) return;
#endif

uint8_t RGBWChannels=DMXINChannels >> 2;
for (short tch=0; tch<RGBWChannels ; tch++)
    {
    short base = tch*4;
    bool updated = false;
    int t;

    for (short trh=0; trh<4 ; trh++)
    if ((t=dmxin.read(base+trh+1)) != DMXin[base+trh])
      {
        D_State |= (1<<tch);
        updated=1;
        DMXin[base+trh]=t;
      }
     if (updated)
        {
        DMXImmediateUpdate(tch,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
        D_checkT=millisNZ();
        }
    }
#endif
}

// INVOKED in safe loop
void DMXCheck(void)
{
DMXOUT_propagate();
#if defined(_dmxin)
if ( (!D_checkT) || (!isTimeOver(D_checkT,millis(),D_CHECKT))) return;
D_checkT=0;
uint8_t RGBWChannels=DMXINChannels >> 2;
for (short rgbwChan=0; rgbwChan < RGBWChannels; rgbwChan++)
    {
    short base = rgbwChan*4;
    short bitMask = 1 << rgbwChan;
       if (D_State & bitMask)
       {
       D_State &= ~bitMask;
       DMXSemiImmediateUpdate(rgbwChan,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
       break;
       }
    }   

#ifdef _dmxout
debugSerial.print(F("DMXIN:"));
for (int i=1; i<17; i++) {debugSerial.print(dmxin.read(i));debugSerial.print(";");}
debugSerial.println();
#endif

#endif
}


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
#if defined  (_dmxout) && defined (_artnet)
  for (unsigned int i = artnetMinCh-1 ; i < length && i<artnetMaxCh ; i++)
  {
    DmxWrite(i+1,data[i]);
  }
#endif
}

void DMXinSetup(int channels)
{
   if (DMXin) return;/// ToDo: re-init
 // //Use digital pin 3 for DMX output. Must be a PWM channel.
 // DmxSimple.usePin(pin);
  //DmxSimple.maxChannel(channels);

#if defined(_dmxin)
   if (channels>(32*4)) channels = 32*4;
   DMXin = new uint8_t [channels];
   DMXINChannels=channels;
 //  debugSerial<<F("DMXIN: init chans:")<<channels<<endl;
#if defined(ARDUINO_ARCH_AVR)
   DMXSerial.init(DMXReceiver,0,channels);
    if (DMXSerial.getBuffer()) {debugSerial.print(F("Init in ch:"));debugSerial.println(channels);} else debugSerial.println(F("DMXin Buffer alloc err"));
   //DMXSerial.maxChannel(channels);
   DMXSerial.attachOnUpdate(&DMXUpdate);
#endif

#if defined(__SAM3X8E__)
dmxin.setRxEvent(DMXUpdate);
dmxin.begin();
#endif

#endif

 #ifdef _artnet
    // this will be called for each packet received
  if (artnet) artnet->setArtDmxCallback(onDmxFrame);
 #endif
}

void DMXoutSetup(int channels)
{
#ifdef _dmxout

//#ifdef _artnet
//if (channels<artnetMaxCh) artnetMaxCh=channels;
//#endif

#if defined(ARDUINO_ARCH_AVR)
 DmxSimple.usePin(AVR_DMXOUT_PIN);
 DmxSimple.maxChannel(channels);
#endif


#if defined(ESP8266)
dmxout.init(channels);
#endif

#if defined(__SAM3X8E__)
dmxout.begin();
dmxout.setTxMaxChannels(channels);
#endif
#endif

#ifndef DMX_DISABLE
for (int i=1;i<=channels;i++) DmxWrite(i,0);
#endif
debugSerial<<F("DMXOut. Free:")<<freeRam()<<endl;
#ifdef DMX_SMOOTH
if (DMXinterimBuf) delete DMXinterimBuf;
DMXinterimBuf = new uint8_t [channels+1];
DMXOUT_Channels=channels;
for (int i=1;i<=channels;i++) DMXinterimBuf[i]=0;
debugSerial<<F("DMXInterim. Free:")<<freeRam()<<endl;
#endif
}

volatile int8_t propagateBusy = 0;
void DMXOUT_propagate()
{
  #ifdef DMX_SMOOTH
  if (propagateBusy) return;
  propagateBusy++;
  uint32_t now = millis();
  //if (now<checkTimestamp) return;
  if (isTimeOver(checkTimestamp,now,DMX_SMOOTH_DELAY)) 
  {
      for(int i=1;i<=DMXOUT_Channels;i++)
      {
      uint8_t currLevel=dmxout.getTx(i);
      int32_t delta = currLevel-DMXinterimBuf[i-1];
      if (delta)
            {
            uint16_t step  = abs(delta) >> 4;
            if (!step) step=1;

            if (delta<0)
                            {
                              DmxWrite2(i,currLevel+step);
                              //debugSerial<<"<";
                              }

            if (delta>0)
                            {
                              DmxWrite2(i,currLevel-step);
                              //debugSerial<<">";
                              }
            }
      }
      checkTimestamp=now;
  }
  propagateBusy--;
  #endif
}

void artnetSetup()
{
#ifdef _artnet
 if (!artnet)  
          {
          artnet = new Artnet;
          artnet->begin();
         // this will be called for each packet received
         if (artnet) artnet->setArtDmxCallback(onDmxFrame);
          }
#endif
}

void artnetSetChans(uint8_t minCh, uint8_t maxCh)
{
#ifdef _artnet
artnetMinCh=minCh;
artnetMaxCh=maxCh;
#endif
}

void DmxWriteBuf(uint16_t chan,uint8_t val)
{
#ifdef DMX_SMOOTH
if (chan && chan<=DMXOUT_Channels)
          DMXinterimBuf[chan-1] = val;
#endif
}
