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

typedef  char cmdstr[9];

const cmdstr commands_P[] PROGMEM =
{
"","ON","OFF","REST","TOGGLE","HALT","XON","XOFF","INCREASE","DECREASE",
"HEAT","COOL","AUTO","FAN_ONLY","DRY","STOP","HIGH","MEDIUM","LOW",
"TRUE","FALSE","ENABLED","DISABLED","RGB","HSV"
};
#define commandsNum sizeof(commands_P)/sizeof(cmdstr)

#define CMD_ON  1
#define CMD_OFF 2
#define CMD_RESTORE 3 //on only if was turned off by CMD_HALT
#define CMD_TOGGLE 4
#define CMD_HALT 5    //just Off
#define CMD_XON 6     //just on
#define CMD_XOFF 7    //off only if was previously turned on by CMD_XON
#define CMD_UP 8      //increase
#define CMD_DN 9      //decrease
#define CMD_HEAT 0xa
#define CMD_COOL 0xb
#define CMD_AUTO 0xc
#define CMD_FAN 0xd
#define CMD_DRY 0xe
#define CMD_STOP 0xf
#define CMD_HIGH 0x10  //AC fan leve
#define CMD_MED 0x11
#define CMD_LOW 0x12
#define CMD_ENABLED 0x13
#define CMD_DISABLED 0x14
#define CMD_TRUE 0x15
#define CMD_FALSE 0x16
#define CMD_RGB  0x17
#define CMD_HSV  0x18
//#define CMD_CURTEMP 0xf
#define CMD_MASK 0xff
#define FLAG_MASK 0xff00

#define CMD_VOID 0
#define CMD_UNKNOWN  -1
#define CMD_JSON -2
//#define CMD_RGB  -3
//#define CMD_HSV  -4

#define SEND_COMMAND 0x100
#define SEND_PARAMETERS 0x200
#define SEND_RETRY 0x400
#define SEND_DEFFERED 0x800
#define ACTION_NEEDED 0x1000
#define ACTION_IN_PROCESS 0x2000


int txt2cmd (char * payload);

/*
enum itemStoreType {
ST_VOID         = 0,
ST_PERCENTS     = 1,
ST_TENS         = 2,
ST_HSV          = 3,
ST_FLOAT_CELSIUS= 4,
ST_FLOAT_FARENHEIT= 5,
ST_RGB          = 6,
ST_RGBW         = 7,
ST_PERCENTS255  = 8,
ST_HSV255       = 9,
ST_INT32        = 10,
ST_UINT32       = 11,
ST_STRING       = 12,
ST_FLOAT        = 13//,
//ST_COMMAND      = 15
};
*/

#define ST_VOID         0
#define ST_PERCENTS     1
#define ST_TENS         2
#define ST_HSV          3
#define ST_FLOAT_CELSIUS   4
#define ST_FLOAT_FARENHEIT 5
#define ST_RGB          6
#define ST_RGBW         7
#define ST_PERCENTS255  8
#define ST_HSV255       9
#define ST_INT32        10
#define ST_UINT32       11
#define ST_STRING       12
#define ST_FLOAT        13


#pragma pack(push, 1)

typedef union
{
  long int aslong;
  int32_t  asInt32;
  uint32_t asUint32;
  struct
      {
        uint8_t cmdCode;
            union {
                  uint8_t cmdFlag;
                  struct
                      { uint8_t  suffixCode:4;
                        uint8_t  itemArgType:4;
                      };
                  };
        uint8_t cmdEffect;
        uint8_t cmdParam;
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
  struct
      {
        uint8_t  r;
        uint8_t  g;
        uint8_t  b;
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
  itemCmd assignFrom(itemCmd from);

  bool loadItem(Item * item, bool includeCommand=false );
  bool saveItem(Item * item, bool includeCommand=false);

  itemCmd Int(int32_t i);
  itemCmd Int(uint32_t i);
  itemCmd Cmd(uint8_t i);
  itemCmd HSV(uint16_t h, uint8_t s, uint8_t v);
  itemCmd RGB(uint8_t r, uint8_t g, uint8_t b);
  itemCmd RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  itemCmd setH(uint16_t);
  itemCmd setS(uint8_t);
  itemCmd setArgType(uint8_t);
  itemCmd Percents(int i);
  itemCmd Percents255(int i);

  uint8_t getSuffix();
  itemCmd setSuffix(uint8_t suffix);

  itemCmd incrementPercents(int16_t);
  itemCmd incrementH(int16_t);
  itemCmd incrementS(int16_t);

  long int getInt();
  short    getPercents(bool inverse=false);
  short    getPercents255(bool inverse=false);
  uint8_t    getCmd();
  uint8_t    getArgType();
  uint8_t    getCmdParam();
  char   * toString(char * Buffer, int bufLen);

  bool isCommand();
  bool isValue();
  bool isHSV();

  itemCmd setDefault();
  };

#pragma pack(pop)
