#ifdef SPRINKLER_ENABLE

#include "modules/out_sprinkler.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "utils.h"

bool out_sprinkler::getConfig()
{
  gatesObj = NULL;
  vinPin = drenPin = pumpPin = PINS_COUNT;
  wMaxPin = wMinPin = fbDrenPin = fbPumpPin = wCtrPin = PINS_COUNT;

  if (!item || !item->itemArg) return false;

  aJsonObject * arg = item->itemArg;
  if (arg->type == aJson_Array && aJson.getArraySize(arg) > 1)
  {
    aJsonObject * second = aJson.getArrayItem(arg, 1);
    if (second && second->type == aJson_Object) gatesObj = second;
  }
  else if (arg->type == aJson_Object)
  {
    gatesObj = arg;
  }

  if (!gatesObj) return false;

  aJsonObject * rootCfg = aJson.getObjectItem(gatesObj, "");
  if (!rootCfg) rootCfg = gatesObj;

  vinPin    = getIntFromJson(rootCfg, "vIn", PINS_COUNT);
  drenPin   = getIntFromJson(rootCfg, "rDren", PINS_COUNT);
  pumpPin   = getIntFromJson(rootCfg, "rPump", PINS_COUNT);
  wMaxPin   = getIntFromJson(rootCfg, "wMax", PINS_COUNT);
  wMinPin   = getIntFromJson(rootCfg, "wMin", PINS_COUNT);
  fbDrenPin = getIntFromJson(rootCfg, "fbDren", PINS_COUNT);
  fbPumpPin = getIntFromJson(rootCfg, "fbPump", PINS_COUNT);
  wCtrPin   = getIntFromJson(rootCfg, "wCtr", PINS_COUNT);

  return true;
}

static bool isValidControlPin(short pin)
{
  return (abs(pin) < PINS_COUNT && !isProtectedPin(abs(pin)));
}

void out_sprinkler::publishBooleanStateIfChanged(const char * subItem, bool state, uint32_t flag, uint32_t & lastState)
{
  if (state == (bool) (lastState & flag)) return;
  if (state) lastState |= flag ; else lastState &= ~flag;
  publishBooleanState(subItem, state);
}

void out_sprinkler::setOutput(short pin, bool value)
{
  writeOutPin(pin, value ? HIGH : LOW);
}

int out_sprinkler::Setup()
{
  abstractOut::Setup();

  if (!getConfig())
  {
    debugSerial << F("SPRINKLER: config failed") << endl;
    return 0;
  }

  if (isValidControlPin(vinPin)) { pinMode(vinPin, OUTPUT); writeOutPin(vinPin, LOW); }
  if (isValidControlPin(drenPin)) { pinMode(drenPin, OUTPUT); writeOutPin(drenPin, LOW); }
  if (isValidControlPin(pumpPin)) { pinMode(pumpPin, OUTPUT); writeOutPin(pumpPin, LOW); }

  debugSerial << F("SPRINKLER: ")<<"vIN=" << vinPin << " dren=" << drenPin << " pump=" << pumpPin << endl;

  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      short pin = getIntFromJson(zone, "pin", PINS_COUNT);
      if (isValidControlPin(pin)) { pinMode(pin, OUTPUT); writeOutPin(pin, LOW); }
      getCreateObject(zone, "cmd", (long)CMD_OFF);
      getCreateObject(zone, "val", (long)0);
      getCreateObject(zone, "set", (long)0);
      getCreateObject(zone, "@active", (long)0);
    }
    zone = zone->next;
  }

  getCreateObject(gatesObj, "@state", (long)SP_UNKNOWN);
  getCreateObject(gatesObj, "@timer", (long)0);
  getCreateObject(gatesObj, "@flowTimer", (long)0);
  
  debugSerial << F("SPRINKLER: ") << " wMax=" << wMaxPin << " wMin=" << wMinPin << " fbDren=" << fbDrenPin << " fbPump=" << fbPumpPin << " wCtr=" << wCtrPin << endl;
  uint16_t lastVals = 0;
  if (abs(wCtrPin) < PINS_COUNT) 
      {      
        setupInPin(wCtrPin);
        lastVals |= (readInPin(wCtrPin) ? LASTWCTRLSTATE : 0);
        lastVals |= (readInPin(wCtrPin) ? LASTWCTRLSTATE_ALL : 0);
      }
  if (wMaxPin != PINS_COUNT) {
    setupInPin(wMaxPin);
    lastVals |= (publishBooleanState("/$wMax", readInPin(wMaxPin)) ? LASTWMAXSTATE : 0);
  }
  if (wMinPin != PINS_COUNT) {
    setupInPin(wMinPin);
    lastVals |= (publishBooleanState("/$wMin", readInPin(wMinPin)) ? LASTWMINSTATE : 0);
  }
  if (fbDrenPin != PINS_COUNT) {
    setupInPin(fbDrenPin);
    lastVals |= (publishBooleanState("/$fbDren", readInPin(fbDrenPin)) ? LASTFBDRENSTATE : 0);
  }
  if (fbPumpPin != PINS_COUNT) {
    setupInPin(fbPumpPin);
    lastVals |= (publishBooleanState("/$fbPump", readInPin(fbPumpPin)) ? LASTFBPUMPSTATE : 0);
  }

  setValToJson(gatesObj, "@lastVals", (long)lastVals);

  //item->setExt(millisNZ());
  setStatus(CST_INITIALIZED);
  notifyState(SP_UNKNOWN);
  return 1;
}

