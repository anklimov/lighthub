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

short modbusBusy = 0;
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
    else if (strcmp_P(payload, CMD_P) == 0) cmd = S_CMD;
    else if (strcmp_P(payload, MODE_P) == 0) cmd = S_MODE;
    else if (strcmp_P(payload, HSV_P) == 0) cmd = S_HSV;
    else if (strcmp_P(payload, RGB_P) == 0) cmd = S_RGB;
    else if (strcmp_P(payload, FAN_P) == 0) cmd = S_FAN;
    else if (strcmp_P(payload, HUE_P) == 0) cmd = S_HUE;
    else if (strcmp_P(payload, SAT_P) == 0) cmd = S_SAT;
    else if (strcmp_P(payload, TEMP_P) == 0) cmd = S_TEMP;
  /*  UnUsed now
    else if (strcmp_P(payload, SETPOINT_P) == 0) cmd = S_SETPOINT;
    else if (strcmp_P(payload, TEMP_P) == 0) cmd = S_TEMP;
    else if (strcmp_P(payload, POWER_P) == 0) cmd = S_POWER;
    else if (strcmp_P(payload, VOL_P) == 0) cmd = S_VOL;
  */
    return cmd;
}

const short defval[4] = {0, 0, 0, 0}; //Type,Arg,Val,Cmd

Item::Item(aJsonObject *obj)//Constructor
{
    itemArr = obj;
    driver = NULL;
    Parse();
}

void Item::Parse() {
    if (isValid()) {
        // Todo - avoid static enlarge for every types
        for (int i = aJson.getArraySize(itemArr); i < 4; i++)
            aJson.addItemToArray(itemArr, aJson.createItem(
                    int(defval[i]))); //Enlarge item to 4 elements. VAL=int if no other definition in conf
        itemType = aJson.getArrayItem(itemArr, I_TYPE)->valueint;
        itemArg = aJson.getArrayItem(itemArr, I_ARG);
        itemVal = aJson.getArrayItem(itemArr, I_VAL);
        itemExt = aJson.getArrayItem(itemArr, I_EXT);
        switch (itemType)
        {
          case CH_PWM:
          driver = new out_pwm (this);
          break;

#ifndef   DMX_DISABLE
            case CH_RGBW:
            case CH_RGB:
            case CH_DIMMER:
            driver = new out_dmx (this);
  //          debugSerial<<F("DMX driver created")<<endl;
            break;
#endif
#ifndef   SPILED_DISABLE
          case CH_SPILED:
          driver = new out_SPILed (this);
//          debugSerial<<F("SPILED driver created")<<endl;
          break;
#endif

#ifndef   AC_DISABLE
          case CH_AC:
          driver = new out_AC (this);
//          debugSerial<<F("AC driver created")<<endl;
          break;
#endif

#ifndef   MOTOR_DISABLE
          case CH_MOTOR:
          driver = new out_Motor (this);
//          debugSerial<<F("AC driver created")<<endl;
          break;
#endif

#ifndef   MBUS_DISABLE
          case CH_MBUS:
          driver = new out_Modbus (this);
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
        driver->Setup();
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
//              debugSerial<<F("Driver destroyed")<<endl;
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
        strncpy(defaultSubItem,sub,sizeof(defaultSubItem));
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
    if (itemCmd)
    {
        itemCmd->valueint = cmdValue & CMD_MASK | itemCmd->valueint & FLAG_MASK;   // Preserve special bits
        debugSerial<<F("SetCmd:")<<cmdValue<<endl;
      }
}

short Item::getFlag   (short flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd)
  {
      return itemCmd->valueint & flag & FLAG_MASK;
    }
return 0;
}

void Item::setFlag   (short flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd)
  {
      itemCmd->valueint |= flag & FLAG_MASK;   // Preserve CMD bits
      debugSerial<<F("SetFlag:")<<flag<<endl;
    }

}

