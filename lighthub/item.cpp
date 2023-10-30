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

#include "options.h"
#include "item.h"
#include "aJSON.h"
#include "utils.h"
#include "textconst.h"
#include "main.h"
#include "bright.h"
#include "itemCmd.h"
//#include "SHA256.h"

#ifdef _dmxout
#include "dmx.h"
#ifdef ADAFRUIT_LED
#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif
#endif

#ifndef MODBUS_DISABLE
#include <ModbusMaster.h>
#endif

#include <PubSubClient.h>
#include "modules/out_spiled.h"
#include "modules/out_ac.h"
#include "modules/out_motor.h"
#include "modules/out_modbus.h"
#include "modules/out_dmx.h"
#include "modules/out_pwm.h"
#include "modules/out_pid.h"
#include "modules/out_multivent.h"
#include "modules/out_uartbridge.h"
#include "modules/out_relay.h"
#include "modules/out_counter.h"

#ifdef MERCURY_ENABLE
#include "modules/out_mercury.h"
#endif

#ifdef ELEVATOR_ENABLE
#include "modules/out_elevator.h"
#endif

#ifdef HUMIDIFIER_ENABLE
#include "modules/out_humidifier.h"
#endif

short modbusBusy = 0;
//bool isPendedModbusWrites = false;

extern aJsonObject *pollingItem;
extern PubSubClient mqttClient;
extern int8_t ethernetIdleCount;
extern int8_t configLocked;
extern lan_status lanStatus;

int retrieveCode(char **psubItem);


int subitem2cmd(char *payload) {
    int cmd = 0;

    // Check for command
    if (payload)
    {
    if (strcmp_P(payload, ON_P) == 0) cmd = CMD_ON;
    else if (strcmp_P(payload, OFF_P) == 0) cmd = CMD_OFF;
    //else if (strcmp_P(payload, REST_P) == 0) cmd = CMD_RESTORE;
    //else if (strcmp_P(payload, TOGGLE_P) == 0) cmd = CMD_TOGGLE;
    else if (strcmp_P(payload, HALT_P) == 0) cmd = CMD_HALT;
    else if (strcmp_P(payload, XON_P) == 0) cmd = CMD_XON;
    //else if (strcmp_P(payload, XOFF_P) == 0) cmd = CMD_XOFF;
    else if (strcmp_P(payload, HEAT_P) == 0) cmd = CMD_HEAT;
    else if (strcmp_P(payload, COOL_P) == 0) cmd = CMD_COOL;
    else if (strcmp_P(payload, AUTO_P) == 0) cmd = CMD_AUTO;
    else if (strcmp_P(payload, FAN_ONLY_P) == 0) cmd = CMD_FAN;
    else if (strcmp_P(payload, DRY_P) == 0) cmd = CMD_DRY;
    //else if (strcmp_P(payload, HIGH_P) == 0) cmd = CMD_HIGH;
    //else if (strcmp_P(payload, MED_P) == 0) cmd = CMD_MED;
    //else if (strcmp_P(payload, LOW_P) == 0) cmd = CMD_LOW;
    }
    return cmd;
}

int txt2subItem(char *payload) {
    int cmd = S_NOTFOUND;
    if (!payload || !strlen(payload)) return S_NOTFOUND;
    // Check for command
    if (strcmp_P(payload, SET_P) == 0) cmd = S_SET;
    else if (strcmp_P(payload, CTRL_P) == 0) cmd = S_CTRL;
    else if (strcmp_P(payload, CMD_P) == 0) cmd = S_CMD;
    else if (strcmp_P(payload, MODE_P) == 0) cmd = S_MODE;
    else if (strcmp_P(payload, HSV_P) == 0) cmd = S_HSV;
    else if (strcmp_P(payload, RGB_P) == 0) cmd = S_RGB;
    else if (strcmp_P(payload, FAN_P) == 0) cmd = S_FAN;
    else if (strcmp_P(payload, HUE_P) == 0) cmd = S_HUE;
    else if (strcmp_P(payload, SAT_P) == 0) cmd = S_SAT;
    else if (strcmp_P(payload, TEMP_P) == 0) cmd = S_TEMP;
    else if (strcmp_P(payload, VAL_P) == 0) cmd = S_VAL;
    else if (strcmp_P(payload, DEL_P) == 0) cmd = S_DELAYED;
    else if (strcmp_P(payload, _RAW_P) == 0) cmd = S_RAW;
    return cmd;
}

//const short defval[4] = {0, 0, 0, 0}; //Type,Arg,Val,Cmd

Item::Item(aJsonObject *obj)//Constructor
{
    itemArr = obj;
    driver = NULL;
    *defaultSubItem = 0;
    defaultSuffixCode = 0;
    Parse();
}

void Item::Parse() {
    if (isValid()) {
        // Todo - avoid static enlarge for every types
        for (int i = aJson.getArraySize(itemArr); i < 4; i++)
            aJson.addItemToArray(itemArr, aJson.createNull());//( (long int) 0));
                   // int(defval[i]) )); //Enlarge item to 4 elements. VAL=int if no other definition in conf
        //itemType = aJson.getArrayItem(itemArr, I_TYPE)->valueint;
        itemType = replaceTypeToInt (aJson.getArrayItem(itemArr, I_TYPE));
        itemArg = aJson.getArrayItem(itemArr, I_ARG);
        itemVal = aJson.getArrayItem(itemArr, I_VAL);
        itemExt = aJson.getArrayItem(itemArr, I_EXT);
        switch (itemType)
        {
#ifndef  PWM_DISABLE            
          case CH_PWM:
          driver = new out_pwm (this);
          break;
#endif          

#ifndef   DMX_DISABLE
            case CH_RGBW:
            case CH_RGB:
            case CH_DIMMER:
            case CH_RGBWW:
            driver = new out_dmx (this);
            break;
#endif
#ifndef   SPILED_DISABLE
          case CH_SPILED:
          driver = new out_SPILed (this);
          break;
#endif

#ifndef   AC_DISABLE
          case CH_AC:
          driver = new out_AC (this);
          break;
#endif

#ifndef   MOTOR_DISABLE
          case CH_MOTOR:
          driver = new out_Motor (this);
          break;
#endif

#ifndef   MBUS_DISABLE
          case CH_MBUS:
          driver = new out_Modbus (this);
          break;
#endif

#ifndef   PID_DISABLE
          case CH_PID:
          driver = new out_pid (this);
          break;
#endif


#ifndef   RELAY_DISABLE
          case CH_RELAYX:
          driver = new out_relay (this);
          break;
#endif

#ifndef   MULTIVENT_DISABLE
          case CH_MULTIVENT:
          driver = new out_Multivent (this);
          break;
#endif

#ifdef   UARTBRIDGE_ENABLE
          case CH_UARTBRIDGE:
          driver = new out_UARTbridge (this);
//          debugSerial<<F("AC driver created")<<endl;
          break;
#endif

#ifdef ELEVATOR_ENABLE
          case CH_ELEVATOR:
          driver = new out_elevator (this);
//          debugSerial<<F("AC driver created")<<endl;
          break;
#endif

#ifdef HUMIDIFIER_ENABLE
          case CH_HUMIDIFIER:
          driver = new out_humidifier (this);
          break;
#endif

#ifdef MERCURY_ENABLE
          case CH_MERCURY:
          driver = new out_Mercury (this);
          break;
#endif

#ifndef COUNTER_DISABLE
          case CH_COUNTER:
          driver = new out_counter (this);
//          debugSerial<<F("AC driver created")<<endl;
          break;
#endif
          default: ;
        }
//        debugSerial << F(" Item:") << itemArr->name << F(" T:") << itemType << F(" =") << getArg() << endl;
 }
}

boolean Item::Setup()
{

if (driver)
       {
        if (driver->Status()) driver->Stop();
        if (driver->Setup())
                {
                         if (getCmd()) setFlag(FLAG_COMMAND);
                         if (itemVal)  setFlag(FLAG_PARAMETERS);
                }
        return true;
       }
else return false;
}

void Item::Stop()
{
if (driver)
       {
        driver->Stop();
       }
return;
}

Item::~Item()
{
  if (driver)
              {
              delete driver;
              }
}

Item::Item(char *name) //Constructor
{
    char * pDefaultSubItem = defaultSubItem;
    driver = NULL;
    defaultSubItem[0] =0;
    defaultSuffixCode = 0;
    if (name && items)
    {   char* sub;
        if (sub=strchr(name,'/'))
        {
        char  buf [MQTT_SUBJECT_LENGTH+1];
        short i;
        for(i=0;(name[i] && (name[i]!='/') && (i<MQTT_SUBJECT_LENGTH));i++)
            buf[i]=name[i];
        buf[i]=0;
        itemArr = aJson.getObjectItem(items, buf);
        sub++;
        strncpy(defaultSubItem,sub,sizeof(defaultSubItem)-1);
        defaultSuffixCode = retrieveCode (&pDefaultSubItem);
        if (!pDefaultSubItem) defaultSubItem[0] =0; //Zero string
        //debugSerial<<F("defaultSubItem: ")<<defaultSubItem<<F(" defaultSuffixCode:")<<defaultSuffixCode<<endl;
        }
        else
        itemArr = aJson.getObjectItem(items, name);
    }
    else itemArr = NULL;
    Parse();
}


uint8_t Item::getCmd() {
    aJsonObject *t = aJson.getArrayItem(itemArr, I_CMD);
    if (t)
      return t->valueint & CMD_MASK;
    else return -1;
}


void Item::setCmd(uint8_t cmdValue) {
    aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
    if (itemCmd && (itemCmd->type == aJson_Int || itemCmd->type == aJson_NULL))
    {   
        itemCmd->type = aJson_Int;
        itemCmd->valueint = cmdValue & CMD_MASK | itemCmd->valueint & (FLAG_MASK);   // Preserve special bits
        debugSerial<<F("SetCmd:")<<cmdValue<<endl;
      }
}

uint32_t Item::getFlag   (uint32_t flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd && (itemCmd->type == aJson_Int))
  {
      return (uint32_t) itemCmd->valueint & flag & FLAG_MASK;
    }
return 0;
}