int out_sprinkler::Stop()
{
  debugSerial << F("SPRINKLER: stop") << endl;
  turnOffAllZones();
  pump(false);
  dren(false);
  setOutput(vinPin, false);
  setStatus(CST_UNKNOWN);
  return 1;
}



bool out_sprinkler::isFreeze()
{
  if (!item) return false;
  return (item->getFlag(FLAG_FREEZED));
}

bool out_sprinkler::isNeedPump(bool steelNeed)
{
  if (!gatesObj) return false;
  if (!steelNeed && (!item || item->getCmd() != CMD_ON)) return false;

  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      if (getIntFromJson(zone, "@active", 0)) return true;
      int cmd = getIntFromJson(zone, "cmd", CMD_OFF);
      if (cmd == CMD_ON)
      {
        long setVal = getIntFromJson(zone, "set", 0);
        long valVal = getIntFromJson(zone, "val", 0);
        if (!setVal || (valVal < setVal)) return true;
      }
    }
    zone = zone->next;
  }
  return false;
}

void out_sprinkler::pump(bool state)
{
  if (!isValidControlPin(pumpPin)) return;
  uint32_t lastVals = getIntFromJson (gatesObj, "@lastVals", 0);
  setOutput(pumpPin, state);
  if (state != (bool)(lastVals & LASTPUMPSTATE))
  {
    if(state) lastVals |= LASTPUMPSTATE; else lastVals &= ~LASTPUMPSTATE;
    setValToJson(gatesObj, "@lastVals", (long)lastVals);
    publishBooleanState("/$rPump", state);
  }
}

void out_sprinkler::dren(bool state)
{
  if (!isValidControlPin(drenPin)) return;
  uint32_t lastVals = getIntFromJson (gatesObj, "@lastVals", 0);
  setOutput(drenPin, state);
  if (state != (bool)(lastVals & LASTDRENSTATE))
  {
    if(state) lastVals |= LASTDRENSTATE; else lastVals &= ~LASTDRENSTATE;
    setValToJson(gatesObj, "@lastVals", (long)lastVals);
    publishBooleanState("/$rDren", state);
  }
}


void out_sprinkler::vin(bool state)
{
  if (!isValidControlPin(vinPin)) return;
  uint32_t lastVals = getIntFromJson (gatesObj, "@lastVals", 0);
  setOutput(vinPin, state);
  if (state != (bool)(lastVals & LASTVINSTATE))
  {
    if(state) lastVals |= LASTVINSTATE; else lastVals &= ~LASTVINSTATE;
    setValToJson(gatesObj, "@lastVals", (long)lastVals);
    publishBooleanState("/$rVIN", state);
  }
}

