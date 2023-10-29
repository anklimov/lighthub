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
aJsonObject *dev2Check = NULL;

OneWire *oneWire = NULL;

//DeviceAddress *term = NULL;
//uint16_t *wstat = NULL;

DallasTemperature *sensors = NULL;

//short si = 0;
//int t_count = 0;
unsigned long owTimer = 0;

//owChangedType owChanged;

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

char * getReableNote(aJsonObject * owObj)
{
     if (owObj && owObj->child)
     {
       if (owObj->child->type==aJson_String && owObj->child->valuestring) return owObj->child->valuestring;
       if (owObj->child->child && owObj->child->child->type==aJson_String && owObj->child->child->valuestring) return owObj->child->child->valuestring;
       return NULL;
     }

}
void processTemp(aJsonObject * owObj, float currentTemp) {
    if (!owObj || !owArr) return;
    char* note = getReableNote(owObj);   
    debugSerial<<endl<<F("1WT:")<<currentTemp<<F(" <")<<owObj->name<<F("> ");    
    if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
        {
        if (note) debugSerial<<note;    
        debugSerial<<endl;    
        executeCommand(owObj,-1,itemCmd(currentTemp).setSuffix(S_VAL));    
        }
    else 
        if (note) errorSerial<<F("1WT: Read error for ")<<note<<endl;  
  
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
    //unsigned long finish = millis();// + OW_UPDATE_INTERVAL;
    if (!oneWire || !owArr) return 0;
    oneWire->reset_search();
    debugSerial << F("1WT: Searching dev")<<endl;
    while (oneWire->wireSearch(dev) > 0)
        {   
            char addrstr[17];
            SetBytes(dev, 8, addrstr);
            addrstr[16] = 0;
            aJsonObject * owObj=aJson.getObjectItem(owArr,addrstr);
            debugSerial<<F("1WT:")<<addrstr;
            if (owObj)
                    {
                        char * note = getReableNote(owObj);
                        if (note) debugSerial<<F(" is ")<<note<<endl;
                    }
            else 
                    {
                        debugSerial<<F(" new")<<endl;
                    }    
        }



/*

    if (oneWire) oneWire->reset_search();
    for (short i = 0; i < t_count; i++) wstat[i] &= ~SW_FIND; //absent

    while (oneWire && oneWire->wireSearch(term[t_count]) > 0 && (t_count < t_max)  && !isTimeOver(finish,millis(), OW_UPDATE_INTERVAL))//&& finish > millis()) 
    {
        short ifind = -1;
        if (oneWire->crc8(term[t_count], 7) == term[t_count][7]) {
            for (short i = 0; i < t_count; i++)
                if (!memcmp(term[i], term[t_count], 8)) {
                    ifind = i;
                    wstat[i] |= SW_FIND;
                    debugSerial.print(F(" Node:"));
                    PrintBytes(term[t_count], 8,0);
                    processTemp(-1, term[t_count], 0.0); //print note
                    debugSerial.println(F(" alive"));
                    break;
                }; //alive
            if (ifind < 0 && sensors && !zero(term[t_count],8))
                {
                wstat[t_count] = SW_FIND; //Newly detected
                debugSerial<<F("dev#")<<t_count<<F(" Addr:");
                PrintBytes(term[t_count], 8,0);
                if processTemp(-1, term[t_count], 0.0); //print note
                debugSerial.println();
                if (term[t_count][0] == 0x28) {
                    sensors->setResolution(term[t_count], TEMPERATURE_PRECISION);
                    #ifdef DS2482_100_I2C_TO_1W_BRIDGE
                    oneWire->setStrongPullup();
                    #endif
                    //                sensors.requestTemperaturesByAddress(term[t_count]);
                }
                t_count++;
            }
        }//if
    } //while

    debugSerial<<F("1-wire count: ")<<t_count<<endl;
*/
#endif
return true;
}


