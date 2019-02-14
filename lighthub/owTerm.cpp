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


OneWire *ds2482_OneWire = NULL;

DeviceAddress *term = NULL;

uint16_t *wstat = NULL;
DallasTemperature *sensors = NULL;

short si = 0;
int t_count = 0;
unsigned long owTimer = 0;

owChangedType owChanged;

void scan_i2c_bus();

int owUpdate() {
#ifndef OWIRE_DISABLE
    unsigned long finish = millis() + OW_UPDATE_INTERVAL;
    short sr;


    Serial.println(F("Searching"));
    if (ds2482_OneWire) ds2482_OneWire->reset_search();
    for (short i = 0; i < t_count; i++) wstat[i] &= ~SW_FIND; //absent

    while (ds2482_OneWire && ds2482_OneWire->wireSearch(term[t_count]) > 0 && (t_count < t_max) && finish > millis()) {
        short ifind = -1;
        if (ds2482_OneWire->crc8(term[t_count], 7) == term[t_count][7]) {
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
                    ds2482_OneWire->setStrongPullup();
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
    if (ds2482_OneWire) return true;    // Already initialized
#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    debugSerial<<F("DS2482_100_I2C_TO_1W_BRIDGE init");
    ds2482_OneWire = new OneWire;
#else
    debugSerial.print(F("One wire setup on PIN:"));
    debugSerial.println(QUOTE(USE_1W_PIN));
    net = new OneWire (USE_1W_PIN);
#endif
    sensors = new DallasTemperature(ds2482_OneWire);
    term = new DeviceAddress[t_max];
    wstat = new uint16_t[t_max];

#ifdef DS2482_100_I2C_TO_1W_BRIDGE
    Wire.begin();
    scan_i2c_bus();
    if (ds2482_OneWire->checkPresence()) {
        debugSerial.println(F("DS2482-100 present"));
        ds2482_OneWire->deviceReset();
        ds2482_OneWire->setActivePullup();
        debugSerial.println(F("\tChecking for 1-Wire devices..."));
        if (ds2482_OneWire->wireReset())
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
#endif
}

void scan_i2c_bus() {
    byte error, address;
    int nDevices;

    debugSerial<<("Scanning...\n");

    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            debugSerial<<("\nI2C device found at address 0x");
            if (address<16)
                debugSerial<<("0");
            debugSerial<<(address,HEX);
            debugSerial<<("  !");

            nDevices++;
        }
        else if (error==4)
        {
            debugSerial<<("\nUnknow error at address 0x");
            if (address<16)
                debugSerial<<("0");
            debugSerial<<(address,HEX);
        }
    }
    if (nDevices == 0)
        debugSerial<<("No I2C devices found\n");
    else
        debugSerial<<("done\n");
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
            //Serial.println("o");
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
        ds2482_OneWire->setStrongPullup();
        //                sensors.requestTemperaturesByAddress(term[t_count]);
    }
    t_count++;
#endif
}
#endif
