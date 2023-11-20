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

#ifndef OWIRE_DISABLE

#include "owTerm.h"
#include <Arduino.h>
#include "utils.h"
#include "options.h"
#include "main.h"
#include "aJSON.h"

extern aJsonObject *owArr;
//extern  uint32_t timerCtr;
aJsonObject *dev2Check = NULL;

OneWire *oneWire = NULL;
DallasTemperature *sensors = NULL;

unsigned long owTimer = 0;

void owSearch()
{
    owUpdate();
}

bool zero(const uint8_t *addr, uint8_t len)
{
	while (len--) 
		if (addr[len]) return false;
	return true;
}

char * getReadableNote(aJsonObject * owObj)
{
     if (owObj && owObj->child)
     {
       if (owObj->child->type==aJson_String && owObj->child->valuestring) return owObj->child->valuestring;
       if (owObj->child->child && owObj->child->child->type==aJson_String && owObj->child->child->valuestring) return owObj->child->child->valuestring;
   
     }
    return NULL;
}
void processTemp(aJsonObject * owObj, float currentTemp) {
    if (!owObj || !owArr) return;
    char* note = getReadableNote(owObj);   
    debugSerial <<F("1WT:")<<currentTemp<<F(" <")<<owObj->name<<F("> ");    
    if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
        {
        if (note) debugSerial<<note;    
        debugSerial<<endl;    
        executeCommand(owObj,-1,itemCmd(currentTemp).setSuffix(S_VAL));    
        }
    else 
        if (note) debugSerial<<F(" read error for ")<<note<<endl;  
  
/*
#ifdef WITH_DOMOTICZ
            aJsonObject *idx = aJson.getObjectItem(owObj, "idx");
        if (idx && && idx->type ==aJson_String && idx->valuestring) {//DOMOTICZ json format support
            debugSerial << endl << idx->valuestring << F(" Domoticz valstr:");
            char valstr[50];
            sprintf(valstr, "{\"idx\":%s,\"svalue\":\"%.1f\"}", idx->valuestring, currentTemp);
            debugSerial << valstr;
            if (mqttClient.connected() && !ethernetIdleCount)
                  mqttClient.publish(owEmitString, valstr);
            return;
        }
#endif
*/
}


int owUpdate() {
#ifndef OWIRE_DISABLE
    DeviceAddress dev;
    if (!oneWire || !owArr) return 0;
    oneWire->reset_search();
    infoSerial << F("1WT: Searching dev")<<endl;
    while (oneWire->wireSearch(dev) > 0)
        {   
            wdt_res();
   //         owIdle();
            char addrstr[17];
            SetBytes(dev, 8, addrstr);
            addrstr[16] = 0;
            aJsonObject * owObj=aJson.getObjectItem(owArr,addrstr);
            infoSerial<<F("1WT:")<<addrstr;
            if (owObj)
                    {
                        char * note = getReadableNote(owObj);
                        if (note) infoSerial<<F(" is ")<<note<<endl;
                    }
            else 
                    {
                        infoSerial<<F(" new")<<endl;
                    }    
        }


#endif
return true;
}


