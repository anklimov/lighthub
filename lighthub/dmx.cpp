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

volatile uint32_t D_State=0;

volatile unsigned long D_checkT=0;

#ifdef _artnet
#include <Artnet.h>
Artnet *artnet = NULL;
uint8_t artnetMinCh=1;
uint8_t artnetMaxCh=512;
#endif


extern aJsonObject *items;
extern aJsonObject *dmxArr;


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
            { //Serial.println(i->valuestring);
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

void DMXSemiImmediateUpdate(short tch,short trh, int val)
{
//Here any code for  passthrow between DMX IN and DMX OUT in idle state
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
volatile int DMXinDoublecheck=0;

void DMXUpdate(void)
{
#if defined(_dmxin)
int t;
if(!DMXin) return;

#if defined(__SAM3X8E__)
if (dmxin.getRxLength()<16) return;
#endif
for (short tch=0; tch<=3 ; tch++)
    {
    short base = tch*4;
    bool updated   = 0;
    bool confirmed = 0;

    for (short trh=0; trh<4 ; trh++)
    if (((t=dmxin.read(base+trh+1)) != DMXin[base+trh]))
      {
        updated=1;
         if (DMXinDoublecheck>2)
           {
            D_State |= (1<<tch);
            DMXin[base+trh]=t;
            confirmed = 1;
           }
      } 

    if (updated) DMXinDoublecheck++; else DMXinDoublecheck=0;


     if (confirmed)
        {

        DMXImmediateUpdate(tch,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
        //for (int i=1; i<17; i++) {debugSerial.print(dmxin.read(i));debugSerial.print("-");};debugSerial.print("|");
        D_checkT=millisNZ();
        }
    }
    //Serial.print(D_State,BIN);Serial.println();
#endif
}


 void DMXCheck(void)
{
//  CHSV hsv;
//  CRGB rgb;
DMXOUT_propagate();
#if defined(_dmxin)

short t,tch;
//Here code for semi-immediate update
  for (t=1,tch=0; t<=8 ; t<<=1,tch++)
       if (D_State & t)
       {
          // Serial.print(D_State,BIN);Serial.print(":");
       D_State &= ~t;
       for (short trh=0; trh<4 ; trh++)
          DMXSemiImmediateUpdate(tch,trh,DMXin[tch*4+trh]);

       }

//if ((millis()<D_checkT) || (D_checkT==0)) return;
  if ( (!D_checkT) || (!isTimeOver(D_checkT,millis(),D_CHECKT))) return;
D_checkT=0;

// Here code for network update
//int ch = 0;

DMXput();

#ifdef _dmxout
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
   DMXin = new uint8_t [channels];
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
 if (!artnet)  artnet = new Artnet;
    // this will be called for each packet received
  if (artnet) artnet->setArtDmxCallback(onDmxFrame);
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
