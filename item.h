/* Copyright © 2017 Andrey Klimov. All rights reserved.

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
#define CH_WHITE   127// 

#define CMD_ON  1
#define CMD_OFF 2
#define CMD_HALT 5
#define CMD_RESTORE 3
#define CMD_TOGGLE 4
#define CMD_CURTEMP 127
#define CMD_SET 9

#define I_TYPE 0 //Type of item
#define I_ARG  1 //Chanel-type depended argument or array of arguments (pin, address etc) 
#define I_VAL  2 //Latest preset (int or array of presets)
#define I_CMD  3 //Latest CMD received
#define I_EXT  4 //Chanell-depended extension - array

#include "aJSON.h"

extern aJsonObject *items; 
 
int txt2cmd (char * payload);

typedef union 
{
  long int aslong;
  struct
      {
        int16_t h;
        int8_t  s;
        int8_t  v;        
      };
} HSVstore;

class Item 
{
  public:
  aJsonObject *itemArr, *itemArg,*itemVal;
  uint8_t itemType;
 

  Item(char * name);
  boolean isValid ();
  virtual int Ctrl(short cmd, short n=0, int * Par=NULL, boolean send=false);
  int getArg();
  boolean getEnableCMD(int delta);
  //int getVal(short n); //From VAL array. Negative if no array
  long int getVal(); //From int val OR array
  uint8_t getCmd();
  void setCmd(uint8_t cmd);
  //void setVal(uint8_t n, int par);
  void setVal(long int par);
  //void copyPar (aJsonObject *itemV);
  inline int On (){Ctrl(CMD_ON);};
  inline int Off(){Ctrl(CMD_OFF);};
  inline int Toggle(){Ctrl(CMD_TOGGLE);};
  int Pool ();
  int SendCmd(short cmd,short n=0, int * Par=NULL);
  
  protected:  
  int VacomSetFan (int addr, int8_t  val);
  int VacomSetHeat(int addr, int8_t  val, int8_t  cmd=0);

};



class PooledItem : public Item
{
  public:
  virtual int Changed() = 0;
  virtual void Idle ();
  protected:
  int PoolingInterval;
  unsigned long next;
  virtual int Pool() =0;
  
};



/*

class Vacon : public Item
{
public:
int Pool ();
virtual int Ctrl(short cmd, short n=0, int * Par=NULL);
protected:  
};

*/
