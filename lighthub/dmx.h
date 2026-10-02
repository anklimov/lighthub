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
/**
 * @file dmx.h
 * @brief DMX input/output (and Art-Net) support.
 *
 * DMX output uses a hardware USART on the SAM3X8E (Due) and a software
 * bit-banged driver (DmxSimple, pin 3/18) on AVR; DMX input uses
 * DMXSerial on AVR and the hardware USART on the Due. Received DMX/Art-Net
 * channels are mapped to items via the dmxArr configuration.
 */
#define D_UPDATED1 1
#define D_UPDATED2 2
#define D_UPDATED3 4
#define D_UPDATED4 8
#define D_CHECKT 300

//#define MAX_CHANNELS 60
//define MAX_IN_CHANNELS 16

//#define DMX_OUT_PIN  3

#include "options.h"

#if defined(_dmxout)

#if defined DMX_SMOOTH

#if defined(ARDUINO_ARCH_AVR)
#include <DmxSimple.h>
#define DmxWrite DmxSimple.write
//#define DmxWrite  DmxWriteBuf
#endif

#if defined(ESP8266)
#include <ESPDMX.h>
extern DMXESPSerial dmxout;
#define DmxWrite dmxout.write
//#define DmxWrite  DmxWriteBuf
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <ESPDMX.h>
extern DMXESPSerial dmxout;
#define DmxWrite dmxout.write
//#define DmxWrite  DmxWriteBuf
#endif

#if defined(__SAM3X8E__)
#include <DmxDue.h>
#define DmxWrite2 dmxout.write
#define DmxWrite  DmxWriteBuf
#endif

#else
#if defined(ARDUINO_ARCH_AVR)
#include <DmxSimple.h>
#define DmxWrite DmxSimple.write
#endif

#if defined(ESP8266)
#include <ESPDMX.h>
extern DMXESPSerial dmxout;
#define DmxWrite dmxout.write
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <ESPDMX.h>
extern DMXESPSerial dmxout;
#define DmxWrite dmxout.write
#endif

#if defined(__SAM3X8E__)
#include <DmxDue.h>
#define DmxWrite dmxout.write
#endif

#endif
#endif

#ifdef _artnet
#include <Artnet.h>
extern Artnet *artnet;
#endif

#ifdef _dmxin
#if defined(ARDUINO_ARCH_AVR)
#include <DMXSerial.h>
#endif
#endif

#include "aJSON.h"

/** @brief DMX channel-to-item mapping array from the JSON config. */
extern aJsonObject *dmxArr;

/**
 * @brief Propagates received DMX input frames to the mapped items.
 *
 * DMX in/out is handled per platform: on the SAM3X8E (Due) a hardware
 * USART (DmxDue) is used for both input and output; on AVR a software
 * bit-banged driver (DmxSimple for output on pin 3/18, DMXSerial for
 * input). ESP8266/ESP32 use ESPDMX.
 */
void DMXput(void);
/**
 * @brief Sets up the DMX input for the given number of channels.
 * @param channels Number of DMX input channels.
 */
void DMXinSetup(int channels);
/**
 * @brief Sets up the DMX output for the given number of channels.
 * @param channels Number of DMX output channels.
 */
void DMXoutSetup(int channels);
/**
 * @brief Initializes the Art-Net receiver and wires its frame callback.
 */
void artnetSetup();
/**
 * @brief Restricts the Art-Net channel range that is forwarded.
 * @param minCh First channel (1-based) to accept.
 * @param maxCh Last channel (1-based) to accept.
 */
void artnetSetChans(uint8_t minCh, uint8_t maxCh);
/**
 * @brief DMX housekeeping called from the main loop: propagates changes
 * and services pending DMX-in updates.
 */
void DMXCheck(void);
/**
 * @brief Drives a named DMX/LED item with an RGB(W) value.
 * @param name Name of the target item.
 * @param r Red component.
 * @param g Green component.
 * @param b Blue component.
 * @param w White component.
 * @return 1 on success, 0 if the item was not found.
 */
int itemCtrl2(char* name,int r,int g, int b, int w);
/**
 * @brief Writes a value into the intermediate (smoothing) DMX output buffer.
 * @param chan 1-based DMX channel.
 * @param val Channel value 0..255.
 */
void DmxWriteBuf(uint16_t chan,uint8_t val);
/**
 * @brief Smoothly ramps the actual DMX output toward the buffered target.
 */
void DMXOUT_propagate();