int owSetup() {
#ifndef OWIRE_DISABLE
    //// todo - move memory allocation to here
    if (oneWire) return true;    // Already initialized
#ifdef DS2482_100_I2C_TO_1W_BRIDGE

    debugSerial<<F("1WT: DS2482_100_I2C_TO_1W_BRIDGE init")<<endl;
    //debugSerial<<F("Free:")<<freeRam()<<endl;
    oneWire = new OneWire;
#else
    debugSerial.print(F("One wire setup on PIN:"));
    debugSerial.println(QUOTE(USE_1W_PIN));
    oneWire = new OneWire (USE_1W_PIN);
#endif

if (!oneWire)
                {
                    errorSerial<<F("Error 1-w init")<<endl;
                    return false;
                }
    

#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    Wire.begin();
    if (oneWire->checkPresence()) {
        infoSerial.println(F("1WT: DS2482-100 present, reset"));
        oneWire->deviceReset();
#ifdef APU_OFF
        debugSerial.println(F("APU off"));
#else
        oneWire->setActivePullup();
#endif

       // debugSerial.println(F("\tChecking for 1-Wire devices..."));
        if (oneWire->wireReset())
            debugSerial.println(F("1WT: Bus Reset done"));
        else 
            debugSerial.println(F("1WT: Bus reset error"));    
        //return true;
    }   
        else 
            {
            i2cReset();
            if (oneWire->checkPresence())
            infoSerial<<F("1WT: DS2482-100 I2C restored")<<endl;
            else
            {    
                errorSerial.println(F("1WT: DS2482-100 not present"));
                return false;
            }
            }
#else
   // software driver
   oneWire->reset();
   delay(500);
 //  return true;
#endif //DS2482-100

if (!owArr) return false;

if (!sensors)
       {
         // Setup sensors library and resolution
         sensors = new DallasTemperature(oneWire);
         sensors->begin();
         // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

         //for (short i = 0; i < t_count; i++) sensors->setResolution(term[i],TEMPERATURE_PRECISION);
         sensors->setWaitForConversion(false);
        }

aJsonObject *item = owArr->child;
DeviceAddress curDev;
while (owArr && item && SetAddr(item->name,curDev) )
{
debugSerial<<F("1WT: setup resolution ")<<item->name<<endl;    
sensors->setResolution(curDev,TEMPERATURE_PRECISION);
item=item->next;   
}

//owUpdate();

return true;

#else //1w is not disabled

return false;
#endif
}


int sensors_loop(void) {
#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    switch (oneWire->getError())
    {  
        case 0:
        break;

        case  DS2482_ERROR_SHORT:
        errorSerial<<F("1WT: 1-wire shorted")<<endl;
        oneWire->wireReset();
        return INTERVAL_1W; 
        
        case  DS2482_ERROR_CONFIG:
        errorSerial<<F("1WT: DS2482_ERROR_CONFIG")<<endl;
        i2cReset();
        break;

        case  DS2482_ERROR_TIMEOUT: //Busy over time
        errorSerial<<F("1WT: BUSY timeout")<<endl;
        i2cReset();
        break;

        default:
        break;
    }
    /*
    if (!oneWire->checkPresence()) 
    {
        infoSerial.println(F("1WT: lost DS2482-100"));
        i2cReset();
    }   

  */          
       
#endif

if (!sensors || !owArr)
    {
        errorSerial<<F("1WT: not init")<<endl;
        return INTERVAL_1W;
    }

    if (!dev2Check && owArr)
        {
          if (owArr && owArr->type == aJson_Object && owArr->child) dev2Check = owArr->child;
          ///owUpdate(); //every check circle - scan for new devices
        }


    setupOwIdle(&owIdle);
    DeviceAddress curDev;
    
    if (dev2Check && SetAddr(dev2Check->name,curDev))
    {
    float t;
    switch (curDev[0]) {

        case 0x28: // Thermomerer
//debugSerial<<millis()<<" "<<timerCtr<<endl;

            t = sensors->getTempC(curDev);//*10.0;
            //owIdle();
//debugSerial<<millis()<<" "<<timerCtr<<endl;
            processTemp(dev2Check, t);
            //owIdle();

            sensors->requestTemperaturesByAddress(curDev);
            //owIdle();
//debugSerial<<millis()<<" "<<timerCtr<<endl;
    } //switch
    }

    //si++;
    dev2Check=dev2Check->next;
    return INTERVAL_1W;

}


void owLoop() {
    if (isTimeOver(owTimer,millis(),INTERVAL_1W))
    {
        sensors_loop();
        owTimer=millis();
    }
}


#endif

void setupOwIdle (void (*ptr)())
{
  #ifdef DS2482_100_I2C_TO_1W_BRIDGE
    if (oneWire) oneWire->idle(ptr);
  #endif
}