void Item::setFlag   (uint32_t flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd && (itemCmd->type == aJson_Int || itemCmd->type == aJson_NULL))
  {   
      itemCmd->type = aJson_Int;
      itemCmd->valueint |= flag & FLAG_MASK;   // Preserve CMD bits
    //  debugSerial<<F("SetFlag:")<<flag<<endl;
    }

}

void Item::clearFlag (uint32_t flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd && (itemCmd->type == aJson_Int || itemCmd->type == aJson_NULL))
  {
      itemCmd->valueint &= CMD_MASK  | ~(flag & FLAG_MASK);    // Preserve CMD bits
    //  debugSerial<<F("ClrFlag:")<<flag<<endl;
    }
}



int Item::getArg(short n) //Return arg int or first array element if Arg is array
{
    if (!itemArg) return 0;//-1;
    if (itemArg->type == aJson_Int){
        if (!n) return itemArg->valueint; else return 0;//-1;
    }
    if ((itemArg->type == aJson_Array) && ( n < aJson.getArraySize(itemArg))) return aJson.getArrayItem(itemArg, n)->valueint;
    else return 0;//-2;
}


float Item::getFloatArg(short n) //Return arg float or first array element if Arg is array
{
    if (!itemArg) return 0.0;//-1;
    if ((itemArg->type == aJson_Array) && ( n < aJson.getArraySize(itemArg))) 
           {
           aJsonObject * obj =  aJson.getArrayItem(itemArg, n);
           if (obj && obj->type == aJson_Int) return static_cast<float> (obj->valueint);
           if (obj && obj->type == aJson_Float) return obj->valuefloat;
           return 0.0;
           }

    else if (!n)
    { 
    if (itemArg->type == aJson_Int) return static_cast<float>(itemArg->valueint); 
    else  if (itemArg->type == aJson_Float) return itemArg->valuefloat; 
    } 
    return 0.0;
}

short Item::getArgCount()
{
  if (!itemArg) return 0;
  if (itemArg->type == aJson_Int) return 1;
  if (itemArg->type == aJson_Array) return aJson.getArraySize(itemArg);
      else return 0;
}
/*
int Item::getVal(short n) //Return Val from Value array
{ if (!itemVal) return -1;
      else if  (itemVal->type==aJson_Array)
            {
            aJsonObject *t = aJson.getArrayItem(itemVal,n);
            if (t)    return t->valueint;
               else   return -3;
            }
           else return -2;
}
*/

long int Item::getVal() //Return Val if val is int or first elem of Value array
{
    if (!itemVal) return 0;//-1;
    if (itemVal->type == aJson_Int) return itemVal->valueint;
    else if (itemVal->type == aJson_Array) {
        aJsonObject *t = aJson.getArrayItem(itemVal, 0);
        if (t) return t->valueint;
        else return 0;//-3;
    } else return 0;//-2;
}

uint8_t Item::getSubtype()
{
  if (!itemVal) return 0;//-1;
  if (itemVal->type == aJson_Int || itemVal->type == aJson_Float || itemVal->type == aJson_NULL) return itemVal->subtype;
  else if (itemVal->type == aJson_Array) {
      aJsonObject *t = aJson.getArrayItem(itemVal, 0);
      if (t) return t->subtype;
      else return 0;//-3;
  } else return 0;//-2;
}
/*
void Item::setVal(short n, int par)  // Only store  if VAL is array defined in config to avoid waste of RAM
{
  if (!itemVal || itemVal->type!=aJson_Array) return;
  debugSerial<<F(" Store p="));debugSerial<<n);debugSerial<<F(" Val="));debugSerial<<par);
  for (int i=aJson.getArraySize(itemVal);i<=n;i++) aJson.addItemToArray(itemVal,aJson.createItem(int(0))); //Enlarge array of Values

  aJsonObject *t = aJson.getArrayItem(itemVal,n);
  if (t) t->valueint=par;
}
*/

void Item::setVal(long int par)  // Only store if VAL is int (autogenerated or config-defined)
{
    if (!itemVal || (itemVal->type != aJson_Int && itemVal->type != aJson_Float && itemVal->type != aJson_NULL)) return;
    //debugSerial<<F(" Store ")<<F(" Val=")<<par<<endl;
    itemVal->valueint = par;
    itemVal->type = aJson_Int;
    if (itemVal->subtype==ST_TENS) itemVal->subtype=ST_INT32; //???
}

void Item::setFloatVal(float par)  // Only store if VAL is int (autogenerated or config-defined)
{
    if (!itemVal || (itemVal->type != aJson_Int && itemVal->type != aJson_Float && itemVal->type != aJson_NULL)) return;
    //debugSerial<<F(" Store ")<<F(" Val=")<<par<<endl;
    itemVal->valuefloat = par;
    itemVal->type = aJson_Float;
    if (itemVal->subtype==ST_TENS) itemVal->subtype=ST_FLOAT; //???
}

void Item::setSubtype(uint8_t par)  // Only store if VAL is int (autogenerated or config-defined)
{
    if (!itemVal || (itemVal->type != aJson_Int && itemVal->type != aJson_Float && itemVal->type != aJson_NULL)) return;
    itemVal->subtype = par & 0xF;
}

long int Item::getExt() //Return Val if val is int or first elem of Value array
{
    if (!itemExt) return 0;//-1;
    if (itemExt->type == aJson_Int) return itemExt->valueint;
    else if (itemExt->type == aJson_Array) {
        aJsonObject *t = aJson.getArrayItem(itemExt, 0);
        if (t) return t->valueint;
        else return 0;//-3;
    } else return 0;//-2;
}

void Item::setExt(long int par)  // Only store if VAL is int (autogenerated or config-defined)
{
    if (!itemExt)
    {
    for (int i = aJson.getArraySize(itemArr); i <= I_EXT; i++)
        aJson.addItemToArray(itemArr, itemExt=aJson.createNull());// Item((long int)0));
    //itemExt = aJson.getArrayItem(itemArr, I_EXT);
      };
    if(!itemExt ) return;
    if(itemExt->type == aJson_NULL) itemExt->type=aJson_Int;
       else if(itemExt->type != aJson_Int ) return;
    itemExt->valueint = par;
    debugSerial<<F("Stored EXT:")<<par<<endl;
}


chPersistent * Item::getPersistent()
{
    if (!itemExt) return NULL;
    if (itemExt->type == aJson_Int) return (chPersistent *) itemExt->child;
    else return NULL;
}

chPersistent * Item::setPersistent(chPersistent * par)
{
    if (!itemExt)
    {
    for (int i = aJson.getArraySize(itemArr); i <= 4; i++)
        aJson.addItemToArray(itemArr, itemExt = aJson.createNull());//Item((long int)0));
    //itemExt = aJson.getArrayItem(itemArr, I_EXT);
      };

    if(!itemExt ) return NULL;
    if(itemExt->type == aJson_NULL) itemExt->type=aJson_Int;
       else if(itemExt->type != aJson_Int ) return NULL;
    itemExt->valueint = 0;
    itemExt->child = (aJsonObject *) par;
  //  debugSerial<<F("Persisted.")<<endl;
    return par;
}



boolean Item::isValid() {
    return (itemArr && (itemArr->type == aJson_Array));
}

// If retrieving subitem code ok - return it
// parameter will point on the rest truncated part of subitem
// or pointer to NULL of whole string converted to subitem code

int retrieveCode(char **psubItem)
{
int   suffixCode;
char* suffix;
//debugSerial<<F("*psubItem:")<<*psubItem<<endl;
if (suffix = strrchr(*psubItem, '/')) //Trying to retrieve right part
  {
    *suffix= 0; //Truncate subItem string
    suffix++;
    suffixCode = txt2subItem(suffix);
    debugSerial<<F("suffixCode:")<<suffixCode<<endl;
    // myhome/dev/item/sub.....Item/suffix
    }
else
  {
    suffix = *psubItem;
    suffixCode = txt2subItem(suffix);
    if (suffixCode)  // some known suffix
          *psubItem = NULL;
          // myhome/dev/item/suffix

    else //Invalid suffix - fallback to Subitem notation
          suffix = NULL;
    // myhome/dev/item/subItem
  }
return suffixCode;
}

long int Item::limitSetValue()
{   
    int _itemType = itemType;
    if (driver) _itemType = driver->getChanType();
    switch (_itemType)
    {
        case CH_COUNTER:
            return 0;
        case CH_THERMO:
            return 80;
        default:
            return 255;    
    }
}