void out_sprinkler::turnOffAllZones()
{
  if (!gatesObj) return;
  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      short pin = getIntFromJson(zone, "pin", PINS_COUNT);
      if (isValidControlPin(pin)) setOutput(pin, false);
      if (getIntFromJson(zone, "@active", 0))
      {
        setZoneActive(zone, false);
      }
    }
    zone = zone->next;
  }
}

void out_sprinkler::turnOffValves()
{
  turnOffAllZones();
  setOutput(vinPin, false);
  setOutput(drenPin, false);
}

//void out_sprinkler::notifyState(short state)
//{
//  if (!gatesObj) return;
//  setValToJson(gatesObj, "@state", (long)state);
//  publishNumericState("$state", state);
//}



void  out_sprinkler::notifyState(short state)
{
char val[16];  
long fault = 0;

  if (!gatesObj) return;
  setValToJson(gatesObj, "@state", (long)state);
aJsonObject * rootCfg = aJson.getObjectItem(gatesObj, "");
if (!rootCfg) rootCfg = gatesObj;
aJsonObject * faultObj = aJson.getObjectItem(rootCfg, "onFault");
switch (state) {

    case SP_OFF:
        strcpy(val,"OFF");
        break;

    case SP_DREN_ON:
        strcpy(val,"DREN_ON");
        break;

    case SP_DREN_OPERATE:
        strcpy(val,"DREN_OPERATE");
        break;

    case SP_DREN_EMPTY:
        strcpy(val,"DREN_EMPTY");
        break;

    case SP_VIN:
        strcpy(val,"VIN");
        break;
    case SP_FULL:

        strcpy(val,"FULL");
        break;

    case SP_FAULT_VIN:
        strcpy(val,"FAULT_VIN");
        fault = 1;
        break;
 
    case SP_FAULT_DREN:
        strcpy(val,"FAULT_DREN");
        fault = 2;
        break;

        
    default:
        strcpy(val,"UNKNOWN");
        break;

}
//if (fault) strcpy(val,"FAULT");

if (faultObj) 
          {
          executeCommand(faultObj,-1,itemCmd().Int((int32_t) fault));  
          }
 else publishTopic(item->itemArr->name,fault,"/$fault");

publishTopic(item->itemArr->name,val,"/$state");
}



int out_sprinkler::moveToState(sprinklerState nextState)
{
  if (!gatesObj) return 0;

  switch (nextState)
  {
    case SP_OFF:
    case SP_FULL:
      dren(false);
      vin(false);
      break;
    case SP_DREN_ON:
    case SP_DREN_OPERATE:
      dren(true);
      vin(false); 
      break;
    case SP_VIN:
      vin(true);
      //setOutput(drenPin, false);
      break;
    case SP_DREN_EMPTY:
      //setOutput(drenPin, false);
      vin(false);
      break;
    case SP_FAULT_VIN:
      vin(false);
      break;
    case SP_FAULT_DREN:
      dren(false);
      break;
    case SP_UNKNOWN:
      dren(false);
      vin(false);
      break;
  }

  //publishBooleanState("/$rDren", nextState == SP_DREN_ON || nextState == SP_DREN_OPERATE);
  //publishBooleanState("/$vIN", nextState == SP_VIN);
  notifyState(nextState);
  return 1;
}

inline aJsonObject * out_sprinkler::getZone(const char * name)
{
  if (!gatesObj || !name || !*name) return NULL;
  aJsonObject * zone = aJson.getObjectItem(gatesObj, name);
  if (zone && zone->type == aJson_Object) return zone;
  return NULL;
}

inline aJsonObject * out_sprinkler::findNextZone()
{
  if (!gatesObj) return NULL;

  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      if (getIntFromJson(zone, "@active", 0)) return zone;
    }
    zone = zone->next;
  }

  zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      int cmd = getIntFromJson(zone, "cmd", CMD_OFF);
      long setVal = getIntFromJson(zone, "set", 0);
      long valVal = getIntFromJson(zone, "val", 0);
      if (cmd == CMD_ON && (!setVal  || valVal < setVal)) return zone;
    }
    zone = zone->next;
  }
  return NULL;
}