void Item::clearFlag (short flag)
{
  aJsonObject *itemCmd = aJson.getArrayItem(itemArr, I_CMD);
  if (itemCmd)
  {
      itemCmd->valueint &= CMD_MASK | ~(flag & FLAG_MASK);    // Preserve CMD bits
      debugSerial<<F("ClrFlag:")<<flag<<endl;
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
  if (itemVal->type == aJson_Int) return itemVal->subtype;
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
    if (!itemVal || itemVal->type != aJson_Int) return;
    debugSerial<<F(" Store ")<<F(" Val=")<<par<<endl;
    itemVal->valueint = par;
}

void Item::setSubtype(uint8_t par)  // Only store if VAL is int (autogenerated or config-defined)
{
    if (!itemVal || itemVal->type != aJson_Int) return;
    //debugSerial<<F(" Store ")<<F(" Val=")<<par<<endl;
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
    for (int i = aJson.getArraySize(itemArr); i <= 4; i++)
        aJson.addItemToArray(itemArr, itemExt=aJson.createItem(0));
    //itemExt = aJson.getArrayItem(itemArr, I_EXT);
      };

    if(!itemExt || itemExt->type != aJson_Int) return;
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
        aJson.addItemToArray(itemArr, itemExt = aJson.createItem(0));
    //itemExt = aJson.getArrayItem(itemArr, I_EXT);
      };

    if(!itemExt || (itemExt->type != aJson_Int)) return NULL;
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

//#define MAXCTRLPAR 3

// myhome/dev/item/subItem
int Item::Ctrl(char * payload, char * subItem)
{
if (!payload) return 0;


int   suffixCode = 0;

if ((!subItem || !strlen(subItem)) && strlen(defaultSubItem))
    subItem = defaultSubItem;  /// possible problem here with truncated default

if (subItem && strlen(subItem))
    suffixCode = retrieveCode(&subItem);

if (!suffixCode && defaultSuffixCode)
        suffixCode = defaultSuffixCode;


int i=0;
while (payload[i]) {payload[i]=toupper(payload[i]);i++;};

int cmd = txt2cmd(payload);
debugSerial<<F("Txt2Cmd:")<<cmd<<endl;

itemCmd st(ST_VOID,cmd);
st.setSuffix(suffixCode);

  switch (cmd) {
      case CMD_UP:
      case CMD_DN:
          st.Int((int32_t)getInt((char **) &payload));
          return   Ctrl(st,subItem);
      break;
      case CMD_HSV: st.Cmd(0);
      case CMD_VOID:
       {
         //Parsing integers from payload
          short i = 0;
          int Par[3];
          while (payload && i < 3)
              Par[i++] = getInt((char **) &payload);

              switch(suffixCode)
              {case S_HUE:
                      st.setH(Par[0]);
                      break;
               case S_SAT:
                      st.setS(Par[0]);
                      break;
                default:
                    switch (i) //Number of params
                    {
                      case 1: st.Percents(Par[0]);
                      break;
                      case 2: st.setH(Par[0]);
                              st.setS(Par[0]);
                      break;
                      case 3: st.HSV(Par[0],Par[1],Par[2]);
                      default:;
                    }
              }
          //return   Ctrl(setCommand, i, Par,  suffixCode, subItem);
          return   Ctrl(st,subItem);
      }
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
         //return   Ctrl(setCommand, i, Par,  suffixCode, subItem);
         return   Ctrl(st,subItem);
      }
      default: //some known command
      return Ctrl(st, subItem);

  } //ctrl
return 0;
}

