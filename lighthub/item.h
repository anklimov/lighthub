/* Copyright © 2017-2025 Andrey Klimov. All rights reserved.

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
#include "options.h"
#include "abstractout.h"
#include "itemCmd.h"


#define S_NOTFOUND  0
#define S_CMD  1
#define S_SET  2
#define S_VAL  3
#define S_DELAYED 4
#define S_HSV  5
#define S_RGB  6
#define S_FAN  7
#define S_MODE 8
#define S_CTRL 9
#define S_HUE  10
#define S_SAT  11
#define S_TEMP 12
#define S_RAW 13

typedef  char suffixstr[5];
const suffixstr suffix_P[] PROGMEM =
{"","cmd","set","val","del","HSV","RGB","fan","mode","ctrl","hue","sat","temp","raw"};
#define suffixNum sizeof(suffix_P)/sizeof(suffixstr)

#define S_ADDITIONAL 13

#define CH_DIMMER 0   //DMX 1-4 ch
#define CH_RGBW   1   //DMX 4 ch
#define CH_RGB    2   //DMX 3 ch
#define CH_PWM    3   //PWM output directly to PIN 1-4 CH
#define CH_MODBUS 4   //Modbus AC Dimmer
#define CH_THERMO 5   //Simple ON/OFF thermostat
#define CH_RELAY  6   //ON_OFF relay output
#define CH_GROUP   7  //Group pseudochannel
#define CH_VCTEMP  8  //Vacom PID regulator
#define CH_VC      9  //Vacom modbus motor regulator
#define CH_AC 10  //AC Haier
#define CH_SPILED 11
#define CH_MOTOR  12
#define CH_PID   13
#define CH_MBUS  14
#define CH_UARTBRIDGE  15
#define CH_RELAYX 16
#define CH_RGBWW    17
#define CH_MULTIVENT 18
#define CH_ELEVATOR 19
#define CH_COUNTER 20
#define CH_HUMIDIFIER 21
#define CH_MERCURY 22
#define CH_MAX 22

#define POLLING_SLOW 1
#define POLLING_FAST 2
#define POLLING_INT  3
#define POLLING_1S   4

//CTRL Execution flags
#define CTRL_DISABLE_RECURSION 1
#define CTRL_DISABLE_NON_GRP  2
#define CTRL_SCHEDULED_CALL_RECURSION (CTRL_DISABLE_RECURSION | CTRL_DISABLE_NON_GRP)

#define I_TYPE 0 //Type of item
#define I_ARG  1 //Chanel-type depended argument or array of arguments (pin, address etc)
#define I_VAL  2 //Latest preset (int or array of presets)
#define I_CMD  3 //Latest CMD received
#define I_EXT  4 //Chanell-depended extension - array
#define I_TIMESTAMP 5

#define MODBUS_CMD_ARG_ADDR 0
#define MODBUS_CMD_ARG_REG 1
#define MODBUS_CMD_ARG_MASK 2
#define MODBUS_CMD_ARG_MAX_SCALE 3
#define MODBUS_CMD_ARG_REG_TYPE 4

#define MODBUS_COIL_REG_TYPE 0
#define MODBUS_DISCRETE_REG_TYPE 1
#define MODBUS_HOLDING_REG_TYPE 2
#define MODBUS_INPUT_REG_TYPE 3

#include "aJSON.h"

extern aJsonObject *items;
extern short thermoSetCurTemp(char *name, float t);

int txt2cmd (char * payload);
int txt2subItem(char *payload);

class Item
{
  public:
  aJsonObject *rootItems, *itemArr, *itemArg,*itemVal,*itemExt;
  uint8_t itemType;
  abstractOut * driver;

  Item(char * name, aJsonObject *_items = items);
  Item(aJsonObject * obj, aJsonObject *_items = items);
  Item(uint16_t num, uint8_t subItem, aJsonObject *_items = items);
  ~Item();

  boolean isValid ();
  boolean Setup();
  void Stop();
  //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL);
  int Ctrl(itemCmd cmd, char* subItem=NULL, uint8_t flags = 0, bool authorized=false);
  int Ctrl(char * payload,  char * subItem=NULL, int remoteID = 0);
  int remoteCtrl(itemCmd cmd, int remoteID, char* subItem=NULL, char * authToken=NULL);
  int getArg(short n=0);
  float getFloatArg(short n=0);
  short getArgCount();
  //int getVal(short n); //From VAL array. Negative if no array
  long int getVal(); //From int val OR array
  uint8_t getSubtype();
  uint8_t getCmd();
  long int getExt(); //From int val OR array
  void setExt(long int par);
  chPersistent * getPersistent();
  chPersistent * setPersistent(chPersistent * par);
  void setCmd(uint8_t cmdValue);
  uint32_t getFlag   (uint32_t flag=FLAG_MASK);
  void setFlag   (uint32_t flag);
  void clearFlag (uint32_t flag);
  void setVal(long int par);
  void setFloatVal(float par);
  void setSubtype(uint8_t par);
  int Poll(int cause);
  int SendStatus(long sendFlags, char * subItem=NULL);
  int SendStatusImmediate(itemCmd st, long sendFlags, char * subItem=NULL);
  int isActive();
  int getChanType();
  inline int On (){return Ctrl(itemCmd(ST_VOID,CMD_ON));};
  inline int Off(){return Ctrl(itemCmd(ST_VOID,CMD_OFF));};
  inline int Toggle(){return Ctrl(itemCmd(ST_VOID,CMD_TOGGLE));};
  int scheduleCommand(itemCmd cmd, bool authorized);
  int scheduleOppositeCommand(itemCmd cmd,short isActiveNow,bool authorized);
  int isScheduled();
  char * getSubItemStrById(uint8_t subItem);
  uint8_t getSubitemId(char * subItem);

  protected:
  bool digGroup (aJsonObject *itemArr, itemCmd *cmd = NULL, char* subItem = NULL, bool authorized = false, uint8_t ctrlFlags = 0);
  long int limitSetValue();
  int VacomSetFan (itemCmd st);
  int VacomSetHeat(itemCmd st);
  int modbusDimmerSet(itemCmd st);
  int modbusDimmerSet(int addr, uint16_t _reg, int _regType, int _mask, uint16_t value);
  void mb_fail(int result=0);
  void Parse();
  int checkModbusDimmer();
  int checkModbusDimmer(int data);
  int checkRetry();
  void sendDelayedStatus();
  int checkFM();
  char defaultSubItem[16];
  int  defaultSuffixCode;
};

class driverFactory {
    public:
    driverFactory(){memset(drivers,0,sizeof(drivers));};
    Item * getItem(Item * item);
    abstractOut * getDriver(Item * item);
    abstractOut * findDriver(Item * item);
    void freeDriver(Item * item);
    abstractOut * newDriver(uint8_t itemType);
    private:
    abstractOut * drivers[CH_MAX+1];
};

typedef union
{
struct
  {
  int16_t  tempX100;
  uint16_t timestamp16;
  };
int32_t  asint;   
} thermostatStore;