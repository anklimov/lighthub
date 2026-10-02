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
 * @file utils.h
 * @brief Utility helpers of LightHub.
 *
 * Miscellaneous platform and protocol helpers: JSON (aJSON) getters and
 * object creation, command execution (executeCommand), MQTT topic building
 * (setTopic), number/byte/string parsing, IP address utilities, CRC16 of
 * JSON objects, pin access and logging.
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

/**
 * MQTT topic prefix types used by setTopic():
 * T_ROOT - root topic only, T_DEV - device topic, T_BCST - broadcast
 * (incoming) topic, T_OUT - output (status) topic.
 */
enum topicType {
    T_ROOT = 4,
    T_DEV = 1,
    T_BCST= 2,
    T_OUT = 3
  };

/// Type for a serial parameter (uint32 on ESP32, uint16 elsewhere)
#if defined(ESP32)
#define serialParamType uint32_t
#else
#define serialParamType uint16_t
#endif

/** Print `count` bytes from `addr` to the debug serial as hex. */
void PrintBytes(uint8_t* addr, uint8_t count, bool newline);
/** Format `count` bytes from `addr` as a hex string into `out`. */
void SetBytes(uint8_t* addr, uint8_t count, char * out);
/** Parse a hex string (AA:BB:...) into a MAC/EUI byte buffer. @return true on success. */
bool SetAddr(char * in,  uint8_t* addr);
/** Convert one hex digit to decimal, setting `err` on failure. */
uint8_t HEX2DEC(char i, bool* err);
/** Parse an integer from `*chan`, advancing the pointer past it (after ','). */
long getIntFromStr(char **chan);
/** Parse a typed number (int / tens / float) from `*chan` into an itemCmd. */
itemCmd getNumber(char ** chan);
/** @return current free heap size in bytes. */
unsigned long  freeRam ();
/** Parse a delimited hex string into a byte array. */
void parseBytes(const char* str, char separator, byte* bytes, int maxBytes, int base);
/** Log a formatted message to the debug serial. */
int log(const char *str, ...);
/** Convert a float to a fixed-format string for MQTT publishing. */
void printFloatValueToStr(char *valstr, float value);
/** Read the chip unique id (MAC) into 32-bit words. @return the id. */
uint32_t ReadUniqueID( uint32_t * pdwUniqueID );
/** Convert an IP address string to an IPAddress. @return true on success. */
int _inet_aton(const char* aIPAddrString, IPAddress& aResult);
/** Convert an IPAddress to a string in `buf`. */
char *_inet_ntoa_r(IPAddress addr, char *buf, int buflen);
/** Print an IP address to the debug serial. */
void printIPAddress(IPAddress ipAddress);
/** Build the full MQTT topic prefix of the given type in `buf`. @return buf. */
char* setTopic(char* buf, int8_t buflen, topicType tt, const char* suffix = NULL);
/** Convert an unsigned long to a string for MQTT publishing. */
void printUlongValueToStr(char *valstr, unsigned long value);
/** Scan the I2C bus and log the found devices. */
void scan_i2c_bus();
/** Perform a soft reboot of the device. */
void softRebootFunc();
/** @return true if (currTime - timestamp) >= time, with wrap-around safety. */
bool isTimeOver(uint32_t timestamp, uint32_t currTime, uint32_t time, uint32_t modulo = 0);
//bool executeCommand(aJsonObject* cmd, int8_t toggle = -1, char* defCmd = NULL);
/**
 * Execute a JSON command object with an empty itemCmd.
 * @param toggle - 0: use icmd/ecmd, 1: use irev/erev, -1: no toggle
 */
bool executeCommand(aJsonObject* cmd, int8_t toggle = -1);
/**
 * @brief Execute a JSON command object: control an item (Ctrl), publish
 *        an emit topic (MQTT) and/or send a CAN command.
 * @param cmd - command object: array (iterate), object with item/emit/can/
 *        icmd/irev/ecmd/erev/act fields, or a legacy string
 * @param toggle - 0: use icmd/ecmd, 1: use irev/erev, -1: no toggle
 * @param _itemCmd - itemCmd value to send when no command string is given
 * @param defaultItem/defaultEmit/defaultCan - fallback targets
 */
bool executeCommand(aJsonObject* cmd, int8_t toggle, itemCmd _itemCmd, aJsonObject* defaultItem=NULL, aJsonObject* defaultEmit=NULL, aJsonObject* defaultCan = NULL);
/** Map a number through a JSON map into an itemCmd. */
itemCmd mapInt(int32_t arg, aJsonObject* map);
/** @return millis() shifted and masked, never returning 0 or -1 (for timestamp storage). */
uint32_t millisNZ(uint8_t shift=0, uint32_t mask=0xFFFFFFFFUL);
/** Parse a serial parameter string (e.g. baud rate / parity) to its code. */
serialParamType  str2SerialParam(char * str);
/** @return an IPAddress as a string. */
String toString(const IPAddress& address);
/** @return the current output state of a pin (fast register read). */
bool getPinVal(uint8_t pin);
/** Parse a register size string (Modbus). */
int  str2regSize(char * str);
/** Validate an authorization token against data. @return true if the token is valid. */
bool checkToken(char * token, char * data);
 /** @return true if the pin is in the protected pins list (arming timer applies). */
 bool isProtectedPin(short pin);
 /** Recover the I2C bus by toggling SCL. @return true on success. */
 bool i2cReset();
 /** @return CRC16 of a JSON object (config integrity, CANDRV builds). */
 uint16_t getCRC(aJsonObject * in);
 /** @return the string element of a JSON array by index, or NULL. */
 char* getStringFromJson(aJsonObject * a, int i);
 /** @return the string field of a JSON object by name, or NULL. */
 char* getStringFromJson(aJsonObject * a, const char * name);
 /** @return the int element of a JSON array by index, or `def`. */
 long  getIntFromJson(aJsonObject * a, int i, long def = 0);
 /** @return the int field of a JSON object by name, or `def`. */
 long  getIntFromJson(aJsonObject * a, const char * name, long def = 0);
 /** @return the float element of a JSON array by index, or `def`. */
 float getFloatFromJson(aJsonObject * a, int i, float def = 0.0);
 /** @return the float field of a JSON object by name, or `def`. */
 float getFloatFromJson(aJsonObject * a, const char * name, float def = 0.0);
 /** @return the int field of a JSON object by name and array index, or `def`. */
 long getIntFromJson(aJsonObject * a, const char * name, int i,long def);
 /** @return the float field of a JSON object by name and array index, or `def`. */
float getFloatFromJson(aJsonObject * a, const char * name, int i, float def);

/** Configure a pin as an input. */
void setupInPin(short pin);
/** @return the (debounced) value of an input pin. */
bool readInPin(short pin);
/** Write a value to an output pin. */
void writeOutPin(short pin, bool val);

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
 aJsonObject * getCreateObject(aJsonObject * a, const char * name, int n, Type def)
{
if (!a) return NULL;
if (a->type == aJson_Object) return getCreateObject(a,name,def);
else if (a->type == aJson_Array) return getCreateObject(a,n,def);
else return NULL;
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
/**
 * @brief Print/Stream sink accumulating a CRC16 over the written bytes.
 *
 * Used together with aJson printing to compute the CRC of a config object
 * (CANDRV builds).
 */
class CRCStream : public Stream
{
public:
    CRCStream() : CRC16(0xFFFF){}
    /// Running CRC16 value
    uint16_t CRC16;
    /** @return the accumulated CRC16. */
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