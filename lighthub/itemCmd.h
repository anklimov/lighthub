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

/**
 * Textual names of item commands; array indexes match the CMD_* codes
 * (CMD_ON=1 ... CMD_RESET=0x19).
 *
 * - "ON" (1) - turn channel ON
 * - "OFF" (2) - turn channel OFF
 * - "REST" (3) - restore: turn ON only if previously halted (CMD_HALT)
 * - "TOGGLE" (4) - toggle ON/OFF
 * - "HALT" (5) - turn OFF, stays locked until REST
 * - "XON" (6) - momentary ON, stays on until XOFF
 * - "XOFF" (7) - release a channel turned on by XON
 * - "INCREASE" (8) / "DECREASE" (9) - step the value up / down
 * - "ENABLE" (0xA) - enable channel (for PID - enable regulation; for XON/XOFF - lift limitation)
 * - "DISABLE" (0xB) - disable channel
 * - "UNFREEZE" (0xC) / "FREEZE" (0xD) - unlock / lock commands and values
 * - "AUTO" (0xE) - thermostat/AC auto mode
 * - "FAN_ONLY" (0xF) - AC fan-only mode
 * - "HIGH" (0x10) / "MEDIUM" (0x11) / "LOW" (0x12) - AC/vent fan levels
 * - "HEAT_COOL" (0x13) - AC auto heat/cool mode
 * - "HEAT" (0x14) / "COOL" (0x15) / "DRY" (0x16) - thermostat/AC modes
 * - "RGB" (0x17) - set color in RGB notation
 * - "HSV" (0x18) - set color in HSV notation
 * - "RESET" (0x19) - reset device to defaults
 */
typedef  char cmdstr[10];

const cmdstr commands_P[] PROGMEM =
{
"","ON","OFF","REST","TOGGLE","HALT","XON","XOFF","INCREASE","DECREASE",
"ENABLE","DISABLE","UNFREEZE","FREEZE",
"AUTO","FAN_ONLY",
"HIGH","MEDIUM","LOW","HEAT_COOL",
"HEAT","COOL","DRY","RGB","HSV","RESET"
};

#define commandsNum sizeof(commands_P)/sizeof(cmdstr)

/**
 * Short textual names of channel types (CH_*); array indexes match
 * CH_DIMMER(0)..CH_SPRINKLER(23). Used to resolve "type":"DMX" etc. in config.
 */
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
"ELEVATOR", //  19  //Elevator controller
"COUNTER",  //  20  //Generic counter
"HUM",      //  21  //Humidifier
"MERCURY",   //  22  //Mercury energy meter/RS485 interface
"SPRINKLR"  //  23  //Sprinkler controller
};

#define ch_typeNum sizeof(ch_type_P)/sizeof(ch_type)

/// Definition of Commands
/**
 * Command codes (indexes of commands_P). Codes 1..9 are decimal,
 * 0xA..0x19 are hex. Commands up to CMD_FAN (0xF) are "channel commands"
 * that affect the whole channel state; the rest are device-specific
 * (AC modes, color, reset). CMD_VOID/CMD_UNKNOWN/CMD_JSON are special
 * non-wired codes used internally.
 */
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

#define CMD_HEATCOOL 0x13  /// 
#define CMD_HEAT 0x14    /// Thermostat/AC set to HEATing mode 
#define CMD_COOL 0x15    /// Thermostat/AC set to COOLing mode 
#define CMD_DRY 0x16     /// AC set to Dry mode

#define CMD_RGB  0x17 
#define CMD_HSV  0x18
#define CMD_RESET  0x19

/// Low byte of the packed I_CMD int - command code
#define CMD_MASK 0xffUL
/// High two bytes of the packed I_CMD int - state flags (FLAG_*)
#define FLAG_MASK 0x00ffff00UL
//#define STATE_MASK 0xff0000

#define CMD_VOID 0
#define CMD_UNKNOWN  -1
#define CMD_JSON -2

//FLAGS
/**
 * State flags packed with the command in the I_CMD item element (FLAG_MASK bits):
 * - FLAG_DISABLED - channel disabled (needs ENABLE to unlock)
 * - FLAG_LOCKED_CMD / FLAG_LOCKED_SET - frozen: commands / values are blocked
 * - FLAG_FREEZED - both locked
 * - FLAG_SEND_DEFFERED - status publish is pending (no network / low RAM)
 * - FLAG_COMMAND / FLAG_PARAMETERS / FLAG_FLAGS - what to include in the next status
 * - FLAG_SEND_RETRY - retry the failed Modbus transaction
 * - FLAG_ACTION_NEEDED / FLAG_ACTION_IN_PROCESS - driver action state machine
 * - FLAG_HALTED - turned off by CMD_HALT (CMD_RESTORE to return)
 * - FLAG_XON - turned on by CMD_XON (CMD_XOFF to release)
 * - FLAG_SEND_DELAYED - publish remaining time of a delayed command
 *
 * SendStatus options:
 * - FLAG_SEND_IMMEDIATE - do not defer the publish
 * - FLAG_NOT_SEND_CAN   - skip publishing to CAN
 */

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