/* 

*/
void out_sprinkler::setZoneActive(aJsonObject * zone, bool active)
{
  if (!zone) return;
  setValToJson(zone, "@active", (long)(active ? 1 : 0));

  char subItem[48];
  snprintf(subItem, sizeof(subItem), "/%s/$state", zone->name);
  publishTopic(item->itemArr->name, active ? "ON": "OFF", subItem);
}

void out_sprinkler::updateZoneValue(aJsonObject * zone, long value)
{
  if (!zone) return;
  long current = getIntFromJson(zone, "val", 0);
  current += value;
  setValToJson(zone, "val", current);
  item->SendStatusImmediate(itemCmd().Int(current).setSuffix(S_VAL), FLAG_PARAMETERS, zone->name);
}

void out_sprinkler::updateCounterValue()
{
  int value = 1;
  if (!gatesObj) return;
  long current = getIntFromJson(gatesObj, "@wCtr", 0);
  current += value;
  setValToJson(gatesObj, "@wCtr", current);
  item->SendStatusImmediate(itemCmd().Int(current).setSuffix(S_SET), FLAG_PARAMETERS);
}


bool out_sprinkler::publishBooleanState(const char * subItem, bool state)
{
  if (!item) return state;

  publishTopic(item->itemArr->name,state ? "ON": "OFF", subItem);
  return state;
}