// myhome/dev/item/subItem
int Item::Ctrl(char * payload, char * subItem)
{
if (!payload) return 0;
int fr = freeRam();                           
if (fr < minimalMemory) 
    {
        errorSerial<<F("CTRL/txt: OutOfMemory: ")<<fr<<endl;
        return -1;
    }   

int   suffixCode = 0;

if ((!subItem || !strlen(subItem)) && strlen(defaultSubItem))
    subItem = defaultSubItem;  /// possible problem here with truncated default

if (subItem && strlen(subItem))
    suffixCode = retrieveCode(&subItem);

if (!suffixCode && defaultSuffixCode)
        suffixCode = defaultSuffixCode;

if (suffixCode == S_RAW)
{   itemCmd ic;
    ic.Str(payload);
    ic.setSuffix(suffixCode);
    return   Ctrl(ic,subItem);
}

bool authorized = false;
char * authPos = strchr(payload,'@');
if (authPos)
{
 *authPos=0;   
 authorized = checkToken(payload,authPos+1);
 payload=authPos+1;
}

int i=0;
while (payload[i]) {payload[i]=toupper(payload[i]);i++;};

int cmd = txt2cmd(payload);

itemCmd st(ST_VOID,cmd);

switch (suffixCode) {
    case S_HSV:
        cmd=CMD_HSV; 
    break;
    case S_RGB:
        cmd=CMD_RGB;
    break;
}
st.setSuffix(suffixCode);

  switch (cmd) {
      case CMD_HSV: 
          st.Cmd(0);
      case CMD_VOID:
       {
         //Parsing numbers from payload
          short i = 0;
          short k = 0;
          itemCmd Par0 = getNumber((char **) &payload);
          Par0.setSuffix(suffixCode);
          if (Par0.getArgType())  i++;

          int Par[3];        
          while (payload && k < 3)
              Par[k++] = getInt((char **) &payload);
          i=i+k; //i=total # of parameters
          switch(suffixCode)
              {case S_HUE:
                      st.setH(Par0.getInt());
                      break;
               case S_SAT:
                      st.setS(Par0.getInt());
                      break;
               case S_TEMP:
                      st.setColorTemp(Par0.getInt());
                      break;        
                default:
                    switch (i) //Number of params
                    {
                      case 1: 
                             st=Par0; 
                      break;
                      case 2: st.HS(Par0.getInt(),Par[0]);
                      break;
                      case 3: 
                              st.HSV255(Par0.getInt(),Par[0],Par[1]);   
                      break;
                      case 4:
                              st.HSV255(Par0.getInt(),Par[0],Par[1]);
                              st.setColorTemp(Par[2]);
                      break;        
                      default: errorSerial<<F("Wrong paylad")<<endl;
                    }
              }
 
          return   Ctrl(st,subItem,true,authorized);
        } //Void command
          break;

      case CMD_UNKNOWN: //Not known command
      case CMD_JSON: //JSON input (not implemented yet
          break;
      case CMD_RGB: //RGB color in #RRGGBB notation

      {  st.Cmd(0);
        //Parsing integers from payload
         short i = 0;
         int Par[4];
         while (payload && i < 4)
             Par[i++] = getInt((char **) &payload);

         switch (i) //Number of params
         {
           case 3: st.RGB(Par[0],Par[1],Par[2]);
           break;
           case 4: st.RGBW(Par[0],Par[1],Par[2],Par[3]);
           default:;
         }
         
         return   Ctrl(st,subItem,true,authorized);
      }
      case CMD_UP:
      case CMD_DN:
          {
          itemCmd Par0 = getNumber((char **) &payload);
          Par0.Cmd(cmd);
          Par0.setSuffix(suffixCode);
          return Ctrl(Par0, subItem,true,authorized);
          }
      default: //some known command
      {
      int32_t intParam = getInt((char **) &payload);
      if (intParam) st.Int(intParam);
      return Ctrl(st,NULL, true, authorized);
      }
  } //ctrl
return 0;
}

// Recursive function with small stack consumption
// if cmd defined - execute Ctrl for any group members recursively
// else performs Activity check for group members and return true if any member is active 
bool digGroup (aJsonObject *itemArr, itemCmd *cmd, char* subItem, bool authorized)
{    if (!itemArr || itemArr->type!=aJson_Array) return false; 
     // Iterate across array of names
      aJsonObject *i = itemArr->child;                                        
                configLocked++;
                while (i) {
                    if (i->type == aJson_String)
                    {   //debugSerial<< i->valuestring<<endl;
                        aJsonObject *nextItem = aJson.getObjectItem(items, i->valuestring);
                        if (nextItem && nextItem->type == aJson_Array) //nextItem is correct item
                               {    
                                Item it(nextItem);  
                                if (cmd && it.isValid()) it.Ctrl(*cmd,subItem,false,authorized); //Execute (non recursive)
                                //Retrieve itemType    
                                aJsonObject * itemtype = aJson.getArrayItem(nextItem,0);
                                if (itemtype && itemtype->type == aJson_Int  && itemtype->valueint == CH_GROUP) 
                                    { //is Group
                                    aJsonObject * itemSubArray = aJson.getArrayItem(nextItem,1);
                                    short res = digGroup(itemSubArray,cmd,subItem,authorized);
                                    if (!cmd && res) 
                                                    {
                                                    configLocked--;    
                                                    return true; //Not execution, just activity check. If any channel is active - return true
                                                    }
                                    }
                                else // Normal channel
                                if (!cmd && it.isValid() && it.isActive()) 
                                                    {
                                                    configLocked--;    
                                                    return true; //Not execution, just activity check. If any channel is active - return true
                                                    }
                               } 
                    }    
                    i = i->next;
                } //while
                configLocked--;
return false;
}


int Item::isScheduled()
{
aJsonObject *timestampObj = aJson.getArrayItem(itemArr, I_TIMESTAMP);
                if (timestampObj && (timestampObj->subtype > 0))
                {     
                 return timestampObj->subtype;
                }
return 0;
}

int Item::scheduleOppositeCommand(itemCmd cmd,bool isActiveNow,bool authorized)
{
    itemCmd nextCmd=cmd;
    
    switch (cmd.getCmd()){
    case CMD_XON:  
                    if (isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_XOFF);
    break;
    case CMD_XOFF:  
                    if (!isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_XON);
    break;
    case CMD_ON:  
                    if (isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_OFF);
    break;
    case CMD_OFF:  
                    if (!isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_ON);
    break;
    case CMD_ENABLE:  
                    if (!getFlag(FLAG_DISABLED) && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_DISABLE);
    break;
    case CMD_DISABLE:  
                    if (getFlag(FLAG_DISABLED) && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_ENABLE);
    break;
    case CMD_FREEZE:  
                    if (getFlag(FLAG_FREEZED) && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_UNFREEZE);
    break;
    case CMD_UNFREEZE:  
                    if (!getFlag(FLAG_FREEZED) && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_FREEZE);
    break;
    case CMD_HALT:  
                    if (!isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_RESTORE);
    break;
    case CMD_RESTORE:  
                    if (isActiveNow && !isScheduled()) return 0;
                    nextCmd.Cmd(CMD_HALT);
    break;
    case CMD_TOGGLE:  nextCmd.Cmd(CMD_TOGGLE);
    break;
   
    default: return 0;
    }
     debugSerial<<F("CTRL: Schedule cmd ")<<nextCmd.getCmd()<<F(" for (ms):")<<nextCmd.getInt()<<endl;
    scheduleCommand(nextCmd,authorized);
    return 1;
}  