/**
 * Translate a text payload to a command code.
 * @param payload - text command word, number or color ("ON", "-5", "#FF0000")
 * @return CMD_* code; CMD_VOID for plain numbers, CMD_UNKNOWN if unrecognized
 */
int txt2cmd (char * payload);


/**
 * Storage type codes (ST_*) - how the 4-byte itemCmd parameter word
 * is interpreted (value representation of a channel).
 */
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

/// Mapping options for doMapping()/doReverseMapping()
#define MAP_SCALE       1
#define MAP_VAL_CMD     2

#pragma pack(push, 1)

/**
 * @brief Packed 32-bit command word of #itemCmd.
 *
 * cmdCode - command code (CMD_*), itemArgType - storage type of the
 * parameter (ST_*), suffixCode - topic suffix (S_*), cmdEffect/cmdParam
 * are reserved.
 */
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

/**
 * @brief 32-bit parameter storage of #itemCmd.
 *
 * Holds a signed/unsigned number, float or string pointer; a HSV255 color
 * (h:9, s, v, colorTemp:7) and the same with signed fields; or a
 * NeoPixel-packed BGWR color (b, g, r, w) compatible layout.
 */
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
/**
 * @brief Command + value container passed between items, drivers and MQTT.
 *
 * itemCmd packs a command code (CMD_*), a typed value (ST_* storage) and a
 * topic suffix (S_*) into two 32-bit words (#itemCmdStore + #itemArgStore).
 * It is the universal currency of the control core: parsed from text or
 * JSON, converted to the channel's storage type, executed by the driver
 * and serialized back to text for status publishing.
 */
class itemCmd
{
public:
  /// Packed command word: code, storage type, suffix
  itemCmdStore      cmd;
  /// Packed parameter: number / float / string / color
  itemArgStore      param;

  /** Create a command of the given code with an empty parameter of the given storage type. */
  itemCmd(uint8_t _type=ST_VOID, uint8_t _code=CMD_VOID);
  /** Create a value with ST_FLOAT storage. */
  itemCmd(float val);
  /** Create a copy of the currently stored command/value of the item. */
  itemCmd(Item *item);
  /** Parse a text command/value ("ON", "50", "-1.5", "#FF0000", "HSV:100,50,255"). */
  itemCmd(char *cmd);

  /** Convert the value of `from` to the storage type appropriate for chanType and copy it to *this. */
  itemCmd assignFrom(itemCmd from, short chanType=-1);

  /** Load the stored command/value of the item into *this according to optionsFlag. */
  bool loadItem(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS);
  /** Like loadItem(), but falls back to channel defaults for a never-controlled item. */
  bool loadItemDef(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS );
  /** Write *this back to the item storage (command, value, flags) according to optionsFlag. */
  bool saveItem(Item * item, uint16_t optionsFlag=FLAG_PARAMETERS);

