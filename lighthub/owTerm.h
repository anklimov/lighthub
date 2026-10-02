/* Copyright © 2017-2020 Andrey Klimov. All rights reserved.

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

//define APU_OFF
/**
 * @file owTerm.h
 * @brief 1-Wire temperature sensing for thermostats.
 *
 * Reads DS18S20/DS18B20 digital thermometers (device type 0x28) through
 * a DS2482-100 I2C-to-1-Wire bridge (or a software 1-Wire driver on a
 * dedicated pin). Each configured sensor is polled periodically and its
 * temperature is published to the item's /val subtopic.
 */
#pragma once

#define SW_AUX0 0x40
#define SW_AUX1 0x80
#define SW_STAT0 0x4
#define SW_STAT1 0x8
#define SW_OUT0 0x20
#define SW_OUT1 0x10
#define SW_MASK 0xF
#define SW_INMASK 0xFC

//#define recheck_interval 5
//#define check_circle 2000/t_count

#define SW_FIND        1
#define SW_DOUBLECHECK 2      //Doublecheck required
#define SW_PULSE0      4      //Pulse Reset started
#define SW_PULSE1      8      //Pulse Reset stsrted
#define SW_PULSE_P0    0x10   //Pulse reset in process
#define SW_PULSE_P1    0x20   //Pulse reset in process
#define SW_CHANGED_P0  0x40   //Changes while pulse in progress
#define SW_CHANGED_P1  0x80   //Changes while pulse in progress
#define SW_PULSE0_R      0x100    //Pulse Reset requested
#define SW_PULSE1_R      0x200    //Pulse Reset requested


//#define recheck_interval 5
//#define check_circle 2000/t_count

#if not defined (OW_DEVICES_LIMIT)
#define t_max 20 //Maximum number of 1w devices
#else
#define t_max OW_DEVICES_LIMIT
#endif

#define TEMPERATURE_PRECISION 12 //9

#ifndef OWIRE_DISABLE

#include <DallasTemperature.h>
#include "aJSON.h"

/** @brief 1-Wire sensor configuration array from the JSON config. */
extern aJsonObject *owArr;

//typedef   void (*owChangedType) (int , DeviceAddress, float) ;

#ifndef USE_1W_PIN
#define DS2482_100_I2C_TO_1W_BRIDGE // HW driver
#include <Wire.h>
  #ifndef ARDUINO_ARCH_STM32F1
  #include <DS2482_OneWire.h>
  #endif
#else
#include <OneWire.h> //Software driver
#define wireReset reset
#define wireSearch search
#endif


/** @brief 1-Wire bus handle (DS2482 bridge or software pin driver). */
extern OneWire *oneWire;
/** @brief DallasTemperature manager on top of the 1-Wire bus. */
extern DallasTemperature *sensors;

/**
 * @brief Scans the 1-Wire bus and logs the devices it finds.
 * @return true on success.
 */
int  owUpdate();
/**
 * @brief Initializes the 1-Wire bus and the temperature sensors, and
 * applies the configured resolution.
 * @return true on success.
 */
int  owSetup();
/**
 * @brief Periodic poll: reads each configured sensor and publishes its
 * temperature to the item's /val subtopic.
 */
void owLoop();
/**
 * @brief Registers an idle callback invoked between 1-Wire operations.
 * @param ptr Function to call while the bus is idle.
 */
void setupOwIdle(void (*)()) ;
/**
 * @brief Triggers a 1-Wire device search/update.
 */
void owSearch();

#endif
