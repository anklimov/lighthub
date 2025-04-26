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
#pragma once
#include "Arduino.h"
#include "aJSON.h"

typedef  char cmdstr[9];

const cmdstr commands_P[] PROGMEM =
{
"","ON","OFF","REST","TOGGLE","HALT","XON","XOFF","INCREASE","DECREASE",
"ENABLE","DISABLE","UNFREEZE","FREEZE",
"AUTO","FAN_ONLY",
"HIGH","MEDIUM","LOW",
"HEAT","COOL","DRY","STOP","RGB","HSV"
};

#define commandsNum sizeof(commands_P)/sizeof(cmdstr)

typedef  char ch_type[9];
const ch_type ch_type_P[] PROGMEM =
{
"DMX",    //    0   //DMX 1-4 ch
"DMXRGBW",//    1   //DMX 4 ch
"DMXRGB", //    2   //DMX 3 ch
"PWM",    //    3   //PWM output directly to PIN 1-4 CH
"MBUSDIM",//    4   //Modbus AC Dimmer
"THERMO", //    5   //Simple ON/OFF thermostat
"RELAY",  //    6   //ON_OFF relay output
"GROUP",  //    7   //Group pseudochannel
"VCTEMP", //    8   //Vacom PID regulator
"MBUSVC", //    9   //Vacom modbus motor regulator
"ACHAIER",//    10  //AC Haier
"SPILED", //    11  //SPI led strip
"MOTOR",  //    12  //Motorized gate with feedback resistor
"PID",    //    13  //PID regulator
"MBUS",   //    14  //Universal Modbus Master channel
"UARTBRDG", //  15  //Bridge between 2 UARTS with reporting PDUs to Wireshark via UDP
"RELAYPWM", //  16  //Slow PWM relay to control objects with inertia
"DMXRGBWW", //  17  //DMX RGBWW channel (warm&cold white)
"VENTS",    //  18  //Multichannel ventilation 
"ELEVATOR", //  19  //
"COUNTER",  //  20  //Generic counter
"HUM",      //  21  //Humidifier
"MERCURY"   //  22  //Mercury energy meter/RS485 interface
};

#define ch_typeNum sizeof(ch_type_P)/sizeof(ch_type)

/// Definition of Commands
#define CMD_ON  1       /// Turn channel ON 
#define CMD_OFF 2       /// Turn channel OFF
#define CMD_RESTORE 3   /// Turn ON only if was previously turned off by CMD_HALT
#define CMD_TOGGLE 4    /// Toggle ON/OFF
#define CMD_HALT 5      /// Just Off
#define CMD_XON 6       /// Just on
#define CMD_XOFF 7      /// OFF only if was previously turned on by CMD_XON
#define CMD_UP 8        /// increase
#define CMD_DN 9        /// decrease

#define CMD_ENABLE 0xa /// for PID regulator and XON/XOFF - chan limitation
#define CMD_DISABLE 0xb /// for PID regulator
#define CMD_UNFREEZE 0xc   ///
#define CMD_FREEZE 0xd /// 

#define CMD_AUTO 0xe    /// Thermostat/AC set to Auto mode 
#define CMD_FAN 0xf     /// AC set to Fan-only mode 

#define CMD_HIGH 0x10   /// AC/Vent fan level HIGH
#define CMD_MED 0x11    /// AC/Vent fan level MEDIUM
#define CMD_LOW 0x12    /// AC/Vent fan level LOW

#define CMD_HEAT 0x13    /// Thermostat/AC set to HEATing mode 
#define CMD_COOL 0x14    /// Thermostat/AC set to COOLing mode 
#define CMD_DRY 0x15     /// AC set to Dry mode
#define CMD_STOP 0x16    /// stop dimming (for further use)

#define CMD_RGB  0x17 
#define CMD_HSV  0x18

#define CMD_MASK 0xffUL
#define FLAG_MASK 0x00ffff00UL
//#define STATE_MASK 0xff0000

#define CMD_VOID 0
#define CMD_UNKNOWN  -1
#define CMD_JSON -2

//FLAGS

//#define FLAG_COMMAND 0x100UL
//#define FLAG_PARAMETERS 0x200UL
//#define FLAG_FLAGS 0x400UL
//#define FLAG_SEND_RETRY 0x800UL
//#define FLAG_SEND_DEFFERED 0x1000UL
//#define FLAG_SEND_DELAYED 0x2000UL
//#define FLAG_ACTION_NEEDED 0x4000UL
//#define FLAG_ACTION_IN_PROCESS 0x8000UL
//#define FLAG_DISABLED 0x10000UL
//#define FLAG_FREEZED 0x20000UL
//#define FLAG_HALTED 0x40000UL
//#define FLAG_XON 0x80000UL

#define FLAG_DISABLED 0x100UL
#define FLAG_LOCKED_CMD 0x200UL
#define FLAG_LOCKED_SET 0x400UL
#define FLAG_FREEZED 0x600UL

#define FLAG_SEND_DEFFERED 0x800UL
#define FLAG_COMMAND 0x1000UL
#define FLAG_PARAMETERS 0x2000UL
#define FLAG_FLAGS 0x4000UL

#define FLAG_SEND_RETRY 0x8000UL