int Item::scheduleCommand(itemCmd cmd,bool authorized)
{
    if (!itemArr)     return 0;

                for (int i = aJson.getArraySize(itemArr); i <= I_TIMESTAMP; i++)
                            aJson.addItemToArray(itemArr, aJson.createNull());
            
                aJsonObject *timestampObj = aJson.getArrayItem(itemArr, I_TIMESTAMP);
                if (timestampObj && cmd.getCmd()<=0xf)
                { 
                   if ((cmd.getInt()>0) && (timestampObj->type == aJson_Int || timestampObj->type == aJson_NULL || timestampObj->type == aJson_Reserved))       
                    {
                        timestampObj->valueint = millis()+cmd.getInt();
                        timestampObj->type = (authorized?aJson_Reserved:aJson_Int);
                        timestampObj->subtype=(cmd.getCmd() & 0xF);
                
                        debugSerial<<F( "Armed for ")<< cmd.getInt() << F(" ms :")<<timestampObj->valueint<<endl;
                    }
                    else
                    {
                        timestampObj->subtype=0;
                        debugSerial<<F( " Disarmed")<<endl;
                    }         
                return 1;
                }        
         return 0;              

}
//Main routine to control Item
//Return values
// 1  complete
// 3  complete, no action, target reached
// -3 ignored
// -1 system error
// -4 invalid argument
// -5 unauthorized
int Item::Ctrl(itemCmd cmd,  char* subItem, bool allowRecursion, bool authorized)
{      
       int fr = freeRam();
       if (fr < minimalMemory) 
        {
        errorSerial<<F("CTRL: OutOfMemory: ")<<fr<<endl;
        return -1;
        }
  {      
  uint32_t time=millis();  
  int suffixCode = cmd.getSuffix();
  bool operation = isNotRetainingStatus();
  
    if ((!subItem || !strlen(subItem)) && strlen(defaultSubItem))
        subItem = defaultSubItem;  /// possible problem here with truncated default

    if (!suffixCode && subItem && strlen(subItem))
        {    
        suffixCode = retrieveCode(&subItem);
        if (!cmd.getSuffix()) cmd.setSuffix(suffixCode);
        }

    if (!suffixCode && defaultSuffixCode)
        {
        suffixCode = defaultSuffixCode;
        if (!cmd.getSuffix()) cmd.setSuffix(suffixCode);
        }    
    
    
    
    debugSerial<<F("CTRL: RAM=")<<fr<<F(" Item=")<<itemArr->name<<F(" Sub=")<<subItem<<F(" ");

    cmd.debugOut();
    //debugSerial<<endl; 
    if (subItem && subItem[0] == '$') {debugSerial<<F("Skipped homie stuff")<<endl;return -4; }
    if (!itemArr) return -1;
    
    /// DELAYED COMMANDS processing
     if (suffixCode == S_DELAYED)
        {
            return scheduleCommand(cmd,authorized);
        }
    /// 
    int8_t  chActive  = -1;
    bool    toExecute = false;
    bool    scale100  = false;
    bool    invalidArgument = false;
    int     res       = -1;
    uint16_t status2Send = 0;
    uint8_t  command2Set = 0;

    /// Common (GRP & NO GRP) commands
    switch (cmd.getCmd()) 
    {
            case CMD_VOID: // No commands, just set value
                         {   
                         itemCmd stored;
                         if (!cmd.isValue()) break;
                               switch (suffixCode)
                               {
                                     case S_NOTFOUND: //For empty (universal) OPENHAB suffix - turn ON/OFF automatically
                                     if (subItem) {debugSerial<<F("CTRL: Unknown suffix")<<endl; break;}; // if some unknown suffix
                                     toExecute=true;
                                     scale100=true;  //openHab topic format
                                     chActive=(isActive()>0);
                                        debugSerial<<chActive<<" "<<cmd.getInt()<<endl;
                                     if (chActive>0 && !cmd.getInt()) {cmd.Cmd(CMD_OFF);status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE;}
                                     if (chActive==0 && cmd.getInt()) {cmd.Cmd(CMD_ON);status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE;}
            
                                     // continue processing as SET
                                     case S_SET:
                                     stored.loadItemDef(this);
                                     // if previous color was in RGB notation but new value is HSV - discard previous val and change type;   
                                     if ((stored.getArgType() == ST_RGB || stored.getArgType() == ST_RGBW) &&
                                        (cmd.getArgType() == ST_HSV255)) 
                                                                stored.setArgType(cmd.getArgType());

                                     if (itemType == CH_GROUP && cmd.isColor()) stored.setArgType(ST_HSV255);//Extend storage for group channel
                                     
                                      //Convert value to most approptiate type for channel
                                     stored.assignFrom(cmd,getChanType());
                                     stored.debugOut();

                                     if ((scale100 || SCALE_VOLUME_100) && (cmd.getArgType()==ST_HSV255 || cmd.getArgType()==ST_PERCENTS255 || cmd.getArgType()==ST_INT32 || cmd.getArgType()==ST_UINT32)) 
                                            stored.scale100();
                                     cmd=stored;
                                     status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;  

                                     break;
                                     case S_VAL:
                                     break;

                                     case S_SAT:
                                     stored.loadItemDef(this);
                                     if (stored.setS(cmd.getS()))
                                        { 
                                          cmd=stored;
                                          cmd.setSuffix(S_SET);  
                                          status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;  
                                        } else invalidArgument=true;
                                     break;

                                     case S_HUE:
                                     stored.loadItemDef(this);
                                     if (stored.setH(cmd.getH()))
                                        {
                                          cmd=stored;  
                                          cmd.setSuffix(S_SET);   
                                          status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;    
                                        } else invalidArgument=true;
                                    break;
                                    case S_TEMP:
                                    stored.loadItemDef(this);
                                    stored.setColorTemp(cmd.getColorTemp());
                                    cmd=stored;
                                    status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;  
                                   }

                        }   
                        break;
              case CMD_TOGGLE:
                if (suffixCode != S_CTRL)
                  {
                  chActive=(isActive()>0);
                  toExecute=true;

                  if (chActive) cmd.Cmd(CMD_OFF);
                      else 
                            { 
                             // cmd.loadItemDef(this);  ///
                              cmd.Cmd(CMD_ON);
                            }   
                  status2Send |=FLAG_COMMAND | FLAG_SEND_IMMEDIATE;    
                  }
                 else
                  {
                    if (getFlag(FLAG_DISABLED)) clearFlag(FLAG_DISABLED); else setFlag(FLAG_DISABLED);
                    status2Send |= FLAG_FLAGS | FLAG_SEND_IMMEDIATE;
                    res=1;
                  } 
                break;


              case CMD_DN:
              case CMD_UP:
                {
                itemCmd fallbackCmd=cmd;    
                long step=0;    
                if (cmd.isValue()) step=cmd.getTens_raw();
                if (!step) step=DEFAULT_INC_STEP;
                if (cmd.getCmd() == CMD_DN) step=-step;    

                cmd.loadItemDef(this); 

                cmd.Cmd(CMD_VOID); // Converting to SET value command
                
                      switch (suffixCode)
                      {
                        case S_NOTFOUND:
                             toExecute=true;
                        case S_SET:
                          {
                             long limit = limitSetValue();
                             //if (limit && suffixCode==S_NOTFOUND) limit = 100;    
                             if (cmd.incrementPercents(step,limit))
                             {  
                               status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;    
                             } else {cmd=fallbackCmd;invalidArgument=true;}
                          }  
                        break;
                      
                        case S_HUE:
                             if (cmd.incrementH(step))
                             {
                               status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;     
                               cmd.setSuffix(S_SET);      
                             } else {cmd=fallbackCmd;invalidArgument=true; errorSerial << F("Invalid arg")<<endl;}
                             break;
                        case S_SAT:
                             if (cmd.incrementS(step))
                             {
                               status2Send |= FLAG_PARAMETERS | FLAG_SEND_DEFFERED;  
                               cmd.setSuffix(S_SET);  
                             } else {cmd=fallbackCmd;invalidArgument=true; errorSerial << F("Invalid arg")<<endl;}
                      } //switch suffix

             } //Case UP/DOWN
             break;   
    // 
    }
    if (itemType==CH_GROUP) 
    {
    bool scheduledOppositeCommand = false;    
    if (fr<350)
            {
                errorSerial<<F("CTRL: Not enough memory for group operation")<<endl;
                return -1;
            }    
    if (allowRecursion && itemArg->type == aJson_Array && operation) 
                {
                chActive=(isActive()>0);    
                digGroup(itemArg,&cmd,subItem,authorized);   

                if ((suffixCode==S_CMD) &&  cmd.isValue())  
                                            {
                                            scheduleOppositeCommand(cmd,chActive,authorized);         
                                            scheduledOppositeCommand = true;
                                            }   
                }
       res=1;     

    
      // Post-processing of group command - converting HALT,REST,XON,XOFF to conventional ON/OFF for status
      switch (cmd.getCmd()) {
              int t;
              case CMD_RESTORE:  // individual for group members
                      switch (t = getCmd()) {
                          case CMD_HALT: //previous command was HALT ?
                              ///if ((suffixCode==S_CMD) && cmd.isValue() && (!chActive || isScheduled())) scheduleOppositeCommand(cmd,authorized);
                              debugSerial << F("CTRL: Restored from:") << t << endl;
                              cmd.loadItemDef(this);
                              cmd.Cmd(CMD_ON);    //turning on
                              status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                              break;
                          default:
                              return 3;
                      }
                  status2Send |= FLAG_COMMAND;     
                  break;
              case CMD_XOFF:    // individual for group members
                      switch (t = getCmd()) {
                          case CMD_XON: //previous command was CMD_XON ?
                             ///if ((suffixCode==S_CMD) && cmd.isValue() && (chActive || isScheduled())) scheduleOppositeCommand(cmd,authorized);
                              debugSerial << F("CTRL: Turned off from:") << t << endl;
                              cmd.Cmd(CMD_OFF);    //turning Off
                              status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                              break;
                          default:
                              debugSerial << F("CTRL: XOFF skipped. Prev cmd:") << t <<endl;
                            return 3;
                    }
                    break;
          
            case CMD_XON:
            
            if (!getFlag(FLAG_DISABLED))
            {
            if (chActive == -1) chActive=(isActive()>0);

            ///if ((suffixCode==S_CMD) && cmd.isValue() && (!chActive || isScheduled())) scheduleOppositeCommand(cmd,authorized);
   

            if (!chActive)  //if channel was'nt active before CMD_XON
                {


                    cmd.loadItemDef(this);
                    cmd.Cmd(CMD_ON);
                    command2Set=CMD_XON;
                    status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE;  
                  }
              else
              { 
                debugSerial<<F("CTRL: XON:Already Active\n");
                return 3;
              }
            }           
                        else 
                { 
                    debugSerial<<F("CTRL: XON: Disabled\n");
                    return 3;
                }

            break;
            case CMD_HALT:
            if (chActive == -1) chActive=(isActive()>0);
            ///if ((suffixCode==S_CMD) && cmd.isValue() && (chActive || isScheduled())) scheduleOppositeCommand(cmd,authorized);
            if (chActive)  //if channel was active before CMD_HALT /// HERE bug - if cmd == On but 0 = active
                  {
                    cmd.Cmd(CMD_OFF);  
                    command2Set=CMD_HALT;
                    status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                  }
            else    
                  {
                    debugSerial<<F("CTRL: HALT:Already Inactive\n");
                    return 3;
                  }
            break;
      }

     status2Send |= FLAG_SEND_IMMEDIATE;  
     if (cmd.isChannelCommand()) status2Send |= FLAG_COMMAND;       
     if (cmd.isValue() || cmd.loadItem(this,FLAG_PARAMETERS)) status2Send |= FLAG_PARAMETERS; 

     if (scheduledOppositeCommand) status2Send &=~FLAG_PARAMETERS;
                                                                                    
    // UPDATE internal variables for GROUP
    if (status2Send) cmd.saveItem(this,status2Send);
    } // end GROUP
    
    else
    {  // NON GROUP part
        if (chActive<0) chActive  = (isActive()>0);
        toExecute=chActive || toExecute; // Execute if active 
        debugSerial<<F("CTRL: Active:")<<chActive<<F( " Exec:")<<toExecute<<endl;
        if (subItem) 
        {
        //Check if subitem is some sort of command    
        int subitemCmd = subitem2cmd(subItem);
        short prevCmd  = getCmd();
        if (!prevCmd && chActive) prevCmd=CMD_ON;

            if (subitemCmd) //Find some COMMAND in subitem
                {           
                if (subitemCmd != prevCmd)
                    {
                        debugSerial<<F("CTRL: Ignored, channel cmd=")<<prevCmd<<endl;
                        return -3;
                    }
                subItem=NULL;   
                }
            else // Fast track for commands to subitems
            {   
                if (driver) return driver->Ctrl(cmd,subItem,toExecute,authorized);
               ///// return 0;
            }
        }
        bool oppositeCommandToBeSchedulled = (suffixCode==S_CMD) && allowRecursion && cmd.isValue();
        // Commands for NON GROUP
        //threating Restore, XOFF (special conditional commands)/ convert to ON, OFF and SET values
          switch (cmd.getCmd()) {
              int t;
              case CMD_RESTORE:  // individual for group members
                      switch (t = getCmd()) {
                          case CMD_HALT: //previous command was HALT ?
                             // if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
                             //                                                        scheduleOppositeCommand(cmd,chActive,authorized);

                              debugSerial << F("CTRL: Restored from:") << t << endl;
                              cmd.loadItemDef(this);
                              toExecute=true;
                              if (itemType == CH_THERMO) cmd.Cmd(CMD_AUTO); ////
                                  else cmd.Cmd(CMD_ON);    //turning on
                              status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE;     
                              break;
                          default:
                              return 3;
                      }
                  status2Send |= FLAG_COMMAND;     
                  break;
              case CMD_XOFF:    // individual for group members
                      switch (t = getCmd()) {
                          case CMD_XON: //previous command was CMD_XON ?
                            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
                            //                                                         scheduleOppositeCommand(cmd,chActive,authorized);
                              debugSerial << F("CTRL: Turned off from:") << t << endl;
                              toExecute=true;
                              cmd.Cmd(CMD_OFF);    //turning Off
                              clearFlag(FLAG_XON);
                              status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                              break;
                          default:
                              debugSerial << F("CTRL: XOFF skipped. Prev cmd:") << t <<endl;
                            return 3;
                    }
                    break;
          
            case CMD_XON:
            if (!getFlag(FLAG_DISABLED))
            {
                //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
                //                                                                     scheduleOppositeCommand(cmd,chActive,authorized);

                if (!chActive)  //if channel was'nt active before CMD_XON
                    {

                        cmd.loadItemDef(this);
                        cmd.Cmd(CMD_ON);
                        command2Set=CMD_XON;
                        setFlag(FLAG_XON);
                        status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE;  
                        toExecute=true;
                    }
                else
                { 
                    debugSerial<<F("CTRL: XON:Already Active\n");
                    return 3;
                }
            }
            else 
                { 
                    debugSerial<<F("CTRL: XON: Disabled\n");
                    return 3;
                }
            break;

            case CMD_HALT:
            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
            if (chActive)  //if channel was active before CMD_HALT
                  {
                    cmd.Cmd(CMD_OFF);  
                    command2Set=CMD_HALT;
                    status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                    toExecute=true;
                  }
            else    
                  {
                    debugSerial<<F("CTRL: HALT:Already Inactive\n");
                    return 3;
                  }
            break;
            
            case CMD_OFF:
                if (!cmd.isChannelCommand() )  //Command for driver, not for whole channel    
                {
                    toExecute=true;
                    break;
                }

            if (getCmd() == CMD_HALT) return 3; //Halted, ignore OFF
            
            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
            status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
            toExecute=true;
            break;

            case CMD_ON:

            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);

                if (!cmd.isChannelCommand())  //Command for driver, not for whole channel    
                {
                    toExecute=true;
                    break;
                }
            if (chActive) 
                {
                        debugSerial<<F("CTRL: ON:Already Active\n");
                        setCmd(CMD_ON);
                        SendStatus(FLAG_COMMAND | FLAG_SEND_DEFFERED);
                        return 3;
                }

            //newly added. For climate commands need to restore previous temperature 
            case CMD_AUTO:
            case CMD_COOL:
            case CMD_HEAT:
            case CMD_FAN:
            case CMD_DRY:
            if (!cmd.isChannelCommand())  //Command for driver, not for whole channel    
                {
                    toExecute=true;
                    break;
                }

            if (!cmd.isValue()) cmd.loadItemDef(this); // if no_suffix - both, command ON and value provided
            status2Send |= FLAG_COMMAND | FLAG_PARAMETERS | FLAG_SEND_IMMEDIATE; 
            toExecute=true; 
            break;
          case CMD_VOID:
          case CMD_TOGGLE:
          case CMD_DN:
          case CMD_UP:
            break;   

          case CMD_ENABLE:
            //clearFlag(FLAG_DISABLED); //saveItem have this
            status2Send |= FLAG_FLAGS;
            toExecute=true;
            //debugSerial<<F("Disable Flag is:")<<getFlag(FLAG_DISABLED)<<endl;
            //if (allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
          break;

          case CMD_DISABLE:
            //setFlag(FLAG_DISABLED); //saveItem have this
            status2Send |= FLAG_FLAGS;
            toExecute=true;
            //debugSerial<<F("Disable Flag is:")<<getFlag(FLAG_DISABLED)<<endl;
            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
          break;

          case CMD_UNFREEZE:
            //clearFlag(FLAG_DISABLED); //saveItem have this
            status2Send = FLAG_FLAGS;
            toExecute=true;
            //debugSerial<<F("Disable Flag is:")<<getFlag(FLAG_DISABLED)<<endl;
            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
          break;

          case CMD_FREEZE:
            //setFlag(FLAG_DISABLED); //saveItem have this
            status2Send = FLAG_FLAGS;
            command2Set = 0;
            toExecute=true;
            //debugSerial<<F("Disable Flag is:")<<getFlag(FLAG_DISABLED)<<endl;
            //if ((suffixCode==S_CMD) && allowRecursion && cmd.isValue()) //invoked not as group part, delayed, non Active or re-schedule 
            //                                                                         scheduleOppositeCommand(cmd,chActive,authorized);
          break;

          default:
            if (cmd.isCommand()) status2Send |= FLAG_COMMAND;
            if (cmd.isValue())   status2Send |= FLAG_PARAMETERS; 
            toExecute=true;
          } //Switch commands

          if (oppositeCommandToBeSchedulled) //invoked not as group part, delayed
                                                                                    { 
                                                                                     scheduleOppositeCommand(cmd,chActive,authorized);
                                                                                     status2Send &=~FLAG_PARAMETERS;
                                                                                    }
} // NO GROUP
if (invalidArgument) return -4;

if ((!driver || driver->isAllowed(cmd)) && (!getFlag(FLAG_FREEZED)))
{

    if (driver) //New style modular code
            {
            // UPDATE internal variables 
            if (status2Send) cmd.saveItem(this,status2Send);    

            res = driver->Ctrl(cmd, subItem, toExecute,authorized);   
            if (driver->getChanType() == CH_THERMO) status2Send |= FLAG_SEND_IMMEDIATE;
            //if (res==-1) status2Send=0;  ///////not working
            if (status2Send & FLAG_FLAGS) res =1; //ENABLE & DISABLE processed by core
            }     
    else 
    {
          switch (itemType) {
    /// rest of Legacy monolite core code (to be refactored ) BEGIN ///
                case CH_RELAY:
                if (cmd.isCommand())
                {

                    
                    short iaddr=getArg();
                    short icmd =cmd.getCmd();
                    if (!authorized && isProtectedPin(iaddr)) {errorSerial<<F("Unauthorized")<<endl; return -5;}

                        // UPDATE internal variables 
                        if (status2Send) cmd.saveItem(this,status2Send);

                    if (iaddr)
                    {
                    int k;
                    short inverse = 0;

                    if (iaddr < 0) {
                        iaddr = -iaddr;
                        inverse = 1;
                    }
                    if (iaddr <= (short) PINS_COUNT && iaddr>0)
                        {
                        pinMode(iaddr, OUTPUT);

                    switch (icmd){
                        case CMD_AUTO:
                        case CMD_COOL:
                        case CMD_ON:
                        case CMD_DRY:
                        case CMD_FAN:
                        case CMD_XON:
                        digitalWrite(iaddr, k = (inverse) ? LOW : HIGH);
                     break;
                        case CMD_OFF:
                        case CMD_HALT:
                        case CMD_XOFF:   
                        digitalWrite(iaddr, k = (inverse) ? HIGH : LOW); 
                    }
/*
                        if (inverse)
                            digitalWrite(iaddr, k = ((icmd == CMD_ON || icmd == CMD_AUTO) ? LOW : HIGH));
                        else
                            digitalWrite(iaddr, k = ((icmd == CMD_ON || icmd == CMD_AUTO) ? HIGH : LOW));

  */                  
                        debugSerial<<F("Pin:")<<iaddr<<F("=")<<k<<endl;
                        if (isProtectedPin(iaddr) && !isScheduled()) attachMaturaTimer();
                        status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                        res=1;
                        }
                    }
                    break;
                }

                case CH_THERMO:
                            // UPDATE internal variables 
                            if (status2Send) cmd.saveItem(this,status2Send);

                        switch (suffixCode)
                        {
                        case S_VAL:
                        //S_NOTFOUND:   
                        {
                        thermostatStore tStore;
                        tStore.asint=getExt();
                        if (!tStore.timestamp16) mqttClient.publish("/alarmoff/snsr", itemArr->name);
                        tStore.tempX100=cmd.getFloat()*100.;         //Save measurement
                        tStore.timestamp16=millisNZ(8) & 0xFFFF;    //And timestamp
                        debugSerial<<F(" T:")<<tStore.tempX100<<F(" TS:")<<tStore.timestamp16<<endl;
                        setExt(tStore.asint);
                        res=1;
                        }
                        break;
                        case S_SET:
                        status2Send |= FLAG_PARAMETERS | FLAG_SEND_IMMEDIATE; 
                        res=1;
                        //st.saveItem(this);
                        break;
                        case S_CMD:
                        status2Send |= FLAG_COMMAND | FLAG_SEND_IMMEDIATE; 
                        res=1;
                        }
                    break;


        #ifndef MODBUS_DISABLE
                case CH_MODBUS:
                            // UPDATE internal variables 
                            if (status2Send) cmd.saveItem(this,status2Send);

                    if (toExecute && !(chActive && cmd.getCmd()==CMD_ON && !cmd.isValue()))
                    {
                    int vol;    
                    if (!chActive && cmd.getCmd()== CMD_ON && (vol=cmd.getPercents())<MIN_VOLUME && vol>=0) 
                            {
                            cmd.setPercents(INIT_VOLUME);
                            status2Send |= FLAG_PARAMETERS | FLAG_SEND_IMMEDIATE; 
                            };

                    res=modbusDimmerSet(cmd);
                    }
                    break;
                case CH_VC:
                            // UPDATE internal variables 
                            if (status2Send) cmd.saveItem(this,status2Send);
                    if (toExecute && !(chActive && cmd.getCmd()==CMD_ON && !cmd.isValue())) res=VacomSetFan(cmd);
                    break;
                case CH_VCTEMP:
                            // UPDATE internal variables 
                            if (status2Send) cmd.saveItem(this,status2Send);
                    if (toExecute && !(chActive && cmd.getCmd()==CMD_ON && !cmd.isValue())) res=VacomSetHeat(cmd);
                    break;
        #endif
    /// rest of Legacy monolite core code (to be refactored ) END ///

    } //switch

    } //else (nodriver)

    //update command for HALT & XON and send MQTT status
    if (command2Set) setCmd(command2Set | FLAG_COMMAND);
    if (operation) SendStatus(status2Send);
} //alowed cmd
 else 
        {
            errorSerial<<F("CTRL: Command blocked by driver or channel frozen")<<endl;
             if ((status2Send & FLAG_FLAGS) && operation) 
                                {
                                cmd.saveItem(this,FLAG_FLAGS);
                                SendStatus(FLAG_FLAGS);
                                }
        }                   
debugSerial<<F("CTRL: Res:")<<res<<F(" time:")<<millis()-time<<endl; 
return res; 
  }
}

