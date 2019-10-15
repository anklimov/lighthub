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
#include "homiedef.h"

#ifdef _dmxout
#include "dmx.h"
#include "FastLED.h"
#endif

#ifndef MODBUS_DISABLE
#include <ModbusMaster.h>
#endif

#include <PubSubClient.h>
#include "modules/out_spiled.h"
#include "modules/out_ac.h"

short modbusBusy = 0;
extern aJsonObject *pollingItem;
extern PubSubClient mqttClient;
extern int8_t ethernetIdleCount;

static unsigned long lastctrl = 0;
static aJsonObject *lastobj = NULL;

int txt2cmd(char *payload) {
    int cmd = CMD_UNKNOWN;

    // Check for command
    if (*payload == '-' || (*payload >= '0' && *payload <= '9')) cmd = CMD_NUM;
    else if (strcmp_P(payload, ON_P) == 0) cmd = CMD_ON;
    else if (strcmp_P(payload, OFF_P) == 0) cmd = CMD_OFF;
    else if (strcmp_P(payload, REST_P) == 0) cmd = CMD_RESTORE;
    else if (strcmp_P(payload, TOGGLE_P) == 0) cmd = CMD_TOGGLE;
    else if (strcmp_P(payload, HALT_P) == 0) cmd = CMD_HALT;
    else if (strcmp_P(payload, XON_P) == 0) cmd = CMD_XON;
    else if (strcmp_P(payload, XOFF_P) == 0) cmd = CMD_XOFF;
    else if (strcmp_P(payload, HEAT_P) == 0) cmd = CMD_HEAT;
    else if (strcmp_P(payload, COOL_P) == 0) cmd = CMD_COOL;
    else if (strcmp_P(payload, AUTO_P) == 0) cmd = CMD_AUTO;
    else if (strcmp_P(payload, FAN_ONLY_P) == 0) cmd = CMD_FAN;
    else if (strcmp_P(payload, DRY_P) == 0) cmd = CMD_DRY;
    else if (strcmp_P(payload, TRUE_P) == 0) cmd = CMD_ON;
    else if (strcmp_P(payload, FALSE_P) == 0) cmd = CMD_OFF;
    else if (strcmp_P(payload, ENABLED_P) == 0) cmd = CMD_ON;
    else if (strcmp_P(payload, DISABLED_P) == 0) cmd = CMD_OFF;
    else if (strcmp_P(payload, INCREASE_P) == 0) cmd = CMD_UP;
    else if (strcmp_P(payload, DECREASE_P) == 0) cmd = CMD_DN;
    else if (strcmp_P(payload, HIGH_P) == 0) cmd = CMD_HIGH;
    else if (strcmp_P(payload, MED_P) == 0) cmd = CMD_MED;
    else if (strcmp_P(payload, LOW_P) == 0) cmd = CMD_LOW;
    else if (*payload == '{') cmd = CMD_JSON;
    else if (*payload == '#') cmd = CMD_RGB;
    else if (strncmp_P(payload, HSV_P, strlen (HSV_P)) == 0) cmd = CMD_HSV;
    else if (strncmp_P(payload, RGB_P, strlen (RGB_P)) == 0) cmd = CMD_RGB;
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

        switch (itemType)
        {
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
    driver = NULL;
    if (name && items)
        itemArr = aJson.getObjectItem(items, name);
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


boolean Item::isValid() {
    return (itemArr && (itemArr->type == aJson_Array));
}



/*
void Item::copyPar (aJsonObject *itemV)
{ int n=aJson.getArraySize(itemV);
  //for (int i=aJson.getArraySize(itemVal);i<n;i++) aJson.addItemToArray(itemVal,aJson.createItem(int(0))); //Enlarge array of Values
   for (int i=0;i<n;i++) setPar(i,aJson.getArrayItem(itemV,i)->valueint);
}
*/

/*
#if defined(ARDUINO_ARCH_ESP32)
void analogWrite(int pin, int val)
{
  //TBD
}
#endif
*/

/*
boolean Item::getEnableCMD(int delta) {
    return ((millis() - lastctrl > (unsigned long) delta));// || ((void *)itemArr != (void *) lastobj));
}
*/

#define MAXCTRLPAR 3

// myhome/dev/item/subItem
int Item::Ctrl(char * payload, boolean send, char * subItem){
  if (!payload) return 0;

char* suffix = NULL;
int   suffixCode = 0;
int   setCommand = CMD_SET;  //default SET behavior now - not turn on channels

if (subItem && strlen(subItem))
{
if (suffix = strrchr(subItem, '/')) //Trying to retrieving right part
  {
    *suffix= 0; //Truncate subItem string
    suffix++;
    suffixCode = txt2subItem(suffix);
    // myhome/dev/item/sub.....Item/suffix
    }
else
  {
    suffix = subItem;
    suffixCode = txt2subItem(suffix);
    if (suffixCode)  // some known suffix
          subItem = NULL;
          // myhome/dev/item/suffix

    else //Invalid suffix - fallback to Subitem notation
          suffix = NULL;
    // myhome/dev/item/subItem
  }
} else
{
//suffixCode=S_SET; /// no subItem - To be removed - old compatmode
setCommand = 0; /// Old-style - turn ON by set value
}

int i=0;
while (payload[i]) {payload[i]=toupper(payload[i]);i++;};

int cmd = txt2cmd(payload);
debugSerial<<F("Txt2Cmd:")<<cmd<<endl;

//if (isSet)
//{
  switch (cmd) {
      case CMD_HSV:
       suffixCode=S_HSV; //override code for known payload
      case CMD_NUM:
       {
          short i = 0;
          int Par[3];

          while (payload && i < 3)
              Par[i++] = getInt((char **) &payload);

          return   Ctrl(setCommand, i, Par, send, suffixCode, subItem);
      }
          break;

      case CMD_UNKNOWN: //Not known command
      case CMD_JSON: //JSON input (not implemented yet
          break;
#if not defined(ARDUINO_ARCH_ESP32) and not defined(ESP8266) and not defined(ARDUINO_ARCH_STM32) and not defined(DMX_DISABLE)
      case CMD_RGB: //RGB color in #RRGGBB notation
      {
          suffixCode=S_HSV; //override code for known payload
          CRGB rgb;
          if (sscanf((const char*)payload, "#%2X%2X%2X", &rgb.r, &rgb.g, &rgb.b) == 3) {
              int Par[3];
              CHSV hsv = rgb2hsv_approximate(rgb);
              Par[0] = map(hsv.h, 0, 255, 0, 365);
              Par[1] = map(hsv.s, 0, 255, 0, 100);
              Par[2] = map(hsv.v, 0, 255, 0, 100);
          return    Ctrl(setCommand, 3, Par, send, suffixCode, subItem);
          }
          break;
      }
#endif
      default: //some known command
          return Ctrl(cmd, 0, NULL, send, suffixCode, subItem);

  } //ctrl
//}
}

/*
short Item::cmd2changeActivity(int lastActivity, short defaultCmd)
{
  if  (isActive())
     if (lastActivity) return defaultCmd;
     else return CMD_ON;
  else if (lastActivity) return CMD_OFF;
       else return defaultCmd;
}
*/

int Item::Ctrl(short cmd, short n, int *Parameters, boolean send, int suffixCode, char *subItem) {

    debugSerial<<F("RAM=")<<freeRam()<<F(" Item=")<<itemArr->name<<F(" Sub=")<<subItem<<F(" Suff=")<<suffixCode<<F(" Cmd=")<<cmd<<F(" Par=(");
    if (!itemArr) return -1;
    int Par[MAXCTRLPAR] = {0, 0, 0};
    if (Parameters)
            for (short i=0;i<n && i<MAXCTRLPAR;i++){
              Par[i] = Parameters[i];
              debugSerial<<F("<")<<Par[i]<<F(">");
            }
    debugSerial<<F(")")<<endl;
    int itemCategory = itemType;


    if (itemType == CH_GROUP && !send)
      {
        debugSerial<<F("Skip Grp")<<endl;
        return -1;
      }


    int iaddr = getArg();
    int chActive =isActive();
    CHstore st;
    switch (cmd) {
        int t;
        case CMD_TOGGLE:
            if (chActive>0) cmd = CMD_OFF;
            else cmd = CMD_ON;
            break;

        case CMD_RESTORE:
            if (itemType != CH_GROUP) //individual threating of channels. Ignore restore command for groups
                switch (t = getCmd()) {
                    case CMD_HALT: //previous command was HALT ?
                        debugSerial << F("Restored from:") << t << endl;
                        cmd = CMD_ON;    //turning on
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
                        cmd = CMD_OFF;    //turning Off
                        break;
                    default:
                        debugSerial<<F("XOFF skipped. Prev cmd:")<<t<<endl;
                      return -3;

              }
              break;

    }

    if (driver) //New style modular code
            {
              int res = -1;
              switch (cmd)
              {
                case CMD_XON:
                if (!chActive>0)  //if channel was'nt active before CMD_XON
                      {
                        debugSerial<<F("Turning XON\n");
                        res = driver->Ctrl(CMD_ON, n, Parameters, send, suffixCode, subItem);
                        setCmd(CMD_XON);
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
                        res = driver->Ctrl(CMD_OFF, n, Parameters, send, suffixCode, subItem);
                        setCmd(CMD_HALT);
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
                       res = driver->Ctrl(cmd, n, Parameters, send, suffixCode, subItem);
                       setCmd(CMD_OFF);
                     }
                else
                      {
                        debugSerial<<F("Already Halted\n");
                        return -3;
                      }
                break;
                case CMD_SET:
                res = driver->Ctrl(cmd, n, Parameters, send, suffixCode, subItem);
                break;

                default:
                res = driver->Ctrl(cmd, n, Parameters, send, suffixCode, subItem);
                setCmd(cmd);
              }
              return res;
            }
    // Legacy monolite core code
    bool toExecute = (chActive>0); //if channel is already active - unconditionally propogate changes
    switch (cmd) {
        case 0:       // old style SET - with turning ON
        toExecute = true;
        case CMD_SET: // new style SET - w/o turning ON

        //if (/*itemType !=CH_THERMO && */send) setCmd(CMD_SET); //prevent ON thermostat by semp set ????
      ////////  setCmd(CMD_SET); ///??? trying... no
            switch (itemType) {

                case CH_RGBW: //only if configured VAL array
                    if (!Par[1] && (n == 3)) itemType = CH_WHITE;
                case CH_RGB:
                case CH_GROUP: //Save for groups as well
                st.aslong = getVal();
                   switch (n)
                  {
                   case 1:

                      st.v = Par[0]; //Volume only
                      if (st.hsv_flag)
                      {
                      Par[0] = st.h;
                      Par[1] = st.s;
                      Par[2] = st.v;
                      n = 3;}
                      break;
                   case 2: // Just hue and saturation
                      st.h = Par[0];
                      st.s = Par[1];
                      Par[2] = st.v;
                      st.hsv_flag = 1;
                      n = 3;
                      break;
                   case 3: //complete triplet
                      st.h = Par[0];
                      st.s = Par[1];
                      st.v = Par[2];
                      st.hsv_flag = 1;
                   }
                setVal(st.aslong);
                if (toExecute)
                          { //
                            if (chActive>0 && !st.v) setCmd(CMD_OFF);
                            if (chActive==0 && st.v) setCmd(CMD_ON);
                            SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
                          }
                else    SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                    break;
                case CH_PWM:
                case CH_VC:
                case CH_DIMMER:
                case CH_MODBUS:
                    setVal(Par[0]);          // Store value
//                    setCmd(cmd2changeActivity(chActive,cmd));
                    if (toExecute)
                              { // Not restoring, working
                                if (chActive>0 && !Par[0]) setCmd(CMD_OFF);
                                if (chActive==0 && Par[0]) setCmd(CMD_ON);
                                SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED); // Send back parameter for channel above this line
                              }
                  else    SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
                    break;

                case CH_VCTEMP: // moved
                case CH_THERMO:  ///? wasnt send before/ now will ///
                    setVal(Par[0]);          // Store value
                    if (send) SendStatus(SEND_PARAMETERS | SEND_DEFFERED); // Send back parameter for channel above this line
            }//itemtype

            if (! toExecute && itemType !=CH_GROUP) return 1; // Parameters are stored, no further action required
            break;

        case CMD_XON:
        if (!chActive>0)  //if channel was'nt active before CMD_XON
              {
                debugSerial<<F("Turning XON\n");
//                setCmd(cmd);
              }
          else
          {  //cmd = CMD_ON;
            debugSerial<<F("Already Active\n");
            if (itemType != CH_GROUP) return -3;
          }
        case CMD_ON:


        if (itemType==CH_RGBW && getCmd() == CMD_ON && send /*&& getEnableCMD(500) */) {
                  debugSerial<<F("Force White\n");
                  itemType = CH_WHITE;
                  Par[1] = 0;   //Zero saturation
                  Par[2] = 100; //Full power
                  // Store
                  st.h = Par[0];
                  st.s = Par[1];
                  st.v = Par[2];
                  setVal(st.aslong);
                  setCmd(cmd);
                  //Send to OH
                  if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS );
          break;
        } // if forcewhite

       case CMD_COOL:
       case CMD_AUTO:
       case CMD_HEAT:
       setCmd(cmd);
       if (chActive>0 && send)
                          {
                            SendStatus(SEND_COMMAND);
                            return 1;
                          }
        {
                short params = 0;
                //retrive stored values
                st.aslong = getVal();

                // If command is ON but saved volume to low - setup mimimum volume
                switch (itemType) {
                  case CH_DIMMER:
                  case CH_MODBUS:
                  if (st.aslong<MIN_VOLUME) st.aslong=INIT_VOLUME;
                  setVal(st.aslong);
                  break;
                  case CH_RGB:
                  case CH_RGBW:
                  if (st.aslong && (st.v<MIN_VOLUME)) st.v=INIT_VOLUME;
                  setVal(st.aslong);
                }

                if (st.aslong )  //Stored smthng

                    switch (itemType) {
                        case CH_GROUP:
                            if (send && isActive()==0) SendStatus(SEND_COMMAND);          // Just send ON, suppress send back ON of chan already active
                            break;
                        case CH_RGBW:
                        case CH_RGB:
                            Par[0] = st.h;
                            Par[1] = st.s;
                            Par[2] = st.v;
                            params = 3;
                            if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS); // ???Send restored triplet. In any cases
                            break;

                        case CH_VCTEMP:
                        case CH_PWM:
                        case CH_DIMMER:         //Everywhere, in flat VAL
                        case CH_MODBUS:
                        case CH_VC:
                            Par[0] = st.aslong;
                            params = 1;
                            if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS);  // ???Send restored parameter, even if send=false - no problem, loop will be supressed at next hop
                            break;
                        case CH_THERMO:
                            Par[0] = st.aslong;
                            params = 0;
                            if (send) SendStatus(SEND_COMMAND);  //  Just ON (switch)
                            break;
                        default:
                            if (send) SendStatus(SEND_COMMAND);          // Just send ON
                    }//itemtype switch
                else {// Default settings, values not stored yet
                    debugSerial<<st.aslong<<F(": No stored values - default\n");
                    switch (itemType) {
                        case CH_VCTEMP:
                        case CH_THERMO:
                            Par[0] = 20;   //20 degrees celsium - safe temperature
                            params = 1;
                            setVal(20);
                            if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS); //??? // deffered ???
                            break;
                        case CH_RGBW:
                        case CH_RGB:
                            Par[0] = 100;
                            Par[1] = 0;
                            Par[2] = 100;
                            params = 3;
                            // Store
                            st.h = Par[0];
                            st.s = Par[1];
                            st.v = Par[2];
                            setVal(st.aslong);
                            if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS ); // deffered ???
                            break;
                        case CH_RELAY:
                            Par[0] = 100;
                            params = 1;
                            setVal(100);
                            if (send) SendStatus(SEND_COMMAND); // deffered ???
                            break;
                      default:
                            Par[0] = 100;
                            params = 1;
                            setVal(100);
                            if (send) SendStatus(SEND_COMMAND | SEND_PARAMETERS); //??? // deffered ???
                    }
                } // default handler
                for (short i = 0; i < params; i++) {
                    debugSerial<<F("Restored: ")<<i<<F("=")<<Par[i]<<endl;
                }
