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
#include "aJson.h"

typedef  char cmdstr[9];

const cmdstr commands_P[] PROGMEM =
{
"","ON","OFF","REST","TOGGLE","HALT","XON","XOFF","INCREASE","DECREASE",
"HEAT","COOL","AUTO","FAN_ONLY","DRY","STOP","HIGH","MEDIUM","LOW",
"TRUE","FALSE","ENABLED","DISABLED","RGB","HSV"
};
#define commandsNum sizeof(commands_P)/sizeof(cmdstr)

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
#define CMD_HEAT 0xa    /// Thermostat/AC set to HEATing mode 
#define CMD_COOL 0xb    /// Thermostat/AC set to COOLing mode 
#define CMD_AUTO 0xc    /// Thermostat/AC set to Auto mode 
#define CMD_FAN 0xd     /// AC set to Fan-only mode 
#define CMD_DRY 0xe     /// AC set to Dry mode
#define CMD_STOP 0xf    /// stop dimming (for further use)
#define CMD_HIGH 0x10   /// AC/Vent fan level HIGH
#define CMD_MED 0x11    /// AC/Vent fan level MEDIUM
#define CMD_LOW 0x12    /// AC/Vent fan level LOW
#define CMD_ENABLED 0x13 /// Aliase for ON
#define CMD_DISABLED 0x14 /// Aliase for OFF
#define CMD_TRUE 0x15   /// Aliase for ON
#define CMD_FALSE 0x16  /// Aliase for OFF
#define CMD_RGB  0x17 
#define CMD_HSV  0x18

#define CMD_MASK 0xff
#define FLAG_MASK 0xff00

#define CMD_VOID 0
#define CMD_UNKNOWN  -1
#define CMD_JSON -2

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

///Definition of all possible types of argument, contained in class
#define ST_VOID         0      /// Not defined
#define ST_PERCENTS     1      /// Percent value 0..100
#define ST_TENS         2      /// Int representation of Float point value in tens part (ex 12.3 = 123 in "tens")
#define ST_HSV          3      /// HUE-SATURATION-VALUE representation of color (0..365, 0..100, 0..100)
#define ST_HS           4      /// just hue and saturation
#define ST_FLOAT_CELSIUS   5   /// Float value - temperature in Celsium
#define ST_FLOAT_FARENHEIT 6   /// Float value - temperature in Farenheit
#define ST_RGB          7      /// RGB replesentation of color
#define ST_RGBW         8      /// RGB + White channel
#define ST_PERCENTS255  9      /// Percent value 0..255
#define ST_HSV255       10     /// HUE-SATURATION-VALUE representation of color (0..365, 0..255, 0..255)
#define ST_INT32        11     /// 32 bits signed integer
#define ST_UINT32       12     /// 32 bits unsigned integer
#define ST_STRING       13     /// pointer to string (for further use)
#define ST_FLOAT        14     /// generic Float value

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
            union {
                  uint8_t cmdFlag;
/*
                  struct
                      { uint8_t  suffixCode:4;
                        uint8_t  itemArgType:4;
                      };
                      */
                  };
                  struct
                      { uint8_t  suffixCode:4;
                        uint8_t  itemArgType:4;
                      };
    //    uint8_t cmdEffect;
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
  itemCmd assignFrom(itemCmd from);

  bool loadItem(Item * item, bool includeCommand=false );
  bool saveItem(Item * item, bool includeCommand=false);

  itemCmd Int(int32_t i);
  itemCmd Int(uint32_t i);
  itemCmd Tens(int32_t i);
  itemCmd Cmd(uint8_t i);
  itemCmd HSV(uint16_t h, uint8_t s, uint8_t v);
  itemCmd HSV255(uint16_t h, uint8_t s, uint8_t v);
  itemCmd HS(uint16_t h, uint8_t s);
  itemCmd RGB(uint8_t r, uint8_t g, uint8_t b);
  itemCmd RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  bool setH(uint16_t);
  bool setS(uint8_t);
  bool setColorTemp(int);
  int getColorTemp();
  uint16_t getH();
  uint16_t getS();
  itemCmd setArgType(uint8_t);
  itemCmd Percents(int i);
  itemCmd Percents255(int i);

  uint8_t getSuffix();
  itemCmd setSuffix(uint8_t suffix);

  bool incrementPercents(int16_t);
  bool incrementH(int16_t);
  bool incrementS(int16_t);

  long int getInt();
  long int getSingleInt();
  short    getPercents(bool inverse=false);
  short    getPercents255(bool inverse=false);
  uint8_t    getCmd();
  uint8_t    getArgType();
  uint8_t    getCmdParam();
  char   * toString(char * Buffer, int bufLen, int sendFlags = SEND_COMMAND | SEND_PARAMETERS );

  bool isCommand();
  bool isValue();
  bool isColor();

  itemCmd setDefault();
  itemCmd setChanType(short chanType);
  void debugOut();

  int doMapping(aJsonObject *mappingData);
  int doReverseMapping (aJsonObject *mappingData);


  };

#pragma pack(pop)