void printActiveStatus(bool active)
{
if (active) debugSerial<<F("(+) ");
    else debugSerial<<F("(-) ");
}

int Item::isActive() {
    itemCmd st;
    int val = 0;

    debugSerial<<itemArr->name<<F(" ");
    if (!isValid())
                      {
                      debugSerial<<F(" invalid")<<endl;
                      return -1;
                      }
    int cmd = getCmd();

    if (driver) {
                short active =  driver->isActive();
                if (active >= 0)
                   { 
                    printActiveStatus(active);
                    return active;
                   }
                }
 // No driver - check command     

    if (itemType != CH_GROUP)
// Simple check last command first
        switch (cmd) {
            case CMD_ON:
            case CMD_XON:
            case CMD_AUTO:
            case CMD_HEAT:
            case CMD_COOL:
                 printActiveStatus(true);
                return 1;
            case CMD_OFF:
            case CMD_HALT:
            case -1: ///// No last command
                  printActiveStatus(false);
                return 0;
        }

// Last time was not a command but parameters set. Looking inside
          
    st.loadItem(this);

    switch (itemType) {
        case CH_GROUP: //make recursive calculation - is it some active in group
            if (itemArg->type == aJson_Array) {
                debugSerial<<F("[");
                val=digGroup(itemArg);
                debugSerial<<F("]=");
                printActiveStatus(val);
                debugSerial<<endl;
                return val;
            } //if
            break;       
        case CH_MODBUS: //Legacy channels 
        case CH_VC:
            val = st.getPercents255();  //Light volume
            break;

        case CH_VCTEMP:
        case CH_PWM:
            val = st.getInt();
        break;
        default:
        debugSerial<<F(" unknown\n");
        return 0;
    } //switch

    debugSerial<<F(" value is ");
    debugSerial<<val<<endl;
    if (val) return 1; else return 0;
}