#define FLAG_ACTION_NEEDED 0x10000UL
#define FLAG_ACTION_IN_PROCESS 0x20000UL
#define FLAG_HALTED 0x40000UL
#define FLAG_XON 0x80000UL
#define FLAG_SEND_DELAYED 0x100000UL


#define FLAG_SEND_IMMEDIATE 0x1UL
#define FLAG_NOT_SEND_CAN 0x2UL

int txt2cmd (char * payload);


///Definition of all possible types of argument, contained in class
#define ST_VOID         0      /// Not defined
#define ST_PERCENTS255  1      /// Percent value 0..255

#define ST_HSV255       2     /// HUE-SATURATION-VALUE representation of color (0..365, 0..100, 0..255)
#define ST_HS           3      /// just hue and saturation

#define ST_RGB          4      /// RGB replesentation of color
#define ST_RGBW         5      /// RGB + White channel

#define ST_TENS         6      /// Int representation of Float point value in tens part (ex 12.3 = 123 in "tens")
#define ST_FLOAT        7     /// generic Float value
#define ST_FLOAT_CELSIUS   8   /// Float value - temperature in Celsium
#define ST_FLOAT_FARENHEIT 9   /// Float value - temperature in Farenheit

#define ST_INT32        10     /// 32 bits signed integer
#define ST_UINT32       11     /// 32 bits unsigned integer
#define ST_STRING       12     /// pointer to string (for further use)
//#define ST_TIMESTAMP    13     /// Timestamp for delayed cmd execution

#define MAP_SCALE       1
#define MAP_VAL_CMD     2

#pragma pack(push, 1)

typedef union
{
  long int aslong;
  int32_t  asInt32;
  uint32_t asUint32;
  struct
      {
        uint8_t cmdCode;
        uint8_t  itemArgType:4;
        uint8_t  cmdEffect:4; //Reserve    
        uint8_t suffixCode; 
        uint8_t cmdParam;  //Reserve
      };
} itemCmdStore;

typedef union
{
  long int aslong;
  int32_t  asInt32;
  uint32_t asUint32;
  char*    asString;
  float    asfloat;

  struct
      { uint8_t  v;
        uint8_t  s;
        uint16_t h:9;
        uint16_t colorTemp:7;
      };   
  struct
      { int8_t  signed_v;
        int8_t  signed_s;
        int16_t signed_h:9;
        int16_t signed_colorTemp:7;
      };    
  struct  //Should be NeoPixel packed Color compatible 
      {
        uint8_t  b;
        uint8_t  g;
        uint8_t  r;
        uint8_t  w;
      };
} itemArgStore;

class Item;
class itemCmd
{
public:
  itemCmdStore      cmd;
  itemArgStore      param;

  itemCmd(uint8_t _type=ST_VOID, uint8_t _code=CMD_VOID);
  itemCmd(float val);
  itemCmd(Item *item);
  itemCmd(char *cmd);

  itemCmd assignFrom(itemCmd from, short chanType=-1);

  bool loadItem(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS);
  bool loadItemDef(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS );
  bool saveItem(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS);

  itemCmd Int(int32_t i);
  itemCmd uInt(uint32_t i);
  itemCmd Float(float f);
  itemCmd Tens(int32_t i);
  itemCmd Tens_raw(int32_t i);
  itemCmd Cmd(uint8_t i);
  itemCmd HSV(uint16_t h, uint8_t s, uint8_t v);
  itemCmd HSV255(uint16_t h, uint8_t s, uint8_t v);
  itemCmd HS(uint16_t h, uint8_t s);
  itemCmd RGB(uint8_t r, uint8_t g, uint8_t b);
  itemCmd RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  itemCmd Str(char * str);
  bool setH(uint16_t);
  bool setS(uint8_t);
  bool setColorTemp(int);
  int getColorTemp();
  uint16_t getH();
  uint16_t getS();
  itemCmd setArgType(uint8_t);
  itemCmd convertTo(uint8_t);
  uint8_t getStoragetypeByChanType(short chanType);

  itemCmd Percents(int i);
  itemCmd Percents255(int i);

  uint8_t getSuffix();
  itemCmd setSuffix(uint8_t suffix);

  bool incrementPercents(long int, long int limit);
  bool incrementH(long int);
  bool incrementS(long int);
  bool incrementTemp(long int dif);

  long int getInt();
  long int getTens();
  long int getTens_raw();
  float    getFloat();
  char *   getString();
  long int getSingleInt();
  short    getPercents(bool inverse=false);
  short    getPercents255(bool inverse=false);
  bool     setPercents(int percents);
  uint8_t    getCmd();
  uint8_t    getArgType();
  uint8_t    getCmdParam();
  char   * toString(char * Buffer, int bufLen, int sendFlags = FLAG_COMMAND | FLAG_PARAMETERS, bool scale100 = false);

  bool isCommand();
  bool isChannelCommand();
  bool isValue();
  bool isColor();

  itemCmd setDefault();
  itemCmd setChanType(short chanType);
  void debugOut();

  itemCmd doMapping(aJsonObject *mappingData);
  itemCmd doReverseMapping (aJsonObject *mappingData);
  bool scale100();

  };

int replaceTypeToInt(aJsonObject* verb);
#pragma pack(pop)