int Item::Ctrl(itemCmd cmd,  char* subItem)
{
  char stringBuffer[16];
  int suffixCode = cmd.getSuffix();
  bool operation = isNotRetainingStatus() ;

    if ((!subItem || !strlen(subItem)) && strlen(defaultSubItem))
        subItem = defaultSubItem;  /// possible problem here with truncated default

    if (!suffixCode && subItem && strlen(subItem))
        suffixCode = retrieveCode(&subItem);

    if (!suffixCode && defaultSuffixCode)
        suffixCode = defaultSuffixCode;


    debugSerial<<F("RAM=")<<freeRam()<<F(" Item=")<<itemArr->name<<F(" Sub=")<<subItem<<F(" Suff=")<<suffixCode<<F(" Cmd=")<<cmd.getCmd()<<F(" Par=")<<cmd.toString(stringBuffer, sizeof(stringBuffer))<<endl;
    if (!itemArr) return -1;

          if (itemType != CH_GROUP )
          {
          //Check if subitem is some sort of command
          int subitemCmd = subitem2cmd(subItem);
          if (subitemCmd && subitemCmd != getCmd())
              {
                debugSerial<<F("Ignored, channel cmd=")<<getCmd()<<endl;
                return -1;
              }
          }
          else
          // Group channel
          if (! operation) return -1;

          bool    chActive  = (isActive()>0);
          bool    toExecute = (chActive>0); // execute if channel is active now

          itemCmd st(ST_VOID,CMD_VOID);

           //Restore previous channel state to "st"
           //if no values - set default values, save and put to MQTT
           if (!st.loadItem(this))
                                    {
                                    debugSerial<<F("No stored values - default: ");
                                    st.setChanType(getChanType());
                                    st.setDefault();
                                    st.saveItem(this);
                                    st.debugOut();
                                    SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                                    }

          //threating Toggle, Restore, XOFF (special conditional commands)/ convert to ON, OFF and SET values
          switch (cmd.getCmd()) {
              int t;
              case CMD_TOGGLE:
                  toExecute=true;
                  if (chActive) st.Cmd(CMD_OFF);
                      else st.Cmd(CMD_ON);
                  cmd.Cmd(st.getCmd()); // For GROUP control
                  break;

              case CMD_RESTORE:
                  if (itemType != CH_GROUP) //individual threating of channels. Ignore restore command for groups
                      switch (t = getCmd()) {
                          case CMD_HALT: //previous command was HALT ?
                              debugSerial << F("Restored from:") << t << endl;
                              toExecute=true;
                              if (itemType == CH_THERMO) st.Cmd(CMD_AUTO);
                                  else st.Cmd(CMD_ON);    //turning on
                              break;
                          default:
                              return -3;
                      }

                  break;
              case CMD_XOFF:
                  if (itemType != CH_GROUP) //individual threating of channels. Ignore restore command for groups
                      switch (t = getCmd()) {
                          case CMD_XON: //previous command was CMD_XON ?
                              debugSerial << F("Turned off from:") << t << endl;
                              toExecute=true;
                              st.Cmd(CMD_OFF);    //turning Off
                              break;
                          default:
                              debugSerial << F("XOFF skipped. Prev cmd:") << t <<endl;
                            return -3;

                    }
                    break;
            case CMD_DN:
            case CMD_UP:
            {
            //if (itemType == CH_GROUP) break; ////bug here
            st.Cmd(CMD_VOID); // Converting to SET value command
            short step=0;
            if (cmd.isValue()) step=cmd.getInt();
            if (!step) step=DEFAULT_INC_STEP;
            if (cmd.getCmd() == CMD_DN) step=-step;

                      switch (suffixCode)
                      {
                        case S_NOTFOUND:
                             toExecute=true;
                        case S_SET:
                             if (st.incrementPercents(step))
                             {
                               st.saveItem(this);
                               SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                             }
                        break;
                        case S_HUE:
                             if (st.incrementH(step))
                             {
                             st.saveItem(this);
                             SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                             }
                             break;
                        case S_SAT:
                             if (st.incrementS(step))
                             {
                             st.saveItem(this);
                             SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                             }
                      } //switch suffix

             } //Case UP/DOWN
             break;
             case CMD_VOID: // No commands, just set value
                         ////if (itemType == CH_GROUP ) break; ////
                         if (!cmd.isValue()) break;
////                         if ( cType == CH_RGB || cType == CH_RGBW || cType == CH_GROUP )
                               switch (suffixCode)
                               {
                                     case S_NOTFOUND: //For empty (universal) suffix - turn ON/OFF automatically
                                     toExecute=true;
                                     if (chActive>0 && !cmd.getPercents()) st.Cmd(CMD_OFF);
                                     if (chActive==0 && cmd.getPercents()) st.Cmd(CMD_ON);
                                     setCmd(st.getCmd());
                                     SendStatus(SEND_COMMAND | SEND_DEFFERED);

                                     // continue processing as SET
                                     case S_SET:
                                     st.assignFrom(cmd);
                                     st.saveItem(this);
                                     SendStatus(SEND_PARAMETERS | SEND_DEFFERED);

                                     break;
                                     case S_SAT:
                                     if (st.setS(cmd.getS()))
                                        {
                                          st.saveItem(this);
                                          SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                                        }
                                     break;

                                     case S_HUE:
                                     if (st.setH(cmd.getH()))
                                        {
                                          st.saveItem(this);
                                          SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                                        }
                                   }

                                  break;

          default:
                st.Cmd(cmd.getCmd());
                toExecute=true;
          } //Switch commands

//==================

if (driver) //New style modular code
        {
          int res = -1;
          switch (cmd.getCmd())
          {
            case CMD_XON:
            if (!chActive>0)  //if channel was'nt active before CMD_XON
                  {
                    debugSerial<<F("Turning XON\n");
                    res = driver->Ctrl(st.Cmd(CMD_ON), subItem);
                    setCmd(CMD_XON);
                    SendStatus(SEND_COMMAND);
                  }
              else
              {  //cmd = CMD_ON;
                debugSerial<<F("Already Active\n");
                return -3;
              }
            break;
            case CMD_HALT:
            if (chActive>0)  //if channel was active before CMD_HALT
                  {
                    res = driver->Ctrl(st.Cmd(CMD_OFF), subItem);
                    setCmd(CMD_HALT);
                    SendStatus(SEND_COMMAND);
                    return res;
                  }
            else
                  {
                    debugSerial<<F("Already Inactive\n");
                    return -3;
                  }
            break;
            case CMD_OFF:
            if (getCmd() != CMD_HALT) //Halted, ignore OFF
                 {
                   res = driver->Ctrl(st.Cmd(CMD_OFF), subItem);
                   setCmd(CMD_OFF);
                   SendStatus(SEND_COMMAND);
                 }
            else
                  {
                    debugSerial<<F("Already Halted\n");
                    return -3;
                  }
            break;

            case CMD_VOID:
            res = driver->Ctrl(st, subItem, toExecute);
            break;

            default: //another command
            res = driver->Ctrl(st, subItem);
            if (st.isCommand())
                        {
                        setCmd(st.getCmd());
                        SendStatus(SEND_COMMAND);
                        }
          }
          return res;
        }
else //Driver not found
//==================
{
switch (itemType) {
                case CH_GROUP:
                {
                    if (itemArg->type == aJson_Array) {
                        aJsonObject *i = itemArg->child;
                        configLocked++;
                        while (i) {
                            if (i->type == aJson_String)
                              {
                              Item it(i->valuestring);
                              it.Ctrl(cmd,subItem);
                              }
                            i = i->next;
                        } //while
                        configLocked--;
                    } //if
                } //case
               break;



// rest of Legacy monolite core code (to be refactored )
              case CH_RELAY:
              {
                 short iaddr=getArg();
                 short icmd =st.getCmd();

                 if (iaddr)
                 {
                  int k;
                  short inverse = 0;

                  if (iaddr < 0) {
                      iaddr = -iaddr;
                      inverse = 1;
                  }
                  if (iaddr <=PINS_COUNT && iaddr>0)
                      {
                      pinMode(iaddr, OUTPUT);

                      if (inverse)
                          digitalWrite(iaddr, k = ((icmd == CMD_ON || icmd == CMD_XON) ? LOW : HIGH));
                      else
                          digitalWrite(iaddr, k = ((icmd == CMD_ON || icmd == CMD_XON) ? HIGH : LOW));
                      debugSerial<<F("Pin:")<<iaddr<<F("=")<<k<<endl;
                      }
                }
                   break;
              }

              case CH_THERMO:
                      ///thermoSet(name,cmd,Par1); all activities done - update temp & cmd
                   break;


      #ifndef MODBUS_DISABLE
              case CH_MODBUS:
                  modbusDimmerSet(st.getPercents());
                  break;
              case CH_VC:
                  VacomSetFan(st.getPercents(), st.getCmd());
                  break;
              case CH_VCTEMP:
                  VacomSetHeat(st.getPercents(), st.getCmd());
                  break;

     #endif
  } //switch
  if (st.isCommand())
              {
              setCmd(st.getCmd());
              SendStatus(SEND_COMMAND);
              }
 }
}