int Item::Poll(int cause) {

//if (isPendedModbusWrites) resumeModbus();                         
checkRetry(); 

aJsonObject *timestampObj = aJson.getArrayItem(itemArr, I_TIMESTAMP);
if (timestampObj)
{
  uint8_t  cmd = timestampObj->subtype & 0xF;
  bool authorized = (timestampObj->type==aJson_Reserved); 

  int32_t remain = (uint32_t) timestampObj->valueint - (uint32_t)millis(); 

  if (cmd)    {
                 itemCmd st(ST_UINT32,cmd);
                 st.Int(remain);

                //if (!(remain % 1000))
                if (cause == POLLING_1S)
                {     
                    SendStatusImmediate(st,FLAG_SEND_DELAYED);
                    debugSerial<< remain/1000 << F(" sec remaining") << endl;
                }

                if (remain <0 && abs(remain)< 0xFFFFFFFFUL/2)
                {
                int fr = freeRam();    
                SendStatusImmediate(st,FLAG_SEND_DELAYED);   
                       
                if (fr < minimalMemory) 
                    {
                    errorSerial<<F("CTRL/poll: OutOfMemory: ")<<fr<<endl;
                    return -1;
                    } 
                timestampObj->subtype=0;
                Ctrl(itemCmd(ST_VOID,cmd),NULL,true,authorized);
                //timestampObj->subtype=0; ////
                }
              }  
}

switch (cause)
{
  case POLLING_SLOW:
    // Legacy polling
    switch (itemType) {
      #ifndef MODBUS_DISABLE
        case CH_MODBUS:
            checkModbusDimmer();
            sendDelayedStatus();
            return true;
            break;
        case CH_VC:
            checkFM();
            sendDelayedStatus();
            return true;
            break;
      #endif
             default:
            sendDelayedStatus();
    }
  }

  if (driver && driver->Status())
                      {
                      return driver->Poll(cause);
                      }                             

    return false;
}

void Item::sendDelayedStatus()
{ long int flags = getFlag(FLAG_COMMAND | FLAG_PARAMETERS);

      if (flags && lanStatus==OPERATION)
      {
      SendStatus(flags);//(FLAG_COMMAND | FLAG_PARAMETERS);
      clearFlag(FLAG_COMMAND | FLAG_PARAMETERS);
      }
}


int Item::SendStatus(int sendFlags) {
    if (sendFlags & FLAG_SEND_IMMEDIATE) sendFlags &= ~ (FLAG_SEND_IMMEDIATE | FLAG_SEND_DEFFERED);
    if ((sendFlags & FLAG_SEND_DEFFERED) ||  freeRam()<150 || (!isNotRetainingStatus() )) {
        setFlag(sendFlags & (FLAG_COMMAND | FLAG_PARAMETERS | FLAG_FLAGS));
        debugSerial<<F("Status deffered\n");
        return -1;
    }
    else 
    {
        itemCmd st(ST_VOID,CMD_VOID);  
        st.loadItem(this, FLAG_COMMAND | FLAG_PARAMETERS);
        sendFlags |= getFlag(FLAG_COMMAND | FLAG_PARAMETERS | FLAG_FLAGS); //if some delayed status is pending
        //debugSerial<<F("ssi:")<<sendFlags<<endl;
        return SendStatusImmediate(st,sendFlags);
    }
  }
    
    int Item::SendStatusImmediate(itemCmd st, int sendFlags, char * subItem) {
    {
      char addrstr[64];
      char valstr[20] = "";
      char cmdstr[9] = "";
    //debugSerial<<"SSI "<<subItem<<endl;  
    st.debugOut();
    if (sendFlags & FLAG_COMMAND)
    {
    // Preparing legacy Command payload  //////////////
    switch (st.getCmd()) {
        case CMD_ON:
        case CMD_XON:
        case CMD_AUTO:
        case CMD_HEAT:
        case CMD_COOL:
        case CMD_DRY:
        case CMD_FAN:
            strcpy_P(cmdstr, ON_P);
            break;  
        //case CMD_ENABLE:
        //case CMD_DISABLE:
        //    break; 
        case CMD_OFF:
        case CMD_HALT:
            strcpy_P(cmdstr, OFF_P);
            break;
        case CMD_VOID:
        case CMD_RGB:
        sendFlags &= ~FLAG_COMMAND; // Not send command for parametrized req
            break;

        default:
            debugSerial<<F("Unknown cmd \n");
            sendFlags &= ~FLAG_COMMAND;
    }
   }

      // publish to MQTT - OpenHab Legacy style to 
      // myhome/s_out/item - mix: value and command
    

                      if (mqttClient.connected()  && !ethernetIdleCount)
                      {
                        if (!subItem )
                        {
                        setTopic(addrstr,sizeof(addrstr),T_OUT);
                        strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
                          

                        if (sendFlags & FLAG_PARAMETERS && st.getCmd() != CMD_OFF && st.getCmd() != CMD_HALT &&
                          // send only for OH bus supported types 
                          (st.getArgType() == ST_PERCENTS255 || st.getArgType() == ST_HSV255 || st.getArgType() == ST_FLOAT_CELSIUS))
                        {
                            st.toString(valstr, sizeof(valstr), FLAG_PARAMETERS,true); 
                            mqttClient.publish(addrstr, valstr, true);
                            debugSerial<<F("Pub: ")<<addrstr<<F("->")<<valstr<<endl;
                        }
                      else if ((sendFlags & FLAG_COMMAND) && (strlen(cmdstr)))
                        {
                            mqttClient.publish(addrstr, cmdstr, true);
                            debugSerial<<F("Pub: ")<<addrstr<<F("->")<<cmdstr<<endl;

                        }
                        } //!subItem
                      }
              else
                      {
                      setFlag(sendFlags);
                      return 0;
                      }
  
        
            // publush to MQTT - New style to
            // myhome/s_out/item/cmd
            // myhome/s_out/item/set

          if ((st.isValue() && (sendFlags & FLAG_PARAMETERS) || (sendFlags & FLAG_SEND_DELAYED)))
             {
              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
              if (subItem) 
                            {
                            strncat(addrstr, "/", sizeof(addrstr)-1);    
                            strncat(addrstr, subItem, sizeof(addrstr)-1);
                            }

              strncat(addrstr, "/", sizeof(addrstr)-1);
              
              if (sendFlags & FLAG_SEND_DELAYED)
                     strncat_P(addrstr, DEL_P, sizeof(addrstr)-1);
              else   strncat_P(addrstr, SET_P, sizeof(addrstr)-1); 

        // Preparing parameters payload //////////
           switch (st.getArgType()) {
                  case ST_RGB:
                  case ST_RGBW:
                  //valstr[0]='#';
                  st.Cmd(CMD_RGB);
                  st.toString(valstr, sizeof(valstr), FLAG_PARAMETERS|FLAG_COMMAND);
                  break;
            default:         
           st.toString(valstr, sizeof(valstr), (sendFlags & FLAG_SEND_DELAYED)?FLAG_COMMAND|FLAG_PARAMETERS:FLAG_PARAMETERS,(SCALE_VOLUME_100));
           }
              

              

              debugSerial<<F("Pub: ")<<addrstr<<F("->")<<valstr<<endl;
              if (mqttClient.connected()  && !ethernetIdleCount)
                 {
                  mqttClient.publish(addrstr, valstr,true);
                  clearFlag(FLAG_PARAMETERS);
                 }
              else
               {
               setFlag(sendFlags);
               return 0;
               }
              }


              if (sendFlags & FLAG_COMMAND)
              {
              // Some additional preparing for extended set of commands:
                switch (st.getCmd()) {
                    case CMD_AUTO:
                          strcpy_P(cmdstr, AUTO_P);
                        break;
                    case CMD_HEAT:
                          strcpy_P(cmdstr, HEAT_P);
                        break;
                    case CMD_COOL:
                          strcpy_P(cmdstr, COOL_P);
                        break;  
                    case CMD_DRY:
                          strcpy_P(cmdstr, DRY_P);
                        break;  
                    case CMD_FAN:
                          strcpy_P(cmdstr, FAN_ONLY_P);          
                        break;
                    //case CMD_ENABLE:
                    //      strcpy_P(cmdstr, ENABLE_P);          
                    //    break;  
                    //case CMD_DISABLE:
                    //      strcpy_P(cmdstr, DISABLE_P);          
                    //    break;                                                
                    case CMD_ON:
                    case CMD_XON:
                          if (itemType == CH_THERMO) strcpy_P(cmdstr, AUTO_P);
                   }

              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
              if (subItem) 
                            {
                            strncat(addrstr, "/", sizeof(addrstr)-1);    
                            strncat(addrstr, subItem, sizeof(addrstr)-1);
                            }
              strncat(addrstr, "/", sizeof(addrstr)-1);
              strncat_P(addrstr, CMD_P, sizeof(addrstr)-1);

              debugSerial<<F("Pub: ")<<addrstr<<F("->")<<cmdstr<<endl;
              if (mqttClient.connected()  && !ethernetIdleCount)
                 {
                  mqttClient.publish(addrstr, cmdstr,true);
                  clearFlag(FLAG_COMMAND);
                 }
              else
               {
                setFlag(sendFlags);
                return 0;
               }
              }
// Send ctrl
              if (sendFlags & FLAG_FLAGS)
            {
              if (getFlag(FLAG_DISABLED))
                   strcpy_P(cmdstr, DISABLE_P);

              else if (getFlag(FLAG_FREEZED))
                   strcpy_P(cmdstr, FREEZE_P);

              else strcpy_P(cmdstr, ENABLE_P);   


              //else strcpy_P(cmdstr, UNFREEZE_P); 

    
              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
              if (subItem) 
                            {
                            strncat(addrstr, "/", sizeof(addrstr)-1);    
                            strncat(addrstr, subItem, sizeof(addrstr)-1);
                            }
              strncat(addrstr, "/", sizeof(addrstr)-1);
              strncat_P(addrstr, CTRL_P, sizeof(addrstr)-1);

              debugSerial<<F("Pub: ")<<addrstr<<F("->")<<cmdstr<<endl;
              if (mqttClient.connected()  && !ethernetIdleCount)
                 {
                  mqttClient.publish(addrstr, cmdstr,true);
                  clearFlag(FLAG_FLAGS);
                 }
              else
               {
                setFlag(sendFlags);
                return 0;
               }
              }
        return 1;
    }
}

