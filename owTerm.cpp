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


#include "owTerm.h"
#include <Arduino.h>
#include "utils.h"

  
OneWire *net = NULL;
// Pass our oneWire reference to Dallas Temperature. 
//DallasTemperature sensors(&net);

DeviceAddress  *term = NULL;
//int            *regs = NULL;
uint16_t       *wstat = NULL;
DallasTemperature *sensors = NULL;

short si=0;
int t_count = 0;
unsigned long owTimer=0;

owChangedType  owChanged;

int owUpdate()
{   
  unsigned long finish = millis() + 5000;
  short sr;
  
  //net.setStrongPullup();
 Serial.println(F("Searching"));
if (net)  net->reset_search();
for (short i=0;i<t_count;i++) wstat[i]&=~SW_FIND; //absent

while (net && net->wireSearch(term[t_count])>0 && (t_count<t_max) && finish > millis ())
  { short ifind=-1; 
    if (net->crc8(term[t_count], 7) == term[t_count][7])
    {
     for (short i=0;i<t_count;i++) if (!memcmp(term[i],term[t_count],8)) {ifind=i;wstat[i]|=SW_FIND; 
                                                                            Serial.print(F(" Node:"));PrintBytes(term[t_count],8);Serial.println(F(" alive"));
                                                                          break;}; //alive
     if (ifind<0 && sensors) 
          {
          wstat[t_count]=SW_FIND; //Newly detected  
          Serial.print(F("dev#"));Serial.print(t_count);Serial.print(F(" Addr:"));PrintBytes(term[t_count],8);
          Serial.println();
          if (term[t_count][0]==0x28) 
                    {
                    sensors->setResolution(term[t_count], TEMPERATURE_PRECISION);
                    net->setStrongPullup();
    //                sensors.requestTemperaturesByAddress(term[t_count]);
                    }
          t_count++;}
    }//if
  } //while

 Serial.print(F("1-wire count: "));
 Serial.println(t_count);

}

 
int owSetup(owChangedType owCh) {
  //// todo - move memory allocation to here

#ifdef _2482   
net = new OneWire;
#else
net = new OneWire (ONE_WIRE_BUS);
#endif



// Pass our oneWire reference to Dallas Temperature. 
sensors = new DallasTemperature (net);

term = new    DeviceAddress[t_max];
//regs = new    int [t_max];
wstat = new   uint16_t  [t_max];


  
  #ifdef _2482
  Wire.begin(); 
  if (net->checkPresence())
  {
    Serial.println(F("DS2482-100 present"));
    net->deviceReset();
    #ifdef APU_OFF
    Serial.println(F("APU off"));
    #else
    net->setActivePullup();
    #endif
    
    Serial.println(F("\tChecking for 1-Wire devices..."));
    if (net->wireReset())
         Serial.println(F("\tReset done"));
    
    sensors->begin();
    owChanged=owCh; 
    //owUpdate();
    //Serial.println(F("\t1-w Updated"));
    sensors->setWaitForConversion(false);

    
    return true;
  }
  #endif
  
 return false;
  // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
 
  
 delay(500);

}


int  sensors_loop(void)
 {  
  if (!sensors) return -1;
  if (si>=t_count)
     {
      owUpdate(); //every check circle - scan for new devices
      si=0;
      return 8000;
     }

  int t;  
  switch (term[si][0]){
  
  case 0x28: // Thermomerer  
  t=sensors->getTempC(term[si]);//*10.0;
  //Serial.println("o");
  if (owChanged) owChanged(si,term[si],t);
  sensors->requestTemperaturesByAddress(term[si]);
  si++;
  return 2500;
    
 //   default 
 //   return sensors_ext();
  } //switch
  
  
  si++;
  return check_circle;   
     
}


void owLoop()
  
{
  if (millis() >=owTimer) owTimer=millis()+sensors_loop();
}


int owFind(DeviceAddress addr)
{ 
  for (short i=0;i<t_count;i++) if (!memcmp(term[i],addr,8)) return i;//find
  return -1;
}

void owAdd (DeviceAddress addr)
{ 
   wstat[t_count]=SW_FIND; //Newly detected  
   memcpy (term[t_count],addr,8);
   //term[t_count]=addr;
   
   Serial.print(F("dev#"));Serial.print(t_count);Serial.print(F(" Addr:"));PrintBytes(term[t_count],8);
   Serial.println();
   if (term[t_count][0]==0x28) 
                    {
                    sensors->setResolution(term[t_count], TEMPERATURE_PRECISION);
                    net->setStrongPullup();
    //                sensors.requestTemperaturesByAddress(term[t_count]);
                    }
   t_count++;
}