/*
            } else {  //Double ON - apply special preset - clean white full power
                if (getEnableCMD(500)) switch (itemType) {
                    case CH_RGBW:
                        debugSerial<<F("Force White"));
                        itemType = CH_WHITE;
                        Par[1] = 0;   //Zero saturation
                        Par[2] = 100; //Full power

                        // Store
                        st.h = Par[0];
                        st.s = Par[1];
                        st.v = Par[2];
                        setVal(st.aslong);

                        //Send to OH
                        if (send) SendStatus(0, 3, Par);
                        break;
                } //itemtype
            } //else
*/
            //debugSerial<<"Sa:");debugSerial<<Par[1]);
            if ((itemType == CH_RGBW) && (Par[1] == 0)) itemType = CH_WHITE;
          }

            break; //CMD_ON

        case CMD_OFF:
            if (getCmd() != CMD_HALT) //if channels are halted - OFF to be ignored (restore issue)
            {
                Par[0] = 0;
                Par[1] = 0;
                Par[2] = 0;
                setCmd(cmd);
                if (send) SendStatus(SEND_COMMAND);
            }
            break;

        case CMD_HALT:

            if (chActive>0) {
                Par[0] = 0;
                Par[1] = 0;
                Par[2] = 0;
                if (getCmd() == CMD_XON) setCmd(CMD_OFF); //Prevent restoring temporary turned on channels (by XON)
                    else setCmd(cmd);
                SendStatus(SEND_COMMAND); //HALT to OFF mapping - send in any cases
                debugSerial<<F(" Halted\n");
            }


    }//switch cmd




    int rgbSaturation =map(Par[1], 0, 100, 0, 255);
    int rgbValue = map(Par[2], 0, 100, 0, 255);
    switch (itemType) {

#ifdef _dmxout
        case CH_DIMMER: //Dimmed light
            if (iaddr>0)
            DmxWrite(iaddr, map(Par[0], 0, 100, 0, 255));
            break;
        case CH_RGBW: //Colour RGBW
        // Saturation 0  - Only white
        // 0..50 - white + RGB
        //50..100 RGB
        {
            int k;
            if (Par[1]<50 && iaddr>0) { // Using white
                            DmxWrite(iaddr + 3, map((50 - Par[1]) * Par[2], 0, 5000, 0, 255));
                            int rgbvLevel = map (Par[1],0,50,0,255*2);
                            rgbValue = map(Par[2], 0, 100, 0, rgbvLevel);
                            rgbSaturation = map(Par[1], 0, 50, 255, 100);
                            if (rgbValue>255) rgbValue = 255;
                           }
            else
            {
              //rgbValue = map(Par[2], 0, 100, 0, 255);
              rgbSaturation = map(Par[1], 50, 100, 100, 255);
              if (iaddr>0) DmxWrite(iaddr + 3, 0);
            }
            //DmxWrite(iaddr + 3, k = map((100 - Par[1]) * Par[2], 0, 10000, 0, 255));
            //debugSerial<<F("W:")<<k<<endl;
        }
        case CH_RGB: // RGB
        if (iaddr>0)
        {
            CRGB rgb = CHSV(map(Par[0], 0, 365, 0, 255), rgbSaturation, rgbValue);
            DmxWrite(iaddr, rgb.r);
            DmxWrite(iaddr + 1, rgb.g);
            DmxWrite(iaddr + 2, rgb.b);
            break;
        }
        case CH_WHITE:
        if (iaddr>0)
        {
            DmxWrite(iaddr, 0);
            DmxWrite(iaddr + 1, 0);
            DmxWrite(iaddr + 2, 0);
            DmxWrite(iaddr + 3, map(Par[2], 0, 100, 0, 255));
            break;
          }
#endif

#ifdef _modbus
        case CH_MODBUS:
        modbusDimmerSet(Par[0]);
        break;
#endif


        case CH_GROUP://Group
        {
            if (itemArg->type == aJson_Array) {
                aJsonObject *i = itemArg->child;
                while (i) {
                    Item it(i->valuestring);
                    it.Ctrl(cmd, n, Par, send,suffixCode,subItem); //// was true
                    i = i->next;
                } //while
            } //if
        } //case
            break;
        case CH_RELAY:
           if (iaddr)
           {
            int k;
            short inverse = 0;

            if (iaddr < 0) {
                iaddr = -iaddr;
                inverse = 1;
            }
            pinMode(iaddr, OUTPUT);

            if (inverse)
                digitalWrite(iaddr, k = ((cmd == CMD_ON || cmd == CMD_XON) ? LOW : HIGH));
            else
                digitalWrite(iaddr, k = ((cmd == CMD_ON || cmd == CMD_XON) ? HIGH : LOW));
            debugSerial<<F("Pin:")<<iaddr<<F("=")<<k<<endl;
            break;
          }
        case CH_THERMO:
                ///thermoSet(name,cmd,Par1); all activities done - update temp & cmd
                break;

        case CH_PWM:
           if (iaddr)
           {
            int k;
            short inverse = 0;

            if (iaddr < 0) {
                iaddr = -iaddr;
                inverse = 1;
            }
            pinMode(iaddr, OUTPUT);
            //timer 0 for pin 13 and 4
            //timer 1 for pin 12 and 11
            //timer 2 for pin 10 and 9
            //timer 3 for pin 5 and 3 and 2
            //timer 4 for pin 8 and 7 and 6
            //prescaler = 1 ---> PWM frequency is 31000 Hz
            //prescaler = 2 ---> PWM frequency is 4000 Hz
            //prescaler = 3 ---> PWM frequency is 490 Hz (default value)
            //prescaler = 4 ---> PWM frequency is 120 Hz
            //prescaler = 5 ---> PWM frequency is 30 Hz
            //prescaler = 6 ---> PWM frequency is <20 Hz
            int tval = 7;             // this is 111 in binary and is used as an eraser
#if defined(__AVR_ATmega2560__)
            TCCR4B &= ~tval;   // this operation (AND plus NOT),  set the three bits in TCCR2B to 0
            TCCR3B &= ~tval;
            tval = 2;
            TCCR4B |= tval;
            TCCR3B |= tval;
#endif
            if (inverse) k = map(Par[0], 100, 0, 0, 255);
            else k = map(Par[0], 0, 100, 0, 255);
            analogWrite(iaddr, k);
            debugSerial<<F("Pin:")<<iaddr<<F("=")<<k<<endl;
            break;
        }
#ifdef _modbus
        case CH_VC:
            VacomSetFan(Par[0], cmd);
            break;
        case CH_VCTEMP: {
            Item it(itemArg->valuestring);
            if (it.isValid() && it.itemType == CH_VC)
                VacomSetHeat(it.getArg(), Par[0], cmd);
            break;
        }
#endif
    }
}

