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
#include "options.h"
#include "abstractout.h"
#include "itemCmd.h"

#define S_NOTFOUND  0
//#define S_SETnCMD 0
#define S_CMD 1
#define S_SET 2
#define S_HSV 3
#define S_RGB 4
#define S_FAN 5
#define S_MODE 6
#define S_HUE 7
#define S_SAT 8
#define S_TEMP 9
#define S_ADDITIONAL 9

#define CH_DIMMER 0   //DMX 1 ch
#define CH_RGBW   1   //DMX 4 ch
#define CH_RGB    2   //DMX 3 ch
#define CH_PWM    3   //PWM output directly to PIN
#define CH_MODBUS 4   //Modbus AC Dimmer
#define CH_THERMO 5   //Simple ON/OFF thermostat
#define CH_RELAY  6   //ON_OFF relay output
#define CH_GROUP   7  //Group pseudochannel
#define CH_VCTEMP  8  //Vacom PID regulator
#define CH_VC      9  //Vacom modbus motor regulator
#define CH_AC 10  //AC Haier
#define CH_SPILED 11
#define CH_MOTOR  12
#define CH_MBUS  14
//#define CHANNEL_TYPES 13

//static uint32_t pollInterval[CHANNEL_TYPES] = {0,0,0,0,MODB};
//static uint32_t nextPollTime[CHANNEL_TYPES] = {0,0,0,0,0,0,0,0,0,0,0,0,0};

#define CH_WHITE   127//




#define POLLING_SLOW 1
#define POLLING_FAST 2
#define POLLING_INT  3


#define I_TYPE 0 //Type of item
#define I_ARG  1 //Chanel-type depended argument or array of arguments (pin, address etc)
#define I_VAL  2 //Latest preset (int or array of presets)
#define I_CMD  3 //Latest CMD received
#define I_EXT  4 //Chanell-depended extension - array

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

class Item
{
  public:
  aJsonObject *itemArr, *itemArg,*itemVal,*itemExt;
  uint8_t itemType;
  abstractOut * driver;

  Item(char * name);
  Item(aJsonObject * obj);
  ~Item();

  boolean isValid ();
  boolean Setup();
  void Stop();
  //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL);
  int Ctrl(itemCmd cmd, char* subItem=NULL);
  int Ctrl(char * payload,  char * subItem=NULL);

  int getArg(short n=0);
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
  short getFlag   (short flag=FLAG_MASK);
  void setFlag   (short flag);
  void clearFlag (short flag);
  void setVal(long int par);
  void setSubtype(uint8_t par);
  int Poll(int cause);
  int SendStatus(int sendFlags);
  int isActive();
  int getChanType();
  inline int On (){return Ctrl(itemCmd(ST_VOID,CMD_ON));};
  inline int Off(){return Ctrl(itemCmd(ST_VOID,CMD_OFF));};
  inline int Toggle(){return Ctrl(itemCmd(ST_VOID,CMD_TOGGLE));};

  protected:
  //short cmd2changeActivity(int lastActivity, short defaultCmd = CMD_SET);
  int VacomSetFan (int8_t  val, int8_t  cmd=0);
  int VacomSetHeat(int8_t  val, int8_t  cmd=0);
  int modbusDimmerSet(int addr, uint16_t _reg, int _regType, int _mask, uint16_t value);
  int modbusDimmerSet(uint16_t value);
  void mb_fail();
  void Parse();
  int checkModbusDimmer();
  int checkModbusDimmer(int data);
  boolean checkModbusRetry();
  boolean checkVCRetry();
  boolean checkHeatRetry();
  void sendDelayedStatus();

  int checkFM();
  char defaultSubItem[10];
  int  defaultSuffixCode;

};
