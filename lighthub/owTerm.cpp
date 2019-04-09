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


OneWire *oneWire = NULL;

DeviceAddress *term = NULL;

uint16_t *wstat = NULL;
DallasTemperature *sensors = NULL;

short si = 0;
int t_count = 0;
unsigned long owTimer = 0;

owChangedType owChanged;

int owUpdate() {
#ifndef OWIRE_DISABLE
    unsigned long finish = millis() + OW_UPDATE_INTERVAL;
    short sr;


    Serial.println(F("Searching"));
    if (oneWire) oneWire->reset_search();
    for (short i = 0; i < t_count; i++) wstat[i] &= ~SW_FIND; //absent

    while (oneWire && oneWire->wireSearch(term[t_count]) > 0 && (t_count < t_max) && finish > millis()) {
        short ifind = -1;
        if (oneWire->crc8(term[t_count], 7) == term[t_count][7]) {
            for (short i = 0; i < t_count; i++)
                if (!memcmp(term[i], term[t_count], 8)) {
                    ifind = i;
                    wstat[i] |= SW_FIND;
                    debugSerial.print(F(" Node:"));
                    PrintBytes(term[t_count], 8,0);
                    debugSerial.println(F(" alive"));
                    break;
                }; //alive
            if (ifind < 0 && sensors) {
                wstat[t_count] = SW_FIND; //Newly detected
                debugSerial<<F("dev#")<<t_count<<F(" Addr:");
                PrintBytes(term[t_count], 8,0);
                debugSerial.println();
                if (term[t_count][0] == 0x28) {
                    sensors->setResolution(term[t_count], TEMPERATURE_PRECISION);
                    oneWire->setStrongPullup();
                    //                sensors.requestTemperaturesByAddress(term[t_count]);
                }
                t_count++;
            }
        }//if
    } //while

    debugSerial<<F("1-wire count: ")<<t_count;
#endif
}


int owSetup(owChangedType owCh) {
#ifndef OWIRE_DISABLE
    //// todo - move memory allocation to here
    if (oneWire) return true;    // Already initialized
#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    debugSerial<<F("DS2482_100_I2C_TO_1W_BRIDGE init")<<endl;
    oneWire = new OneWire;
#else
    debugSerial.print(F("One wire setup on PIN:"));
    debugSerial.println(QUOTE(USE_1W_PIN));
    oneWire = new OneWire (USE_1W_PIN);
#endif



// Pass our oneWire reference to Dallas Temperature.
    sensors = new DallasTemperature(oneWire);

    term = new DeviceAddress[t_max];
//regs = new    int [t_max];
    wstat = new uint16_t[t_max];


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

        sensors->begin();
        owChanged = owCh;
        //owUpdate();
        //debugSerial.println(F("\t1-w Updated"));
        sensors->setWaitForConversion(false);


        return true;
    }
#endif
    debugSerial.println(F("\tDS2482 error"));
    return false;
    // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement


    delay(500);

#endif
}


int sensors_loop(void) {
    if (!sensors) return -1;
    if (si >= t_count) {
        owUpdate(); //every check circle - scan for new devices
        si = 0;
        return 8000;
    }

    float t;
    switch (term[si][0]) {

        case 0x28: // Thermomerer
            t = sensors->getTempC(term[si]);//*10.0;
            if (owChanged) owChanged(si, term[si], t);
            sensors->requestTemperaturesByAddress(term[si]);
            si++;
            return 2500;

            //   default
            //   return sensors_ext();
    } //switch


    si++;
    return check_circle;

}


void owLoop() {
    if (millis() >= owTimer) owTimer = millis() + sensors_loop();
}


int owFind(DeviceAddress addr) {
    for (short i = 0; i < t_count; i++) if (!memcmp(term[i], addr, 8)) return i;//find
    return -1;
}

void owAdd(DeviceAddress addr) {
#ifndef OWIRE_DISABLE
  if (t_count>=t_max) return;
    wstat[t_count] = SW_FIND; //Newly detected
    memcpy(term[t_count], addr, 8);
    //term[t_count]=addr;

    debugSerial<<F("dev#")<<t_count<<F(" Addr:");
    PrintBytes(term[t_count], 8,0);
    debugSerial.println();
    if (term[t_count][0] == 0x28) {
        sensors->setResolution(term[t_count], TEMPERATURE_PRECISION);
        oneWire->setStrongPullup();
        //                sensors.requestTemperaturesByAddress(term[t_count]);
    }
    t_count++;
#endif
}
#endif
