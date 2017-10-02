/* Copyright © 2017 Andrey Klimov. All rights reserved.

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
//#include <Artnet.h>
//#include <DMXSerial.h>

uint8_t * DMXin = NULL;
int D_State=0;
unsigned long D_checkT=0;

Artnet *artnet = NULL;
aJsonObject *dmxArr = NULL;

void DMXImmediateUpdate(short tch,short r, short g, short b, short w) {
//Here only safe re-interable code for quick passthrow between DMX IN and DMX OUT
       if (dmxArr && (dmxArr->type==aJson_Array))
     
        { 
        char* itemname = aJson.getArrayItem(dmxArr,tch)->valuestring;
        itemCtrl2(itemname,r,g,b,w);
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
int t;
for (short tch=0; tch<=3 ; tch++) 
    {
    short base = tch*4;  
    bool updated = 0;
    
    for (short trh=0; trh<4 ; trh++) 
    if ((t=DMXSerial.read(base+trh+1)) != DMXin[base+trh])
      {
        D_State |= (1<<tch);
        updated=1;
        //Serial.print("Changed :"); Serial.print(DMXin[tch*4+trh]); Serial.print(" => "); Serial.print(t);Serial.println();
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
}


 void DMXCheck(void)
{
//  CHSV hsv;
//  CRGB rgb;
   
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
for (int i=1; i<17; i++) {Serial.print(DMXSerial.read(i));Serial.print(";");}
       Serial.println();
}


void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  for (int i = 0 ; i < length && i<MAX_CHANNELS ; i++)
  {
    DmxSimple.write(i+1,data[i]);   
  }
}

void DMXinSetup(int channels)
{
 // //Use digital pin 3 for DMX output. Must be a PWM channel.
 // DmxSimple.usePin(pin);
  //DmxSimple.maxChannel(channels);

   DMXin = new uint8_t [channels];
  
   DMXSerial.init(DMXReceiver,0,channels);
    if (DMXSerial.getBuffer()) {Serial.print(F("Init in ch:"));Serial.println(channels);} else Serial.println(F("DMXin Buffer alloc err"));
   //DMXSerial.maxChannel(channels);
   DMXSerial.attachOnUpdate(&DMXUpdate);

    // this will be called for each packet received
  if (artnet) artnet->setArtDmxCallback(onDmxFrame);
}

void ArtnetSetup()
{
 if (!artnet)  artnet = new Artnet;
    // this will be called for each packet received
  if (artnet) artnet->setArtDmxCallback(onDmxFrame);
}