int Item::getChanType()
{
if (driver) return driver->getChanType();
return itemType;
}


// Setup FLAG_SEND_RETRY flag to repeat unsucsessfull modbus tranzaction after release line
void Item::mb_fail(int result) {
    debugSerial<<F("Modbus op failed:")<<_HEX(result)<<endl;
    setFlag(FLAG_SEND_RETRY);
 //   isPendedModbusWrites=true;
}

#define M_SUCCESS 1
#define M_FAILED  0
#define M_BUSY    -1
#define M_CLEAN   2

//Check pended transactoin
//Output:
//M_SUCCESS (1)  - Succeded 
//M_FAILED  (0)  - Failed
//M_BUSY    (-1) - Modbus busy
//M_CLEAN   (2)  - Clean. Not needed to repeat 

int Item::checkRetry() {
  int result = -1;  

    if (getFlag(FLAG_SEND_RETRY)) 
    {   // if last sending attempt of command was failed
      itemCmd val(ST_VOID,CMD_VOID);
      val.loadItem(this, FLAG_COMMAND | FLAG_PARAMETERS);
      
      if (driver) 
                    {
                    clearFlag(FLAG_SEND_RETRY);     // Clean retry flag    
                    debugSerial<<F("Retrying CMD\n");
                    result=driver->Ctrl(val);
                    }
      #ifndef MODBUS_DISABLE
      else switch (itemType) 
      {

      case CH_MODBUS:
        if (modbusBusy) return M_BUSY;
           clearFlag(FLAG_SEND_RETRY);     // Clean retry flag
           debugSerial<<F("Retrying modbus CMD\n");
           result=modbusDimmerSet(val);
          break;
      case CH_VC:
           if (modbusBusy) return M_BUSY;
           clearFlag(FLAG_SEND_RETRY);     // Clean retry flag
           debugSerial<<F("Retrying VC CMD\n");
           result=VacomSetFan(val);
           break;     
      case CH_VCTEMP:
           if (modbusBusy) return M_BUSY;
           clearFlag(FLAG_SEND_RETRY);     // Clean retry flag
           debugSerial<<F("Retrying VCTEMP CMD\n");
           result=VacomSetHeat(val);
           break;           
      default:
          break;
      }
      #endif

      switch (result){
          case -1: return M_CLEAN;
          case 0:  
                    debugSerial<<itemArr->name<<F(":")<<F(" failed")<<endl;
                    return M_FAILED; 
      }
      debugSerial<<itemArr->name<<F(":")<<F(" ok")<<endl;
      return M_SUCCESS;
    }
return M_CLEAN;
}

/*
bool Item::resumeModbus()
{

if (modbusBusy) return false;
bool success = true;

//debugSerial<<F("Pushing MB: ");
configLocked++;
if (items) {
    aJsonObject * item = items->child;
    while (items && item)
        if (item->type == aJson_Array && aJson.getArraySize(item)>1)  {
            Item it(item);
            if (it.isValid()) {

                        short res = it.checkModbusRetry(); 
                        if (res<=0) success = false;
               
            } //isValid
            yield();
            item = item->next;
        }  //if
debugSerial<<endl;
}
configLocked--;
 if (success) isPendedModbusWrites=false;
return true;
}

*/

//////////////////// Begin of legacy MODBUS code - to be moved in separate module /////////////////////


/*
addr 10d
Снять аварию 42001 (2001=7d1) =>4

[22:20:33] Write task has completed successfully
[22:20:33] <= Response: 0A 06 07 D0 00 04 89 FF
[22:20:32] => Poll: 0A 06 07 D0 00 04 89 FF

100%
2003-> 10000
[22:24:05] Write task has completed successfully
[22:24:05] <= Response: 0A 06 07 D2 27 10 33 C0
[22:24:05] => Poll: 0A 06 07 D2 27 10 33 C0

ON
2001->1
[22:24:50] Write task has completed successfully
[22:24:50] <= Response: 0A 06 07 D0 00 01 49 FC
[22:24:50] => Poll: 0A 06 07 D0 00 01 49 FC

OFF
2001->0
[22:25:35] Write task has completed successfully
[22:25:35] <= Response: 0A 06 07 D0 00 00 88 3C
[22:25:34] => Poll: 0A 06 07 D0 00 00 88 3C


POLL  2101x10
[22:27:29] <= Response: 0A 03 14 00 23 00 00 27 10 13 88 0B 9C 00 32 00 F8 00 F2 06 FA 01 3F AD D0
[22:27:29] => poll: 0A 03 08 34 00 0A 87 18

*/

#ifndef MODBUS_DISABLE
extern ModbusMaster node;

int Item::modbusDimmerSet(itemCmd st)
        {

          int value=st.getPercents();
          int cmd=st.getCmd();

          switch (cmd){
            case CMD_OFF:
            case CMD_HALT:
            value=0;
            break;

          }

               short numpar=0;
            if ((itemArg->type == aJson_Array) && ((numpar = aJson.getArraySize(itemArg)) >= 2)) {
                int _addr = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_ADDR)->valueint;
                int _reg = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_REG)->valueint;
                int _mask = -1;
                if (numpar >= (MODBUS_CMD_ARG_MASK+1))  _mask = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_MASK)->valueint;
                int _maxval = 0x3f;
                if (numpar >= (MODBUS_CMD_ARG_MAX_SCALE+1)) _maxval = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_MAX_SCALE)->valueint;
                int _regType = MODBUS_HOLDING_REG_TYPE;
                if (numpar >= (MODBUS_CMD_ARG_REG_TYPE+1)) _regType = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_REG_TYPE)->valueint;
                if (_maxval) return modbusDimmerSet(_addr, _reg, _regType, _mask, map(value, 0, 100, 0, _maxval));
                             else return modbusDimmerSet(_addr, _reg, _regType, _mask, value);
            }
            return 0;
        }


int Item::VacomSetFan(itemCmd st) {

  int val=st.getPercents();
  int cmd=st.getCmd();

  if (st.isCommand()) 
     switch (st.getSuffix()){
        case S_CMD:
        case S_NOTFOUND:
        break;
    default:
    return -1;    
    }

  switch (cmd){
    case CMD_OFF:
    case CMD_HALT:
    // produvka here
    val=0;
    break;

  }
    uint8_t result;
    int addr = getArg();
    debugSerial<<F("VC#")<<addr<<F("=")<<val<<endl;
    if (modbusBusy) {
       // setCmd(cmd);
       // setVal(val);
       st.saveItem(this,FLAG_PARAMETERS|FLAG_COMMAND);
        mb_fail();
        return 0;
    }
    modbusBusy = 1;
    //uint8_t j;//, result;
    //uint16_t data[1];

    modbusSerial.begin(MODBUS_FM_BAUD, MODBUS_FM_PARAM);
    node.begin(addr, modbusSerial);

    if (val) {
        result=node.writeSingleRegister(2001 - 1, 4 + 1);//delay(500);
        //node.writeSingleRegister(2001-1,1);
    } else result=node.writeSingleRegister(2001 - 1, 0);
    delay(100);
    if (result == node.ku8MBSuccess) debugSerial << F("MB ok")<<endl;
    result = node.writeSingleRegister(2003 - 1, val * 100);
    modbusBusy = 0;
    //resumeModbus();

    if (result == node.ku8MBSuccess) return 1;
    mb_fail(result);
    return 0;
}

#define a 0.1842f
#define b -36.68f

///move to float todo
int Item::VacomSetHeat(itemCmd st)
{
float val=st.getFloat();
int   cmd=st.getCmd();

uint8_t result;
int addr;
    if (itemArg->type != aJson_String) return 0;

    Item it(itemArg->valuestring);
    if (it.isValid() && it.itemType == CH_VC) addr=it.getArg();
    else return 0;

    debugSerial<<F("VC_heat#")<<addr<<F("=")<<val<<F(" cmd=")<<cmd<<endl;
    if (modbusBusy) {
      //setCmd(cmd);
      //setVal(val);
       st.saveItem(this,FLAG_COMMAND|FLAG_PARAMETERS);
      mb_fail();
      return 0;
    }
    modbusBusy = 1;

    modbusSerial.begin(MODBUS_FM_BAUD, MODBUS_FM_PARAM);
    node.begin(addr, modbusSerial);

    uint16_t regval;

    switch (cmd) {
        case CMD_OFF:
        case CMD_HALT:
            regval = 0;
            break;

        default:
            regval = round(( val - b) * 10 / a);
    }

    //debugSerial<<regval);
    result=node.writeSingleRegister(2004 - 1, regval);
    modbusBusy = 0;
    //resumeModbus();
    if (result == node.ku8MBSuccess) return 1;
    mb_fail(result);
    return 0;

}