  /** Set the parameter to a signed 32-bit int (ST_INT32). */
  itemCmd Int(int32_t i);
  /** Set the parameter to an unsigned 32-bit int (ST_UINT32). */
  itemCmd uInt(uint32_t i);
  /** Set the parameter to a float (ST_FLOAT). */
  itemCmd Float(float f);
  /** Set the parameter to a fixed-point value in tens (ST_TENS, scaled by TENS_BASE). */
  itemCmd Tens(int32_t i);
  /** Set the parameter to a raw fixed-point value in tens (ST_TENS, pre-scaled). */
  itemCmd Tens_raw(int32_t i);
  /** Set the command code (and S_CMD suffix). */
  itemCmd Cmd(uint8_t i);
  /** Copy the command code and parameter from another itemCmd. */
  itemCmd Cmd(itemCmd i);
  /** Set an HSV255 color (hue 0..365, sat 0..100, value 0..255). */
  itemCmd HSV(uint16_t h, uint8_t s, uint8_t v);
  /** Set an HSV255 color (ST_HSV255 storage). */
  itemCmd HSV255(uint16_t h, uint8_t s, uint8_t v);
  /** Set hue and saturation only (ST_HS). */
  itemCmd HS(uint16_t h, uint8_t s);
  /** Set an RGB color (ST_RGB). */
  itemCmd RGB(uint8_t r, uint8_t g, uint8_t b);
  /** Set an RGBW color (ST_RGBW). */
  itemCmd RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  /** Set the parameter to a string pointer (ST_STRING). */
  itemCmd Str(char * str);
  /** Set the hue component (0..365). @return true on success. */
  bool setH(uint16_t);
  /** Set the saturation component (0..100). @return true on success. */
  bool setS(uint8_t);
  /** Set the color temperature in mireds (153..500). @return true on success. */
  bool setColorTemp(int);
  /** @return stored color temperature in mireds (153..500), -1 if none. */
  int getColorTemp();
  /** @return stored hue (0..365). */
  uint16_t getH();
  /** @return stored saturation (0..100). */
  uint16_t getS();
  /** Change the parameter storage type (ST_*). */
  itemCmd setArgType(uint8_t);
  /** Convert the stored value to the given storage type (ST_*). */
  itemCmd convertTo(uint8_t);
  /** @return the default storage type (ST_*) for the given channel type. */
  uint8_t getStoragetypeByChanType(short chanType);

  /** Set the parameter to a percent value 0..100 (ST_PERCENTS255 storage). */
  itemCmd Percents(int i);
  /** Set the parameter to a percent value 0..255 (ST_PERCENTS255 storage). */
  itemCmd Percents255(int i);

  /** @return the topic suffix code (S_*) of this command. */
  uint8_t getSuffix();
  /** Set the topic suffix code (S_*). */
  itemCmd setSuffix(uint8_t suffix);

  /** Increase/decrease a percent/int/float value by `step`, clamped to [0, limit]. */
  bool incrementPercents(long int, long int limit);
  /** Increase/decrease the hue by `step` (with wrap-around). */
  bool incrementH(long int);
  /** Increase/decrease the saturation by `step`. */
  bool incrementS(long int);
  /** Increase/decrease the temperature by `step`. */
  bool incrementTemp(long int dif);

  /** @return the parameter as a signed 32-bit int. */
  long int getInt();
  /** @return the parameter as a fixed-point value in tens. */
  long int getTens();
  /** @return the raw fixed-point parameter (no scaling). */
  long int getTens_raw();
  /** @return the parameter as a float. */
  float    getFloat();
  /** @return the parameter as a string (ST_STRING). */
  char *   getString();
  /** @return the parameter as a single int (first byte of a color, etc). */
  long int getSingleInt();
  /** @return the value as percent 0..100 (inverse flips low/high). */
  short    getPercents(bool inverse=false);
  /** @return the value as percent 0..255. */
  short    getPercents255(bool inverse=false);
  /** Set the value from a percent 0..100. @return true on success. */
  bool     setPercents(int percents);
  /** @return the command code (CMD_*). */
  uint8_t    getCmd();
  /** @return the parameter storage type (ST_*). */
  uint8_t    getArgType();
  /** @return the reserved cmdParam field. */
  uint8_t    getCmdParam();
  /**
   * Serialize the command and/or value to a text payload for MQTT.
   * @param sendFlags - FLAG_COMMAND and/or FLAG_PARAMETERS select what to print
   * @param scale100 - scale 0..255 values to 0..100
   */
  char   * toString(char * Buffer, int bufLen, int sendFlags = FLAG_COMMAND | FLAG_PARAMETERS, bool scale100 = false);

  /** @return true if this is a command (not just a value). */
  bool isCommand();
  /** @return true if this is a whole-channel command (not a subcommand). */
  bool isChannelCommand();
  /** @return true if this carries a value. */
  bool isValue();
  /** @return true if this carries a color (HSV/RGB/RGBW). */
  bool isColor();

  /** Set the value to the channel default for the current storage type. */
  itemCmd setDefault();
  /** Set the storage type appropriate for the given channel type. */
  itemCmd setChanType(short chanType);
  /** Dump the command to the debug serial. */
  void debugOut();

  /** Map the unified command/value to a device-specific value via `mappingData`. */
  itemCmd doMapping(aJsonObject *mappingData);
  /** Map a device-specific value back to the unified representation. */
  itemCmd doReverseMapping (aJsonObject *mappingData);
  /** Scale a 0..255 value to 0..100 in place. @return true on success. */
  bool scale100();

  };

/** @return the channel type code (CH_*) for a config "type" verb (string or int). */
int replaceTypeToInt(aJsonObject* verb);
#pragma pack(pop)
