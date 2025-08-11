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
    T_ROOT = 4,
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
uint32_t millisNZ(uint8_t shift=0, uint32_t mask=0xFFFFFFFFUL);
serialParamType  str2SerialParam(char * str);
String toString(const IPAddress& address);
bool getPinVal(uint8_t pin);
int  str2regSize(char * str);
bool checkToken(char * token, char * data);
 bool isProtectedPin(short pin);
 bool i2cReset();
 uint16_t getCRC(aJsonObject * in);
 char* getStringFromJson(aJsonObject * a, int i);
 char* getStringFromJson(aJsonObject * a, const char * name);
 long  getIntFromJson(aJsonObject * a, int i, long def = 0);
 long  getIntFromJson(aJsonObject * a, const char * name, long def = 0);
 float getFloatFromJson(aJsonObject * a, int i, float def = 0.0);
 float getFloatFromJson(aJsonObject * a, const char * name, float def = 0.0);

 

// Get object from array, create if absent and return pointer to object
 template<typename Type>
 aJsonObject * getCreateObject(aJsonObject * a, int n,  Type def); //set num value if created
 aJsonObject * getCreateObject(aJsonObject * a, int n); //just create null object if not find

// Get object from object by name, create absent find and return pointer to object
 template<typename Type>
 aJsonObject * getCreateObject(aJsonObject * a, const char * name, Type def); //set num value if created
 aJsonObject * getCreateObject(aJsonObject * a, const char * name); //just create null object if not find

 template<typename Type>
 aJsonObject * setValToJson(aJsonObject * a, int n, Type val);
 template<typename Type>
 aJsonObject * setValToJson(aJsonObject * a, const char * name, Type val);




template<typename Type>
aJsonObject * getCreateObject(aJsonObject * a, int n, Type val)
{
if (!a) return NULL;
if (a->type == aJson_Array) 
     {
      aJsonObject * element = aJson.getArrayItem(a, n);
      if (!element) 
            {
            for (int i = aJson.getArraySize(a); i < n; i++)
                  if (i==n-1) 
                        aJson.addItemToArray(a, element = aJson.createItem(val));
                  else  aJson.addItemToArray(a, element = aJson.createNull());    
            }        
        return element;    
       }     
 return NULL;         
}



 template<typename Type>
 aJsonObject * getCreateObject(aJsonObject * a, const char * name, Type def)
{
if (!a) return NULL;
if (a->type == aJson_Object)
     {
      aJsonObject * element = aJson.getObjectItem(a, name);
      if (!element) 
            {
            aJson.addNumberToObject(a, name, def);
            element = aJson.getObjectItem(a, name);    
            }
      return element;      
     }       
return NULL;       
}



 template<typename Type>
 aJsonObject * setValToJson(aJsonObject * a, const char * name, Type val)
{
aJsonObject * element = getCreateObject(a,name);
if (element)
  switch (element->type)
  { 
    case aJson_Float:  element->valuefloat = val;
    break;
    case aJson_NULL:   element->type = aJson_Int;
    case aJson_Int:    element->valueint = val;
  } 
return element;
}

 template<typename Type>
 aJsonObject * setValToJson(aJsonObject * a, int n, Type val)
{
aJsonObject * element = getCreateObject(a,n);

if (element)
  switch (element->type)
  {
    case aJson_Float:  element->valuefloat = val;
    break;
    case aJson_NULL:   element->type = aJson_Int;
    case aJson_Int:    element->valueint =val;
  }
return element;
}

#ifdef CANDRV
#include "util/crc16_.h"
class CRCStream : public Stream
{
public:
    CRCStream() : CRC16(0xFFFF){}
    uint16_t CRC16;
    uint16_t getCRC16() {return CRC16;}

    // Stream methods
    virtual int available(){return 0;};
    virtual int read(){return 0;};
    virtual int peek(){return 0;};

    virtual void flush(){};
    // Print methods
    virtual size_t write(uint8_t c) {CRC16 = crc16_update(CRC16, c);return 1;};
    virtual int availableForWrite(){return 1;};

};
#endif