int out_sprinkler::Poll(short cause)
{
  if (!item || !gatesObj) return 0;


  bool freeze = isFreeze();
  bool wMax   = (wMaxPin != PINS_COUNT) ? readInPin(wMaxPin) : false;
  bool wMin   = (wMinPin != PINS_COUNT) ? readInPin(wMinPin) : false;
  bool fbDren = (fbDrenPin != PINS_COUNT) ? readInPin(fbDrenPin) : false;
  bool fbPump = (fbPumpPin != PINS_COUNT) ? readInPin(fbPumpPin) : false;

  uint32_t lastVals = getIntFromJson(gatesObj, "@lastVals", 0);

  publishBooleanStateIfChanged("/$wMax", wMax, LASTWMAXSTATE, lastVals);
  publishBooleanStateIfChanged("/$wMin", wMin, LASTWMINSTATE, lastVals);
  publishBooleanStateIfChanged("/$fbDren", fbDren, LASTFBDRENSTATE, lastVals);
  publishBooleanStateIfChanged("/$fbPump", fbPump, LASTFBPUMPSTATE, lastVals);

  uint32_t now = millisNZ();
  int state = getIntFromJson(gatesObj, "@state", SP_UNKNOWN);
  uint32_t timer = (uint32_t)getIntFromJson(gatesObj, "@timer", 0);
  bool lastWctrlState = lastVals & LASTWCTRLSTATE;
  bool lastWctrlStateAll = lastVals & LASTWCTRLSTATE_ALL;

  if (abs(wCtrPin) < PINS_COUNT)
      {
        bool curr = readInPin(wCtrPin);
        if (curr && !lastWctrlStateAll)
        {
          updateCounterValue();         
        }
        if (curr) lastVals |= LASTWCTRLSTATE_ALL; else lastVals &= ~LASTWCTRLSTATE_ALL;
      }
  setValToJson(gatesObj, "@lastVals", (long)lastVals);

  if (freeze)
  {
    moveToState(SP_OFF);
    turnOffValves();
    pump(false);
    return 0;
  }

  switch (state)
  {
    case SP_UNKNOWN:
    case SP_OFF:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        notifyState(state);
      }
      else
      {
        state = SP_DREN_ON;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_ON);
      }
      break;

    case SP_DREN_ON:
      if (fbDren)
      {
        state = SP_DREN_OPERATE;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_OPERATE);
      }
      else if (isTimeOver(timer, now, DRENAGE_ON_TIME))
      {
        state = SP_DREN_EMPTY;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_EMPTY);
      }
      break;

    case SP_DREN_OPERATE:
      if (!fbDren)
      {
        state = SP_DREN_EMPTY;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_EMPTY);
      }
      else if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FULL);
      }
      else if (isTimeOver(timer, now, DRENAGE_MAX_TIME))
      {
        state = SP_FAULT_DREN;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FAULT_DREN);
      }
      break;

    case SP_DREN_EMPTY:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FULL);
      }
      else if (item->getCmd() == CMD_ON)
      {
        state = SP_VIN;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_VIN);
      }
      break;

    case SP_VIN:
      if (fbDren)
      {
        state = SP_DREN_OPERATE;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_OPERATE);
      }
      else if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FULL);
      }
      else if (isTimeOver(timer, now, VIN_MAX_TIME))
      {
        state = SP_FAULT_VIN;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FAULT_VIN);
      }
      break;

    case SP_FULL:
      if (!wMax)
      {
        state = SP_DREN_ON;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_DREN_ON);
      }
      break;

    case SP_FAULT_VIN:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FULL);
      }
      break;

    case SP_FAULT_DREN:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        moveToState(SP_FULL);
      }
      break;
  }

  bool tankReady = (state == SP_FULL || wMax || wMin || fbPump);
  bool needPump = false;
  aJsonObject * currentZone = NULL;

  if (item->getCmd() == CMD_ON && !tankReady)
  {
    turnOffAllZones();
    pump(false);
    return 0;
  }

  if (item->getCmd() == CMD_ON && tankReady)
  {
    currentZone = findNextZone();
    if (currentZone)
    {
      long setVal = getIntFromJson(currentZone, "set", 0);
      long valVal = getIntFromJson(currentZone, "val", 0);

      if (!getIntFromJson(currentZone, "@active", 0))
      {
        turnOffAllZones();
        setZoneActive(currentZone, true);
        short zonePin = getIntFromJson(currentZone, "pin", PINS_COUNT);
        if (isValidControlPin(zonePin)) setOutput(zonePin, true);
        setValToJson(gatesObj, "@flowTimer", (long)now);
      }

      if (abs(wCtrPin) < PINS_COUNT)
      {
        bool curr = readInPin(wCtrPin);
        if (curr && !lastWctrlState)
        {
          updateZoneValue(currentZone, 1);
        }
        if (curr) lastVals |= LASTWCTRLSTATE; else lastVals &= ~LASTWCTRLSTATE;
        setValToJson(gatesObj, "@lastVals", (long)lastVals);
      }
      else
      {
        uint32_t flowTimer = (uint32_t)getIntFromJson(gatesObj, "@flowTimer", now);
        if (isTimeOver(flowTimer, now, 1000UL))
        {
          updateZoneValue(currentZone, 1);
          setValToJson(gatesObj, "@flowTimer", (long)now);
        }
      }


      if (setVal > 0 && valVal >= setVal)
      {
        short zonePin = getIntFromJson(currentZone, "pin", PINS_COUNT);
        if (isValidControlPin(zonePin)) setOutput(zonePin, false);
        
        setZoneActive(currentZone, false);
        //////setValToJson(currentZone, "cmd", (long)CMD_OFF);
        //item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, currentZone->name);
        currentZone = findNextZone();
      }

      if (currentZone)
      {
        needPump = true;
      }
    }
  }

  if (!needPump)
  {
    pump(false);
    if (item->getCmd() == CMD_ON)
    {
      aJsonObject * resultZone = findNextZone();
      if (!resultZone)
      {
        item->setCmd(CMD_OFF);
        item->SendStatus(FLAG_COMMAND);
      }
    }
  }
  else
  {
    pump(true);
  }

  return 0;
}