int Item::isActive() {
    itemArgStore st;
    int val = 0;


    debugSerial<<itemArr->name;
    if (!isValid())
                      {
                      debugSerial<<F(" invalid")<<endl;
                      return -1;
                      }
    int cmd = getCmd();


    if (itemType != CH_GROUP)
// Simple check last command first
        switch (cmd) {
            case CMD_ON:
            case CMD_XON:
            case CMD_AUTO:
            case CMD_HEAT:
            case CMD_COOL:
                debugSerial<<F(" active\n");
                return 1;
            case CMD_OFF:
            case CMD_HALT:
            case -1: ///// No last command
                debugSerial<<F(" inactive\n");
                return 0;
        }

// Last time was not a command but parameters set. Looking inside
    if (driver) return driver->isActive();
    st.aslong = getVal();

    switch (itemType) {
        case CH_GROUP: //make recursive calculation - is it some active in group
            if (itemArg->type == aJson_Array) {
                debugSerial<<F(" Grp:");
                aJsonObject *i = itemArg->child;
                while (i) {
                    if (i->type == aJson_String)
                    {
                        Item it(i->valuestring);

                        if (it.isValid() && it.isActive()>0) {
                            debugSerial<<F(" active\n");
                            return 1;
                        }
                    }
                    i = i->next;
                } //while
                debugSerial<<F(" inactive\n");
                return 0;
            } //if
            break;


        case CH_RGBW:
        case CH_RGB:


            val = st.v;  //Light volume
            break;

        case CH_DIMMER:         //Everywhere, in flat VAL
        case CH_MODBUS:
  //      case CH_THERMO:
        case CH_VC:
        case CH_VCTEMP:
        case CH_PWM:
            val = st.aslong;
        break;
        default:
        debugSerial<<F(" unknown\n");
        return 0;
    } //switch
    debugSerial<<F(" value is ");
    debugSerial<<val<<endl;
    if (val) return 1; else return 0;
}