int Item::modbusDimmerSet(int addr, uint16_t _reg, int _regType, int _mask, uint16_t value) {
    uint8_t result = 0;
    if (_regType != MODBUS_COIL_REG_TYPE || _regType != MODBUS_HOLDING_REG_TYPE) {

    }

    if (modbusBusy) {
      mb_fail();
        return 0;
    };
    modbusBusy = 1;

    modbusSerial.begin(MODBUS_SERIAL_BAUD, MODBUS_SERIAL_PARAM);
    node.begin(addr, modbusSerial);
    uint8_t t;
    switch (_mask) {
       case 1:
        value <<= 8;
        value |= (0xff);
        break;
       case 0:
        value &= 0xff;
        value |= (0xff00);
        break;
       case 2:
        break;
       case 3: // Modbus relay
        //value++;
          if (value) value = 1;
             else    value = 2;   
       case 4: //Swap high and low bytes
        t = (value & 0xff00) >> 8;
        value <<=8;
        value |= t;


    }
    debugSerial<<F("Addr:")<<addr<<F(" Reg:0x")<<_HEX(_reg)<<F(" T:")<<_regType<<F(" Val:0x")<<_HEX(value)<<endl;
    switch (_regType) {
        case MODBUS_HOLDING_REG_TYPE:
            result = node.writeSingleRegister(_reg, value);
            break;
        case MODBUS_COIL_REG_TYPE:
            result = node.writeSingleCoil(_reg, value);
            break;
        default:
            debugSerial<<F("Not supported reg type\n");
    }
    modbusBusy = 0;
    //resumeModbus();

    if (result == node.ku8MBSuccess) return 1;
    mb_fail(result);
    return 0;

}

int Item::checkFM() {
    if (modbusBusy) return -1;
    //if (checkVCRetry()) return -2;
    //if (checkModbusRetry()) return -2;
    modbusBusy = 1;

    uint8_t j, result;
    int16_t data;


    aJsonObject *out = aJson.createObject();
    char *outch;
    char addrstr[32];

    //strcpy_P(addrstr, outprefix);
    setTopic(addrstr,sizeof(addrstr),T_OUT);

    strncat(addrstr, itemArr->name, sizeof(addrstr) - 1);
    strncat(addrstr, "_stat", sizeof(addrstr) - 1);

    // aJson.addStringToObject(out,"type",   "rect");


    modbusSerial.begin(MODBUS_FM_BAUD, MODBUS_FM_PARAM);
    node.begin(getArg(), modbusSerial);


    result = node.readHoldingRegisters(2101 - 1, 10);

    // do something with data if read is successful
    if (result == node.ku8MBSuccess) {
        debugSerial<<F(" FM Val :");
        for (j = 0; j < 10; j++) {
            data = node.getResponseBuffer(j);
            debugSerial<<_HEX(data)<<F("-");
        }
        debugSerial<<endl;
        long int RPM;
        //     aJson.addNumberToObject(out,"gsw",    (int) node.getResponseBuffer(1));
        aJson.addNumberToObject(out, "V", (long int) node.getResponseBuffer(2) / 100.);
        //     aJson.addNumberToObject(out,"f",      (int) node.getResponseBuffer(3)/100.);
        aJson.addNumberToObject(out, "RPM", RPM=(long int) node.getResponseBuffer(4));
        aJson.addNumberToObject(out, "I", (long int) node.getResponseBuffer(5) / 100.);
        aJson.addNumberToObject(out, "M", (long int) node.getResponseBuffer(6) / 10.);
        //     aJson.addNumberToObject(out,"P",      (int) node.getResponseBuffer(7)/10.);
        //     aJson.addNumberToObject(out,"U",      (int) node.getResponseBuffer(8)/10.);
        //     aJson.addNumberToObject(out,"Ui",     (int) node.getResponseBuffer(9));
        aJson.addNumberToObject(out, "sw", (long int) node.getResponseBuffer(0));
        if (RPM && itemArg->type == aJson_Array) {
            aJsonObject *airGateObj = aJson.getArrayItem(itemArg, 1);
            if (airGateObj && airGateObj->type == aJson_String) {
                int val = 100;
                Item item(airGateObj->valuestring);
                if (item.isValid())
                //    item.Ctrl(0, 1, &val);
                item.Ctrl(itemCmd().Percents(val));
            }
        }
    } else
        debugSerial << F("Modbus polling error=") << _HEX(result) << endl;

    if (node.getResponseBuffer(0) & 8) //Active fault
    {
        result = node.readHoldingRegisters(2111 - 1, 1);
        if (result == node.ku8MBSuccess) aJson.addNumberToObject(out, "flt", (long int) node.getResponseBuffer(0));
        modbusBusy=0;
        //resumeModbus();

        if (isActive()>0) Off(); //Shut down ///
        modbusBusy=1;
    } else aJson.addNumberToObject(out, "flt", (long int)0);

    delay(50);
    result = node.readHoldingRegisters(20 - 1, 4);

    // do something with data if read is successful
    if (result == node.ku8MBSuccess) {
        debugSerial << F(" PI Val :");
        for (j = 0; j < 4; j++) {
            data = node.getResponseBuffer(j);
            debugSerial << data << F("-");
        }
        debugSerial << endl;
        int set = node.getResponseBuffer(0);
        float ftemp, fset = set * a + b;
        if (set)
            aJson.addNumberToObject(out, "set", fset);
        aJson.addNumberToObject(out, "t", ftemp = (int) node.getResponseBuffer(1) * a + b);
        //   aJson.addNumberToObject(out,"d",    (int) node.getResponseBuffer(2)*a+b);
        int16_t pwr =  node.getResponseBuffer(3);
        if (pwr > 0)
            aJson.addNumberToObject(out, "pwr", pwr / 10.);
        else aJson.addNumberToObject(out, "pwr", (long int) 0);

        if (ftemp > FM_OVERHEAT_CELSIUS && set) {
          if (mqttClient.connected()  && !ethernetIdleCount)
              mqttClient.publish("/alarm/ovrht", itemArr->name);
            Off(); //Shut down
        }
    } else
        debugSerial << F("Modbus polling error=") << _HEX(result);
    outch = aJson.print(out);
    if (mqttClient.connected()  && !ethernetIdleCount)
        mqttClient.publish(addrstr, outch);
    free(outch);
    aJson.deleteItem(out);
    modbusBusy = 0;
    //resumeModbus();
    return 1;
}

int Item::checkModbusDimmer() {
    if (modbusBusy) return -1;
    //if (checkModbusRetry()) return -2;

    short numpar = 0;
    if ((itemArg->type != aJson_Array) || ((numpar = aJson.getArraySize(itemArg)) < 2)) {
        debugSerial<<F("Illegal arguments\n");
        return -4;
    }

    modbusBusy = 1;

    uint8_t result;

    uint16_t addr = getArg(MODBUS_CMD_ARG_ADDR);
    uint16_t reg = getArg(MODBUS_CMD_ARG_REG);
    int _regType = MODBUS_HOLDING_REG_TYPE;
    if (numpar >= (MODBUS_CMD_ARG_REG_TYPE+1)) _regType = aJson.getArrayItem(itemArg, MODBUS_CMD_ARG_REG_TYPE)->valueint;
  //  short mask = getArg(2);
    // debugSerial<<F("Modbus polling "));
    // debugSerial<<addr);
    // debugSerial<<F("=>"));
    // debugSerial<<reg, HEX);
    // debugSerial<<F("(T:"));
    // debugSerial<<_regType);
    // debugSerial<<F(")"));

    int data;

    //node.setSlave(addr);

    modbusSerial.begin(MODBUS_SERIAL_BAUD, MODBUS_SERIAL_PARAM);
    node.begin(addr, modbusSerial);

    switch (_regType) {
        case MODBUS_HOLDING_REG_TYPE:
            result = node.readHoldingRegisters(reg, 1);
            break;
        case MODBUS_COIL_REG_TYPE:
            result = node.readCoils(reg, 1);
            break;
        case MODBUS_DISCRETE_REG_TYPE:
            result = node.readDiscreteInputs(reg, 1);
            break;
        case MODBUS_INPUT_REG_TYPE:
            result = node.readInputRegisters(reg, 1);
            break;
        default:
            debugSerial<<F("Not supported reg type\n");
    }

    if (result == node.ku8MBSuccess) {
        data = node.getResponseBuffer(0);
        debugSerial << F("MB: ") << itemArr->name << F(" Val: ") << _HEX(data) << endl;
        checkModbusDimmer(data);

        // Looking 1 step ahead for modbus item, which uses same register
            Item nextItem(pollingItem->next);
            if (pollingItem && nextItem.isValid() && nextItem.itemType == CH_MODBUS && nextItem.getArg(0) == addr &&
                nextItem.getArg(1) == reg) {
                nextItem.checkModbusDimmer(data);
                pollingItem = pollingItem->next;
                if (!pollingItem)
                    pollingItem = items->child;
            }
    } else
        debugSerial << F("Modbus polling error=") << _HEX(result) << endl;
    modbusBusy = 0;
    //resumeModbus();

return 1;
}


int Item::checkModbusDimmer(int data) {
    short mask = getArg(2);
    itemCmd st;
    
    if (mask < 0) return 0;
    st.loadItem(this);
    short maxVal = getArg(3);
    if (maxVal<=0) maxVal = 0x3f;

    int d = data;
    if (mask == 1) d >>= 8;
    if (mask == 0 || mask == 1) d &= 0xff;

    if (maxVal) d = map(d, 0, maxVal, 0, 255);

    int cmd = getCmd();
    //debugSerial<<d);
    if (st.getPercents255() != d || d && cmd == CMD_OFF || d && cmd == CMD_HALT) //volume changed or turned on manualy
    {
        if (d) { // Actually turned on
            if (cmd != CMD_XON && cmd != CMD_ON) setCmd(CMD_ON);  //store command
            st.Percents255(d);
            st.saveItem(this,FLAG_PARAMETERS);
            //setVal(d);       //store value
            if (cmd == CMD_OFF || cmd == CMD_HALT) SendStatus(FLAG_COMMAND); //update OH with ON if it was turned off before
            SendStatus(FLAG_PARAMETERS); //update OH with value
        } else {
            if (cmd != CMD_HALT && cmd != CMD_OFF) {
                setCmd(CMD_OFF); // store command (not value)
                SendStatus(FLAG_COMMAND);// update OH
            }
        }
    } //if data changed
  return 1;  
}

#endif