int Item::isActive() {
    CHstore st;
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
                    Item it(i->valuestring);

                    if (it.isValid() && it.isActive()>0) {
                        debugSerial<<F(" active\n");
                        return 1;
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
#ifdef _modbus
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
                if (_maxval) modbusDimmerSet(_addr, _reg, _regType, _mask, map(value, 0, 100, 0, _maxval));
                             else modbusDimmerSet(_addr, _reg, _regType, _mask, value);
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
    int addr = getArg();
    debugSerial<<F("VC#")<<addr<<F("=")<<val<<endl;
    if (modbusBusy) {
        setCmd(cmd);
        setVal(val);
        mb_fail();
        return -1;
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
    node.writeSingleRegister(2003 - 1, val * 100);
    modbusBusy = 0;
}

#define a 0.1842f
#define b -36.68f

int Item::VacomSetHeat(int addr, int8_t val, int8_t cmd) {

    debugSerial<<F("VC_heat#")<<addr<<F("=")<<val<<F(" cmd=")<<cmd<<endl;
    if (modbusBusy) {
      setCmd(cmd);
      setVal(val);
      mb_fail();
        return -1;
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
    node.writeSingleRegister(2004 - 1, regval);
    modbusBusy = 0;
}

int Item::modbusDimmerSet(int addr, uint16_t _reg, int _regType, int _mask, uint16_t value) {

    if (_regType != MODBUS_COIL_REG_TYPE || _regType != MODBUS_HOLDING_REG_TYPE) {

    }

    if (modbusBusy) {
      mb_fail();
        return -1;
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
            node.writeSingleRegister(_reg, value);
            break;
        case MODBUS_COIL_REG_TYPE:
            node.writeSingleCoil(_reg, value);
            break;
        default:
            debugSerial<<F("Not supported reg type\n");
    }
    modbusBusy = 0;
}

int Item::checkFM() {
    if (modbusBusy) return -1;
    if (checkModbusRetry()) return -2;
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
            if (airGateObj) {
                int val = 100;
                Item item(airGateObj->valuestring);
                if (item.isValid())
                    item.Ctrl(0, 1, &val);
            }
        }
    } else
        debugSerial << F("Modbus polling error=") << _HEX(result) << endl;

    if (node.getResponseBuffer(0) & 8) //Active fault
    {
        result = node.readHoldingRegisters(2111 - 1, 1);
        if (result == node.ku8MBSuccess) aJson.addNumberToObject(out, "flt", (int) node.getResponseBuffer(0));
        modbusBusy=0;
        if (isActive()>0) Ctrl(CMD_OFF); //Shut down ///
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
            Ctrl(CMD_OFF); //Shut down
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
    int cmd = getCmd();
    if (getFlag(SEND_RETRY)) {   // if last sending attempt of command was failed
      int val = getVal();
      debugSerial<<F("Retrying CMD\n");
      clearFlag(SEND_RETRY);     // Clean retry flag
      //Ctrl(cmd,1,&val);      // Execute command again
      modbusDimmerSet(val);
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

int Item::Poll() {

    if (driver && driver->Status())
                        {
                        driver->Poll();
                        return INTERVAL_POLLING; //TODO refactoring
                        }
    // Legacy polling
    switch (itemType) {
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
    return INTERVAL_POLLING;
}

void Item::sendDelayedStatus()
{
      if (getFlag(SEND_COMMAND | SEND_PARAMETERS))
      {
      SendStatus(SEND_COMMAND | SEND_PARAMETERS);
      clearFlag(SEND_COMMAND | SEND_PARAMETERS);
      }
}

#endif
int Item::SendStatus(int sendFlags) {
    int chancmd=getCmd();
    if (sendFlags & SEND_DEFFERED) {
        setFlag(sendFlags & (SEND_COMMAND | SEND_PARAMETERS));
        debugSerial<<F("Status deffered\n");
        return -1;
    }
    else {
      sendFlags |= getFlag(SEND_COMMAND | SEND_PARAMETERS); //if some delayed status is pending
      char addrstr[48];
      char valstr[16] = "";
      char cmdstr[8] = "";

      if (sendFlags & SEND_PARAMETERS)
      {
      // Preparing parameters payload //////////
       CHstore st;
       //retrive stored values
       st.aslong = getVal();
       switch (itemType) {
               //case CH_GROUP:
               case CH_RGBW:
               case CH_RGB:
               snprintf(valstr, sizeof(valstr), "%d,%d,%d", st.h,st.s,st.v);
            break;
               case CH_GROUP:
               if (st.hsv_flag)
                snprintf(valstr, sizeof(valstr), "%d,%d,%d", st.h,st.s,st.v);
               else
                snprintf(valstr, sizeof(valstr), "%d", st.v);
            break;
               default:
               snprintf(valstr, sizeof(valstr), "%d", st.aslong);
           }//itemtype
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
        case CMD_SET:
        sendFlags &= ~SEND_COMMAND; // Not send command for parametrized req
            break;
        default:
            debugSerial<<F("Unknown cmd \n");
            sendFlags &= ~SEND_COMMAND;
    }
   }
      //publish to MQTT - OpenHab Legacy style to myhome/s_out/item flat values
        setTopic(addrstr,sizeof(addrstr),T_OUT);
        strncat(addrstr, itemArr->name, sizeof(addrstr));

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
              strncat(addrstr, itemArr->name, sizeof(addrstr));
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
                          if (itemType == CH_THERMO) strcpy_P(cmdstr, HEAT_P);
                   }

              setTopic(addrstr,sizeof(addrstr),T_OUT);
              strncat(addrstr, itemArr->name, sizeof(addrstr));
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



/////////////////////////////////////////