int owSetup() {
#ifndef OWIRE_DISABLE
    //// todo - move memory allocation to here
    if (oneWire) return true;    // Already initialized
#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    debugSerial<<F("DS2482_100_I2C_TO_1W_BRIDGE init")<<endl;
    debugSerial<<F("Free:")<<freeRam()<<endl;
    oneWire = new OneWire;
#else
    debugSerial.print(F("One wire setup on PIN:"));
    debugSerial.println(QUOTE(USE_1W_PIN));
    oneWire = new OneWire (USE_1W_PIN);
#endif

if (!oneWire)
                {
                    errorSerial<<F("Error 1-w init #1")<<endl;
                    return false;
                }

            

// Pass our oneWire reference to Dallas Temperature.
//    sensors = new DallasTemperature(oneWire);

/*
    term = new DeviceAddress[t_max];
    debugSerial<<F("Term. Free:")<<freeRam()<<endl;
//regs = new    int [t_max];
    wstat = new uint16_t[t_max];
    debugSerial<<F("wstat. Free:")<<freeRam()<<endl;
if (!term || ! wstat)
                {
                    errorSerial<<F("Error 1-w init #2 Free:")<<freeRam()<<endl;
                    return false;
                }

    owChanged = owCh;
*/

#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    Wire.begin();
    if (oneWire->checkPresence()) {
        debugSerial.println(F("DS2482-100 present"));
        oneWire->deviceReset();
#ifdef APU_OFF
        debugSerial.println(F("APU off"));
#else
        oneWire->setActivePullup();
#endif

        debugSerial.println(F("\tChecking for 1-Wire devices..."));
        if (oneWire->wireReset())
            debugSerial.println(F("\tReset done"));
        return true;
    }
    debugSerial.println(F("\tDS2482 error"));
    return false;
#else
   // software driver
   oneWire->reset();
   delay(500);
   return true;
#endif //DS2482-100

#endif //1w is not disabled
return false;
}


int sensors_loop(void) {
#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    if (oneWire->getError() == DS2482_ERROR_SHORT)
       {
         debugSerial<<F("1-wire shorted")<<endl;
         oneWire->wireReset();
         return 10000;
       }
#endif


if (!sensors)
       {
         // Setup sensors library and resolution
         sensors = new DallasTemperature(oneWire);
         sensors->begin();
         // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

         //for (short i = 0; i < t_count; i++) sensors->setResolution(term[i],TEMPERATURE_PRECISION);
         sensors->setWaitForConversion(false);
        }

    if (!dev2Check && owArr)
        {
 
          if (owArr && owArr->type == aJson_Object && owArr->child) dev2Check = owArr->child;
          ///owUpdate(); //every check circle - scan for new devices
        }

/*
    if (si >= t_count) {
        owUpdate(); //every check circle - scan for new devices
        si = 0;
        return 8000;
    }
*/
    DeviceAddress curDev;
    
    if (dev2Check && SetAddr(dev2Check->name,curDev))
    {
    float t;
    switch (curDev[0]) {

        case 0x28: // Thermomerer
            sensors->setResolution(curDev,TEMPERATURE_PRECISION);

            t = sensors->getTempC(curDev);//*10.0;
            processTemp(dev2Check, t);
            sensors->requestTemperaturesByAddress(curDev);

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

/*
int owFind(DeviceAddress addr) {
    for (short i = 0; i < t_count; i++) if (!memcmp(term[i], addr, 8)) return i;//find
    return -1;
}

void owAdd(DeviceAddress addr) {
#ifndef OWIRE_DISABLE
    infoSerial<<F("dev#")<<t_count<<F(" Addr:");
    PrintBytes(term[t_count], 8,0);
    infoSerial<<endl;

  if (t_count>=t_max) return;
  if (zero(term[t_count],8)) return;

    wstat[t_count] = SW_FIND; //Newly detected
    memcpy(term[t_count], addr, 8);


    #ifdef DS2482_100_I2C_TO_1W_BRIDGE
    if (term[t_count][0] == 0x28)
                  oneWire->setStrongPullup();
    #endif
    t_count++;
#endif
}

*/
#endif

void setupOwIdle (void (*ptr)())
{
  #ifdef DS2482_100_I2C_TO_1W_BRIDGE
    if (oneWire) oneWire->idle(ptr);
  #endif
}