/*

short thermoSet(char * name, short cmd, short t)
{

if (items)
  {
  aJsonObject *item= aJson.getObjectItem(items, name);
  if (item && (item->type==aJson_Array) && (aJson.getArrayItem(item, I_TYPE)->valueint==CH_THERMO))
      {

        for (int i=aJson.getArraySize(item);i<4;i++) aJson.addItemToArray(item,aJson.createItem(int(0))); //Enlarge item to 4 elements
             if (!cmd) aJson.getArrayItem(item, I_VAL)->valueint=t;
             aJson.getArrayItem(item, I_CMD)->valueint=cmd;

      }

  }
}


void PooledItem::Idle()
{
if (PoolingInterval)
  {
    Pool();
    next=millis()+PoolingInterval;
  }

};





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
int Item::modbusDimmerSet(uint16_t value)
        {
          switch (getCmd())
          {
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
        }
#endif

void Item::mb_fail() {
    debugSerial<<F("Modbus op failed\n");
    setFlag(SEND_RETRY);
}

#ifndef MODBUS_DISABLE
extern ModbusMaster node;



int Item::VacomSetFan(int8_t val, int8_t cmd) {
    uint8_t result;
    int addr = getArg();
    debugSerial<<F("VC#")<<addr<<F("=")<<val<<endl;
    if (modbusBusy) {
        setCmd(cmd);
        setVal(val);
        mb_fail();
        return 0;
    }
    modbusBusy = 1;
    uint8_t j;//, result;
    //uint16_t data[1];

    modbusSerial.begin(9600, fmPar);
    node.begin(addr, modbusSerial);

    if (val) {
        node.writeSingleRegister(2001 - 1, 4 + 1);//delay(500);
        //node.writeSingleRegister(2001-1,1);
    } else node.writeSingleRegister(2001 - 1, 0);
    delay(50);
    result = node.writeSingleRegister(2003 - 1, val * 100);
    modbusBusy = 0;

    if (result == node.ku8MBSuccess) return 1;
    mb_fail();
    return 0;
}

#define a 0.1842f
#define b -36.68f

int Item::VacomSetHeat(int8_t val, int8_t cmd) {
    uint8_t result;
    int addr;
    if (itemArg->type != aJson_String) return 0;

    Item it(itemArg->valuestring);
    if (it.isValid() && it.itemType == CH_VC) addr=it.getArg();
    else return 0;

    debugSerial<<F("VC_heat#")<<addr<<F("=")<<val<<F(" cmd=")<<cmd<<endl;
    if (modbusBusy) {
      setCmd(cmd);
      setVal(val);
      mb_fail();
      return 0;
    }
    modbusBusy = 1;

    modbusSerial.begin(9600, fmPar);
    node.begin(addr, modbusSerial);

    uint16_t regval;

    switch (cmd) {
        case CMD_OFF:
        case CMD_HALT:
            regval = 0;
            break;

        default:
            regval = round(((float) val - b) * 10 / a);
    }

    //debugSerial<<regval);
    result=node.writeSingleRegister(2004 - 1, regval);
    modbusBusy = 0;
    if (result == node.ku8MBSuccess) return 1;
    mb_fail();
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

    modbusSerial.begin(MODBUS_SERIAL_BAUD, dimPar);
    node.begin(addr, modbusSerial);
    switch (_mask) {
       case 1:
        value <<= 8;
        value |= (0xff);
        break;
       case 0:
        value &= 0xff;
        value |= (0xff00);
    }
    debugSerial<<addr<<F("=>")<<_HEX(_reg)<<F("(T:")<<_regType<<F("):")<<_HEX(value)<<endl;
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

    if (result == node.ku8MBSuccess) return 1;
    mb_fail();
    return 0;

}

int Item::checkFM() {
    if (modbusBusy) return -1;
    if (checkVCRetry()) return -2;
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


    modbusSerial.begin(9600, fmPar);
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
        int RPM;
        //     aJson.addNumberToObject(out,"gsw",    (int) node.getResponseBuffer(1));
        aJson.addNumberToObject(out, "V", (int) node.getResponseBuffer(2) / 100.);
        //     aJson.addNumberToObject(out,"f",      (int) node.getResponseBuffer(3)/100.);
        aJson.addNumberToObject(out, "RPM", RPM=(int) node.getResponseBuffer(4));
        aJson.addNumberToObject(out, "I", (int) node.getResponseBuffer(5) / 100.);
        aJson.addNumberToObject(out, "M", (int) node.getResponseBuffer(6) / 10.);
        //     aJson.addNumberToObject(out,"P",      (int) node.getResponseBuffer(7)/10.);
        //     aJson.addNumberToObject(out,"U",      (int) node.getResponseBuffer(8)/10.);
        //     aJson.addNumberToObject(out,"Ui",     (int) node.getResponseBuffer(9));
        aJson.addNumberToObject(out, "sw", (int) node.getResponseBuffer(0));
        if (RPM && itemArg->type == aJson_Array) {
            aJsonObject *airGateObj = aJson.getArrayItem(itemArg, 1);
            if (airGateObj && airGateObj->type == aJson_String) {
                int val = 100;
                Item item(airGateObj->valuestring);
                if (item.isValid())
                //    item.Ctrl(0, 1, &val);
                item.Ctrl(itemCmd(ST_PERCENTS,CMD_VOID).Percents(val));
            }
        }
    } else
        debugSerial << F("Modbus polling error=") << _HEX(result) << endl;

    if (node.getResponseBuffer(0) & 8) //Active fault
    {
        result = node.readHoldingRegisters(2111 - 1, 1);
        if (result == node.ku8MBSuccess) aJson.addNumberToObject(out, "flt", (int) node.getResponseBuffer(0));
        modbusBusy=0;
        if (isActive()>0) Off(); //Shut down ///
        modbusBusy=1;
    } else aJson.addNumberToObject(out, "flt", 0);

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
        else aJson.addNumberToObject(out, "pwr", 0);

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
}

boolean Item::checkModbusRetry() {
  if (modbusBusy) return false;
//    int cmd = getCmd();
    if (getFlag(SEND_RETRY)) {   // if last sending attempt of command was failed
      int val = getVal();
      debugSerial<<F("Retrying dimmer CMD\n");
      clearFlag(SEND_RETRY);     // Clean retry flag
      modbusDimmerSet(val);
      return true;
    }
return false;
}

boolean Item::checkVCRetry() {
    if (modbusBusy) return false;
    int cmd = getCmd();
    if (getFlag(SEND_RETRY)) {   // if last sending attempt of command was failed
      int val = getVal();
      debugSerial<<F("Retrying VC CMD\n");
      clearFlag(SEND_RETRY);     // Clean retry flag
      VacomSetFan(val,cmd);
      return true;
    }
return false;
}

boolean Item::checkHeatRetry() {
    if (modbusBusy) return false;
    int cmd = getCmd();
    if (getFlag(SEND_RETRY)) {   // if last sending attempt of command was failed
      int val = getVal();
      debugSerial<<F("Retrying VC temp CMD\n");
      clearFlag(SEND_RETRY);     // Clean retry flag
      VacomSetHeat(val,cmd);
      return true;
    }
return false;
}

int Item::checkModbusDimmer() {
    if (modbusBusy) return -1;
    if (checkModbusRetry()) return -2;

    short numpar = 0;
    if ((itemArg->type != aJson_Array) || ((numpar = aJson.getArraySize(itemArg)) < 2)) {
        debugSerial<<F("Illegal arguments\n");
        return -3;
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

    modbusSerial.begin(MODBUS_SERIAL_BAUD, dimPar);
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


}


int Item::checkModbusDimmer(int data) {
    short mask = getArg(2);
    if (mask < 0) return 0;

    short maxVal = getArg(3);
    if (maxVal<=0) maxVal = 0x3f;

    int d = data;
    if (mask == 1) d >>= 8;
    if (mask == 0 || mask == 1) d &= 0xff;

    if (maxVal) d = map(d, 0, maxVal, 0, 100);

    int cmd = getCmd();
    //debugSerial<<d);
    if (getVal() != d || d && cmd == CMD_OFF || d && cmd == CMD_HALT) //volume changed or turned on manualy
    {
        if (d) { // Actually turned on
            if (cmd != CMD_XON && cmd != CMD_ON) setCmd(CMD_ON);  //store command
            setVal(d);       //store value
            if (cmd == CMD_OFF || cmd == CMD_HALT) SendStatus(SEND_COMMAND); //update OH with ON if it was turned off before
            SendStatus(SEND_PARAMETERS); //update OH with value
        } else {
            if (cmd != CMD_HALT && cmd != CMD_OFF) {
                setCmd(CMD_OFF); // store command (not value)
                SendStatus(SEND_COMMAND);// update OH
            }
        }
    } //if data changed
}

#endif

int Item::Poll(int cause) {

switch (cause)
{
  case POLLING_SLOW:
    // Legacy polling
    switch (itemType) {
      #ifndef MODBUS_DISABLE
        case CH_MODBUS:
            checkModbusDimmer();
            sendDelayedStatus();
            return INTERVAL_CHECK_MODBUS;
            break;
        case CH_VC:
            checkFM();
            sendDelayedStatus();
            return INTERVAL_CHECK_MODBUS;
            break;
        case CH_VCTEMP:
            checkHeatRetry();
            sendDelayedStatus();
            return INTERVAL_CHECK_MODBUS;
            break;
        #endif
      /*  case CH_RGB:    //All channels with slider generate too many updates
        case CH_RGBW:
        case CH_DIMMER:
        case CH_PWM:
        case CH_VCTEMP:
        case CH_THERMO:
        case CH_GROUP:*/
             default:
            sendDelayedStatus();
    }
  }

  if (driver && driver->Status())
                      {
                      return driver->Poll(cause);
                      }

    return INTERVAL_POLLING;
}

