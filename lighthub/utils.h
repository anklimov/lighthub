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
#pragma once

#define Q(x) #x
#define QUOTE(x) Q(x)
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)
#define VAR_NAME_VALUE(var) #var "="  VALUE(var)

#include <Arduino.h>
#include <IPAddress.h>
#include "aJSON.h"
#include "options.h"
#include "item.h"
#ifdef WITH_PRINTEX_LIB
#include "PrintEx.h"
using namespace ios;
#else
#include "Streaming.h"
#endif

enum topicType {
    T_DEV = 1,
    T_BCST= 2,
    T_OUT = 3
  };

void PrintBytes(uint8_t* addr, uint8_t count, bool newline);
void SetBytes(uint8_t* addr, uint8_t count, char * out);
void SetAddr(char * out,  uint8_t* addr);
uint8_t HEX2DEC(char i);
int getInt(char ** chan);
unsigned long freeRam ();
void parseBytes(const char* str, char separator, byte* bytes, int maxBytes, int base);
int log(const char *str, ...);
void printFloatValueToStr(float value, char *valstr);
void ReadUniqueID( uint32_t * pdwUniqueID );
int inet_aton(const char* aIPAddrString, IPAddress& aResult);
char *inet_ntoa_r(IPAddress addr, char *buf, int buflen);
void printIPAddress(IPAddress ipAddress);
char* setTopic(char* buf, int8_t buflen, topicType tt, const char* suffix = NULL);
void printUlongValueToStr(char *valstr, unsigned long value);
void scan_i2c_bus();
void softRebootFunc();
bool isTimeOver(uint32_t timestamp, uint32_t currTime, uint32_t time, uint32_t modulo = 0xFFFFFFFF);
//bool executeCommand(aJsonObject* cmd, int8_t toggle = -1, char* defCmd = NULL);
bool executeCommand(aJsonObject* cmd, int8_t toggle = -1);
bool executeCommand(aJsonObject* cmd, int8_t toggle, itemCmd _itemCmd);
itemCmd mapInt(int32_t arg, aJsonObject* map);

#define ledRED 1
#define ledGREEN 2
#define ledBLUE 4
#define ledBLINK 8
#define ledFASTBLINK 16
#define ledParams (ledRED | ledGREEN | ledBLUE | ledBLINK | ledFASTBLINK)

#define ledFlash 32
#define ledHidden 64

#define pinRED 50
#define pinGREEN 51
#define pinBLUE 52

#define ledDelayms 1000UL
#define ledFastDelayms 300UL

class statusLED {
public:
  statusLED(uint8_t pattern = 0);
  void set (uint8_t pattern);
  void show (uint8_t pattern);
  void poll();
  void flash(uint8_t pattern);
private:
  uint8_t curStat;
  uint32_t timestamp;
};