int out_sprinkler::Ctrl(itemCmd cmd, char* subItem, bool toExecute, bool authorized)
{
  if (!item || !gatesObj) return 0;
  int suffixCode = cmd.isCommand() ? S_CMD : cmd.getSuffix();
  debugSerial << "SPRINKLER: CTRL " << subItem << " "<< "Execute:"<< toExecute << " "; cmd.debugOut();
  bool sendStatus = isNotRetainingStatus();

  if (subItem && *subItem)
  {
    aJsonObject * zone = getZone(subItem);
    if (!zone) return 0;
///// FOR ZONES
    switch (suffixCode)
    {
      case S_SET:
//        if (toExecute)
        {
          long value = cmd.getInt();
          setValToJson(zone, "set", value);
          if (sendStatus)
          {
            item->SendStatusImmediate(itemCmd().Int(value).setSuffix(S_SET), FLAG_PARAMETERS, subItem);
          }
        }
        return 1;

      case S_VAL:
//        if (toExecute)
        {
          long value = cmd.getInt();
          setValToJson(zone, "val", value);
          if (sendStatus)
          {
            item->SendStatusImmediate(itemCmd().Int(value).setSuffix(S_VAL), FLAG_PARAMETERS, subItem);
          }
        }
        return 1;

      case S_CMD:
      default:
        switch (cmd.getCmd())
        {
          case CMD_ON:
            setValToJson(zone, "cmd", (long)CMD_ON);
            if (sendStatus)
            { 
              item->SendStatusImmediate(itemCmd().Cmd(CMD_ON).setSuffix(S_CMD), FLAG_COMMAND, subItem);
            }
            return 1;

          case CMD_OFF:
            setValToJson(zone, "cmd", (long)CMD_OFF);
            setZoneActive(zone, false);
            setOutput(getIntFromJson(zone, "pin", PINS_COUNT), false);

            if (sendStatus)
            {
              item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, subItem);
            }
            return 1;

          case CMD_RESET:
            setValToJson(zone, "val", (long)0);
            if (sendStatus)
            {
              item->SendStatusImmediate(itemCmd().Int(0).setSuffix(S_VAL), FLAG_PARAMETERS, subItem);
            }
            return 1;

          default:
            return 0;
        }
    }
  }
/// FOR SPRINKLER CONTROLLER
  switch (suffixCode)
  {
    case S_CMD:
      switch (cmd.getCmd())
      {
        case CMD_ON:
          return 1;

        case CMD_OFF:
          turnOffAllZones();
          pump(false);
          //dren(false);
          return 1;

        case CMD_RESET:
          {
            aJsonObject * zone = gatesObj->child;
            while (zone)
            {
              if (zone->name && *zone->name && zone->type == aJson_Object)
              {
                setValToJson(zone, "val", (long)0);
                if (sendStatus)
                {
                  item->SendStatusImmediate(itemCmd().Int(0).setSuffix(S_VAL), FLAG_PARAMETERS, zone->name);
                }
              }
              zone = zone->next;
            }
                      turnOffAllZones();
            pump(false); 
            item->Off();  
          }
          return 1;

        default:
          break;
      }
      break;

    case S_SET:
    {
        debugSerial << F("SPRINKLER: set wCtr to ") << cmd.getInt() << endl;
        long value = cmd.getInt();
        setValToJson(gatesObj, "@wCtr", value);

      if (sendStatus)
      {
        item->SendStatusImmediate(itemCmd().Int(0).setSuffix(S_SET), FLAG_PARAMETERS);
      }
      return 1;
    }
    case S_VAL:
    {
       debugSerial << F("SPRINKLER: ext temperature: ") << cmd.getInt() << endl;
        long value = cmd.getInt();
        if (value < 0)
        {
          item->setFlag(FLAG_FREEZED);
          item->SendStatus(FLAG_FLAGS);
        }
//       else if (isFreeze())
//        {
 //         item->clearFlag(FLAG_FREEZED);
  //        item->SendStatus(FLAG_FLAGS);
    //    }
     
      return 1;
    }

  //  default:
    //  break;
  }
  return 0;
}

int out_sprinkler::getChanType()
{
  return CH_COUNTER;
}

#endif // SPRINKLER_ENABLE