void Item::sendDelayedStatus()
{ long int flags = getFlag(SEND_COMMAND | SEND_PARAMETERS);

      if (flags && lanStatus==OPERATION)
      {
      SendStatus(flags);//(SEND_COMMAND | SEND_PARAMETERS);
      clearFlag(SEND_COMMAND | SEND_PARAMETERS);
      }
}


int Item::SendStatus(int sendFlags) {

    if ((sendFlags & SEND_DEFFERED) || (!isNotRetainingStatus() )) {
        setFlag(sendFlags & (SEND_COMMAND | SEND_PARAMETERS));
        debugSerial<<F("Status deffered\n");
        return -1;
    }
    else {
      int chancmd=getCmd();
      sendFlags |= getFlag(SEND_COMMAND | SEND_PARAMETERS); //if some delayed status is pending

      char addrstr[48];
      char valstr[16] = "";
      char cmdstr[8] = "";

      if (sendFlags & SEND_PARAMETERS)
      {
      // Preparing parameters payload //////////
/*
       itemArgStore st;
       int chanType = itemType;
        if (driver) chanType = driver->getChanType();
       //retrive stored values
       st.aslong = getVal();
       switch (chanType) {
               //case CH_GROUP:
               case CH_RGBW:
               case CH_RGB:
               snprintf(valstr, sizeof(valstr), "%d,%d,%d", st.h,st.s,st.v);
            break;
               case CH_GROUP:
               //if (st.hsv_flag)
               if (getSubtype()==ST_HSV)
                snprintf(valstr, sizeof(valstr), "%d,%d,%d", st.h,st.s,st.v);
               else
                snprintf(valstr, sizeof(valstr), "%d", st.v);
            break;
              case CH_RELAY:
                sendFlags &= ~SEND_PARAMETERS;  //No need to send value for relay
            break;
               default:
               snprintf(valstr, sizeof(valstr), "%ld", st.aslong);
           }//itemtype
           */




    itemCmd st(ST_VOID,CMD_VOID);
    st.loadItem(this, true);
    st.toString(valstr, sizeof(valstr), SEND_PARAMETERS);
    }
    if (sendFlags & SEND_COMMAND)
    {
    // Preparing Command payload  //////////////
    switch (chancmd) {
        case CMD_ON:
        case CMD_XON:
        case CMD_AUTO:
        case CMD_HEAT:
        case CMD_COOL:
            strcpy_P(cmdstr, ON_P);
            break;
        case CMD_OFF:
        case CMD_HALT:
            strcpy_P(cmdstr, OFF_P);
            break;
        case 0:
    ///    case CMD_SET:
        sendFlags &= ~SEND_COMMAND; // Not send command for parametrized req
            break;
        default:
            debugSerial<<F("Unknown cmd \n");
            sendFlags &= ~SEND_COMMAND;
    }
   }
      //publish to MQTT - OpenHab Legacy style to myhome/s_out/item flat values
        setTopic(addrstr,sizeof(addrstr),T_OUT);
        strncat(addrstr, itemArr->name, sizeof(addrstr)-1);

              if (mqttClient.connected()  && !ethernetIdleCount)
                      {
                      if (sendFlags & SEND_PARAMETERS && chancmd != CMD_OFF && chancmd != CMD_HALT)
                      {
                        mqttClient.publish(addrstr, valstr, true);
                        debugSerial<<F("Pub: ")<<addrstr<<F("->")<<valstr<<endl;

                      }
                      else if (sendFlags & SEND_COMMAND)
                      {
                        mqttClient.publish(addrstr, cmdstr, true);
                        debugSerial<<F("Pub: ")<<addrstr<<F("->")<<cmdstr<<endl;

                      }
                      }
              else
                      {
                      setFlag(sendFlags);
                      return 0;
                      }

            // publush to MQTT - New style to
            // myhome/s_out/item/cmd
            // myhome/s_out/item/set

          if (sendFlags & SEND_PARAMETERS)
             {
              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
              strncat(addrstr, "/", sizeof(addrstr));
              strncat_P(addrstr, SET_P, sizeof(addrstr));


              debugSerial<<F("Pub: ")<<addrstr<<F("->")<<valstr<<endl;
              if (mqttClient.connected()  && !ethernetIdleCount)
                 {
                  mqttClient.publish(addrstr, valstr,true);
                  clearFlag(SEND_PARAMETERS);
                 }
              else
               {
               setFlag(sendFlags);
               return 0;
               }
              }


              if (sendFlags & SEND_COMMAND)
              {
              // Some additional preparing for extended set of commands:
                switch (chancmd) {
                    case CMD_AUTO:
                          strcpy_P(cmdstr, AUTO_P);
                        break;
                    case CMD_HEAT:
                          strcpy_P(cmdstr, HEAT_P);
                        break;
                    case CMD_COOL:
                          strcpy_P(cmdstr, COOL_P);
                        break;
                    case CMD_ON:
                    case CMD_XON:
                          if (itemType == CH_THERMO) strcpy_P(cmdstr, AUTO_P);
                   }

              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr)-1);
              strncat(addrstr, "/", sizeof(addrstr));
              strncat_P(addrstr, CMD_P, sizeof(addrstr));

              debugSerial<<F("Pub: ")<<addrstr<<F("->")<<cmdstr<<endl;
              if (mqttClient.connected()  && !ethernetIdleCount)
                 {
                  mqttClient.publish(addrstr, cmdstr,true);
                  clearFlag(SEND_COMMAND);
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

/////////////////////////////////////////
