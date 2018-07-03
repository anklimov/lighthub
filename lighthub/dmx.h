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

#define D_UPDATED1 1
#define D_UPDATED2 2
#define D_UPDATED3 4
#define D_UPDATED4 8
#define D_CHECKT 300

#define MAX_CHANNELS 60
//define MAX_IN_CHANNELS 16

//#define DMX_OUT_PIN  3

#include "options.h"

#if defined(_dmxout)

#if defined(__AVR__)
#include <DmxSimple.h>
#define DmxWrite DmxSimple.write
#endif

#if defined(ESP8266)
#include <ESPDMX.h>
extern DMXESPSerial dmxout;
#define DmxWrite dmxout.write
#endif

#if defined(__SAM3X8E__)
#include <DmxDue.h>
#define DmxWrite dmxout.write
#endif
#endif

#ifdef _artnet
#include <Artnet.h>
extern Artnet *artnet;
#endif

#ifdef _dmxin
#if defined(__AVR__)
#include <DMXSerial.h>
#endif
#endif

#include "aJSON.h"

extern aJsonObject *dmxArr;


void DMXput(void);
void DMXinSetup(int channels);
void DMXoutSetup(int channels);
void ArtnetSetup();
void DMXCheck(void);
int itemCtrl2(char* name,int r,int g, int b, int w);
void ArtnetSetup();
