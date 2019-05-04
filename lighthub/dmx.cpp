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

#ifdef _dmxin
#if defined(ARDUINO_ARCH_AVR)
#include <DMXSerial.h>
#endif
#endif

#if defined(ESP8266)
#ifndef DMX_DISABLE
DMXESPSerial dmxout;
#endif
#endif

uint8_t * DMXin = NULL;
int D_State=0;

unsigned long D_checkT=0;

#ifdef _artnet
#include <Artnet.h>
Artnet *artnet = NULL;
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
       case 0: //Dimmed light

       DmxWrite(itemaddr, w);
       break;


       case 1: //Colour RGBW
        DmxWrite(itemaddr+3, w);

       case 2: // RGB
      {

        DmxWrite(itemaddr,   r);
        DmxWrite(itemaddr+1, g);
        DmxWrite(itemaddr+2, b);

         break; }
#endif
        case 7: //Group
        aJsonObject *groupArr= aJson.getArrayItem(itemArr, 1);
        if (groupArr && (groupArr->type==aJson_Array))
        { aJsonObject *i =groupArr->child;
          while (i)
            { //Serial.println(i->valuestring);
            itemCtrl2(i->valuestring,r,g,b,w);
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
        char* itemname = aJson.getArrayItem(dmxArr,tch)->valuestring;
        if (itemname) itemCtrl2(itemname,r,g,b,w);
        }
}

void DMXSemiImmediateUpdate(short tch,short trh, int val)
{
//Here any code for  passthrow between DMX IN and DMX OUT in idle state
}

void DMXput(void)
{
int t;
for (short tch=0; tch<=3 ; tch++)
    {
    short base = tch*4;
    DMXImmediateUpdate(tch,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
    }
};


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
    bool updated = 0;

    for (short trh=0; trh<4 ; trh++)
    if ((t=dmxin.read(base+trh+1)) != DMXin[base+trh])
      {
        D_State |= (1<<tch);
        updated=1;
        //Serial.print("onContactChanged :"); Serial.print(DMXin[tch*4+trh]); Serial.print(" => "); Serial.print(t);Serial.println();
        DMXin[base+trh]=t;
        //DMXImmediateUpdate(tch,trh,t);
        //break;
      }

     if (updated)
        {
        DMXImmediateUpdate(tch,DMXin[base],DMXin[base+1],DMXin[base+2],DMXin[base+3]);
        D_checkT=millis()+D_CHECKT;
        }
    }
    //Serial.print(D_State,BIN);Serial.println();
#endif
}


 void DMXCheck(void)
{
//  CHSV hsv;
//  CRGB rgb;
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

if ((millis()<D_checkT) || (D_checkT==0)) return;
D_checkT=0;

// Here code for network update
//int ch = 0;
DMXput();
#ifdef _dmxout
for (int i=1; i<17; i++) {Serial.print(dmxin.read(i));Serial.print(";");}
#endif
       Serial.println();
#endif
}


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
#ifdef _dmxout
  for (int i = 0 ; i < length && i<MAX_CHANNELS ; i++)
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
    if (DMXSerial.getBuffer()) {Serial.print(F("Init in ch:"));Serial.println(channels);} else Serial.println(F("DMXin Buffer alloc err"));
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
for (int i=1;i<=channels;i++) DmxWrite(i,0);
}

void ArtnetSetup()
{
#ifdef _artnet
 if (!artnet)  artnet = new Artnet;
    // this will be called for each packet received
  if (artnet) artnet->setArtDmxCallback(onDmxFrame);
#endif
}
