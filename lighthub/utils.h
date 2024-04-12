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

#if defined(ESP32)
#define serialParamType uint32_t
#else
#define serialParamType uint16_t
#endif

void PrintBytes(uint8_t* addr, uint8_t count, bool newline);
void SetBytes(uint8_t* addr, uint8_t count, char * out);
bool SetAddr(char * in,  uint8_t* addr);
uint8_t HEX2DEC(char i, bool* err);
long getIntFromStr(char **chan);
itemCmd getNumber(char ** chan);
unsigned long  freeRam ();
void parseBytes(const char* str, char separator, byte* bytes, int maxBytes, int base);
int log(const char *str, ...);
void printFloatValueToStr(char *valstr, float value);
uint32_t ReadUniqueID( uint32_t * pdwUniqueID );
int _inet_aton(const char* aIPAddrString, IPAddress& aResult);
char *_inet_ntoa_r(IPAddress addr, char *buf, int buflen);
void printIPAddress(IPAddress ipAddress);
char* setTopic(char* buf, int8_t buflen, topicType tt, const char* suffix = NULL);
void printUlongValueToStr(char *valstr, unsigned long value);
void scan_i2c_bus();
void softRebootFunc();
bool isTimeOver(uint32_t timestamp, uint32_t currTime, uint32_t time, uint32_t modulo = 0);
//bool executeCommand(aJsonObject* cmd, int8_t toggle = -1, char* defCmd = NULL);
bool executeCommand(aJsonObject* cmd, int8_t toggle = -1);
bool executeCommand(aJsonObject* cmd, int8_t toggle, itemCmd _itemCmd, aJsonObject* defaultItem=NULL, aJsonObject* defaultEmit=NULL, aJsonObject* defaultCan = NULL);
itemCmd mapInt(int32_t arg, aJsonObject* map);
unsigned long millisNZ(uint8_t shift=0);
serialParamType  str2SerialParam(char * str);
String toString(const IPAddress& address);
bool getPinVal(uint8_t pin);
int  str2regSize(char * str);
bool checkToken(char * token, char * data);
 bool isProtectedPin(short pin);
 bool i2cReset();

