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
  vinPin = drenPin = pumpPin = -1;
  wMaxPin = wMinPin = fbDrenPin = fbPumpPin = wCtrPin = -1;

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

  vinPin    = getIntFromJson(rootCfg, "vIn", -1);
  drenPin   = getIntFromJson(rootCfg, "rDren", -1);
  pumpPin   = getIntFromJson(rootCfg, "rPump", -1);
  wMaxPin   = getIntFromJson(rootCfg, "wMax", -1);
  wMinPin   = getIntFromJson(rootCfg, "wMin", -1);
  fbDrenPin = getIntFromJson(rootCfg, "fbDren", -1);
  fbPumpPin = getIntFromJson(rootCfg, "fbPump", -1);
  wCtrPin   = getIntFromJson(rootCfg, "wCtr", -1);

  return true;
}

static bool isValidControlPin(short pin)
{
  return (pin >= 0 && pin < PINS_COUNT && !isProtectedPin(pin));
}

void out_sprinkler::publishBooleanStateIfChanged(const char * subItem, bool state, uint32_t flag, uint32_t & lastState)
{
  if (state == (bool) (lastState & flag)) return;
  if (state) lastState |= flag ; else lastState &= ~flag;
  publishBooleanState(subItem, state);
}

void out_sprinkler::setOutput(short pin, bool value)
{
  if (!isValidControlPin(pin)) return;
  digitalWrite(pin, value ? HIGH : LOW);
}

int out_sprinkler::Setup()
{
  abstractOut::Setup();

  if (!getConfig())
  {
    debugSerial << F("SPRINKLER: config failed") << endl;
    return 0;
  }

  if (isValidControlPin(vinPin)) { pinMode(vinPin, OUTPUT); digitalWrite(vinPin, LOW); }
  if (isValidControlPin(drenPin)) { pinMode(drenPin, OUTPUT); digitalWrite(drenPin, LOW); }
  if (isValidControlPin(pumpPin)) { pinMode(pumpPin, OUTPUT); digitalWrite(pumpPin, LOW); }

  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      short pin = getIntFromJson(zone, "pin", -1);
      if (isValidControlPin(pin)) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
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

  uint16_t lastVals = 0;
  if (wCtrPin >= 0 && wCtrPin < PINS_COUNT) 
      {      
        pinMode(wCtrPin, INPUT);
        lastVals |= (getPinVal(wCtrPin) ? LASTWCTRLSTATE : 0);
        lastVals |= (getPinVal(wCtrPin) ? LASTWCTRLSTATE_ALL : 0);
      }
  if (wMaxPin >= 0 && wMaxPin < PINS_COUNT) {
    pinMode(wMaxPin, INPUT);
    lastVals |= (publishBooleanState("$wMax", getPinVal(wMaxPin)) ? LASTWMAXSTATE : 0);
  }
  if (wMinPin >= 0 && wMinPin < PINS_COUNT) {
    pinMode(wMinPin, INPUT);
    lastVals |= (publishBooleanState("$wMin", getPinVal(wMinPin)) ? LASTWMINSTATE : 0);
  }
  if (fbDrenPin >= 0 && fbDrenPin < PINS_COUNT) {
    pinMode(fbDrenPin, INPUT);
    lastVals |= (publishBooleanState("$fbDren", getPinVal(fbDrenPin)) ? LASTFBDRENSTATE : 0);
  }
  if (fbPumpPin >= 0 && fbPumpPin < PINS_COUNT) {
    pinMode(fbPumpPin, INPUT);
    lastVals |= (publishBooleanState("$fbPump", getPinVal(fbPumpPin)) ? LASTFBPUMPSTATE : 0);
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
  setOutput(vinPin, false);
  setOutput(drenPin, false);
  setOutput(pumpPin, false);
  setStatus(CST_UNKNOWN);
  return 1;
}

/*
int out_sprinkler::Status()
{
  if (!item || !gatesObj) return 0;

  bool wMax   = (wMaxPin >= 0) ? getPinVal(wMaxPin) : false;
  bool wMin   = (wMinPin >= 0) ? getPinVal(wMinPin) : false;
  bool fbDren = (fbDrenPin >= 0) ? getPinVal(fbDrenPin) : false;
  bool fbPump = (fbPumpPin >= 0) ? getPinVal(fbPumpPin) : false;

  publishBooleanStateIfChanged("$wMax", wMax, lastWMaxState);
  publishBooleanStateIfChanged("$wMin", wMin, lastWMinState);
  publishBooleanStateIfChanged("$fbDren", fbDren, lastFbDrenState);
  publishBooleanStateIfChanged("$fbPump", fbPump, lastFbPumpState);

  int state = getIntFromJson(gatesObj, "@state", SP_UNKNOWN);
  publishNumericState("$state", state);

  return 1;
}
*/

bool out_sprinkler::isFreeze()
{
  if (!item) return false;
  return ((item->getFlag(FLAG_FREEZED) & FLAG_FREEZED) == FLAG_FREEZED);
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
        if (valVal < setVal) return true;
      }
    }
    zone = zone->next;
  }
  return false;
}

void out_sprinkler::pump(bool state)
{
  if (!isValidControlPin(pumpPin)) return;
  setOutput(pumpPin, state);
  publishBooleanState("$rPump", state);
}

void out_sprinkler::turnOffAllZones()
{
  if (!gatesObj) return;
  aJsonObject * zone = gatesObj->child;
  while (zone)
  {
    if (zone->name && *zone->name && zone->type == aJson_Object)
    {
      short pin = getIntFromJson(zone, "pin", -1);
      setOutput(pin, false);
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

void out_sprinkler::notifyState(short state)
{
  if (!gatesObj) return;
  setValToJson(gatesObj, "@state", (long)state);
  publishNumericState("$state", state);
}

int out_sprinkler::shutdown(sprinklerState nextState)
{
  if (!gatesObj) return 0;

  switch (nextState)
  {
    case SP_OFF:
    case SP_FULL:
      setOutput(drenPin, false);
      setOutput(vinPin, false);
      break;
    case SP_DREN_ON:
    case SP_DREN_OPERATE:
      setOutput(drenPin, true);
      setOutput(vinPin, false);
      break;
    case SP_VIN:
      setOutput(vinPin, true);
      setOutput(drenPin, false);
      break;
    case SP_DREN_EMPTY:
      setOutput(drenPin, false);
      setOutput(vinPin, false);
      break;
    case SP_FAULT_VIN:
      setOutput(vinPin, false);
      break;
    case SP_FAULT_DREN:
      setOutput(drenPin, false);
      break;
    case SP_UNKNOWN:
      setOutput(drenPin, false);
      setOutput(vinPin, false);
      break;
  }

  publishBooleanState("$rDren", nextState == SP_DREN_ON || nextState == SP_DREN_OPERATE);
  publishBooleanState("$vIN", nextState == SP_VIN);
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
      if (cmd == CMD_ON && valVal < setVal) return zone;
    }
    zone = zone->next;
  }
  return NULL;
}

void out_sprinkler::setZoneActive(aJsonObject * zone, bool active)
{
  if (!zone) return;
  setValToJson(zone, "@active", (long)(active ? 1 : 0));

  char subItem[48];
  snprintf(subItem, sizeof(subItem), "%s/$state", zone->name);
  item->SendStatusImmediate(itemCmd().Cmd(active ? CMD_ON : CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, subItem);
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
  setValToJson(gatesObj, "wCtr", current);
  item->SendStatusImmediate(itemCmd().Int(current).setSuffix(S_SET), FLAG_PARAMETERS);
}


bool out_sprinkler::publishBooleanState(const char * subItem, bool state)
{
  if (!item) return state;
  item->SendStatusImmediate(itemCmd().Cmd(state ? CMD_ON : CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, (char *)subItem);
  return state;
}

void out_sprinkler::publishNumericState(const char * subItem, long value)
{
  if (!item) return;
  item->SendStatusImmediate(itemCmd().Int(value).setSuffix(S_SET), FLAG_PARAMETERS, (char *)subItem);
}

int out_sprinkler::Poll(short cause)
{
  if (!item || !gatesObj) return 0;


  bool freeze = isFreeze();
  bool wMax   = (wMaxPin >= 0) ? getPinVal(wMaxPin) : false;
  bool wMin   = (wMinPin >= 0) ? getPinVal(wMinPin) : false;
  bool fbDren = (fbDrenPin >= 0) ? getPinVal(fbDrenPin) : false;
  bool fbPump = (fbPumpPin >= 0) ? getPinVal(fbPumpPin) : false;

  uint32_t lastVals = getIntFromJson(gatesObj, "@lastVals", 0);

  publishBooleanStateIfChanged("$wMax", wMax, LASTWMAXSTATE, lastVals);
  publishBooleanStateIfChanged("$wMin", wMin, LASTWMINSTATE, lastVals);
  publishBooleanStateIfChanged("$fbDren", fbDren, LASTFBDRENSTATE, lastVals);
  publishBooleanStateIfChanged("$fbPump", fbPump, LASTFBPUMPSTATE, lastVals);

  uint32_t now = millisNZ();
  int state = getIntFromJson(gatesObj, "@state", SP_UNKNOWN);
  uint32_t timer = (uint32_t)getIntFromJson(gatesObj, "@timer", 0);
  bool lastWctrlState = lastVals & LASTWCTRLSTATE;
  bool lastWctrlStateAll = lastVals & LASTWCTRLSTATE_ALL;

  if (wCtrPin >= 0)
      {
        bool curr = getPinVal(wCtrPin);
        if (curr && !lastWctrlStateAll)
        {
          updateCounterValue();         
        }
        if (curr) lastVals |= LASTWCTRLSTATE_ALL; else lastVals &= ~LASTWCTRLSTATE_ALL;
      }
  setValToJson(gatesObj, "@lastVals", (long)lastVals);

  if (freeze)
  {
    shutdown(SP_OFF);
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
        shutdown(SP_DREN_ON);
      }
      break;

    case SP_DREN_ON:
      if (fbDren)
      {
        state = SP_DREN_OPERATE;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_DREN_OPERATE);
      }
      else if (isTimeOver(timer, now, DRENAGE_ON_TIME))
      {
        state = SP_DREN_EMPTY;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_DREN_EMPTY);
      }
      break;

    case SP_DREN_OPERATE:
      if (!fbDren)
      {
        state = SP_DREN_EMPTY;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_DREN_EMPTY);
      }
      else if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FULL);
      }
      else if (isTimeOver(timer, now, 1200000UL))
      {
        state = SP_FAULT_DREN;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FAULT_DREN);
      }
      break;

    case SP_DREN_EMPTY:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FULL);
      }
      else if (item->getCmd() == CMD_ON)
      {
        state = SP_VIN;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_VIN);
      }
      break;

    case SP_VIN:
      if (fbDren)
      {
        state = SP_DREN_OPERATE;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_DREN_OPERATE);
      }
      else if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FULL);
      }
      else if (isTimeOver(timer, now, VIN_MAX_TIME))
      {
        state = SP_FAULT_VIN;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FAULT_VIN);
      }
      break;

    case SP_FULL:
      if (!wMax)
      {
        state = SP_DREN_ON;
        setValToJson(gatesObj, "@timer", (long)now);
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_DREN_ON);
      }
      break;

    case SP_FAULT_VIN:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FULL);
      }
      break;

    case SP_FAULT_DREN:
      if (wMax)
      {
        state = SP_FULL;
        setValToJson(gatesObj, "@state", (long)state);
        shutdown(SP_FULL);
      }
      break;
  }

  bool tankReady = (state == SP_FULL || wMax);
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
        short zonePin = getIntFromJson(currentZone, "pin", -1);
        setOutput(zonePin, true);
        setValToJson(gatesObj, "@flowTimer", (long)now);
      }

      if (wCtrPin >= 0)
      {
        bool curr = getPinVal(wCtrPin);
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
        setOutput(getIntFromJson(currentZone, "pin", -1), false);
        setZoneActive(currentZone, false);
        //////setValToJson(currentZone, "cmd", (long)CMD_OFF);
        item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, currentZone->name);
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

  if (subItem && *subItem)
  {
    aJsonObject * zone = getZone(subItem);
    if (!zone) return 0;

    switch (suffixCode)
    {
      case S_SET:
        if (toExecute)
        {
          long value = cmd.getInt();
          setValToJson(zone, "set", value);
          item->SendStatusImmediate(itemCmd().Int(value).setSuffix(S_SET), FLAG_PARAMETERS, subItem);
        }
        return 1;

      case S_VAL:
        if (toExecute)
        {
          long value = cmd.getInt();
          setValToJson(zone, "val", value);
          item->SendStatusImmediate(itemCmd().Int(value).setSuffix(S_VAL), FLAG_PARAMETERS, subItem);
        }
        return 1;

      case S_CMD:
      default:
        switch (cmd.getCmd())
        {
          case CMD_ON:
            if (toExecute)
            {
              setValToJson(zone, "cmd", (long)CMD_ON);
              item->SendStatusImmediate(itemCmd().Cmd(CMD_ON).setSuffix(S_CMD), FLAG_COMMAND, subItem);
            }
            return 1;

          case CMD_OFF:
            if (toExecute)
            {
              setValToJson(zone, "cmd", (long)CMD_OFF);
              setZoneActive(zone, false);
              setOutput(getIntFromJson(zone, "pin", -1), false);
              item->SendStatusImmediate(itemCmd().Cmd(CMD_OFF).setSuffix(S_CMD), FLAG_COMMAND, subItem);
            }
            return 1;

          case CMD_RESET:
            if (toExecute)
            {
              setValToJson(zone, "val", (long)0);
              item->SendStatusImmediate(itemCmd().Int(0).setSuffix(S_VAL), FLAG_PARAMETERS, subItem);
            }
            return 1;

          default:
            return 0;
        }
    }
  }

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
          return 1;

        case CMD_RESET:
          {
            aJsonObject * zone = gatesObj->child;
            while (zone)
            {
              if (zone->name && *zone->name && zone->type == aJson_Object)
              {
                setValToJson(zone, "val", (long)0);
                item->SendStatusImmediate(itemCmd().Int(0).setSuffix(S_VAL), FLAG_PARAMETERS, zone->name);
              }
              zone = zone->next;
            }
          }
          return 1;

        default:
          break;
      }
      break;

    case S_SET:
      if (toExecute)
      {
        long value = cmd.getInt();
        setValToJson(gatesObj, "@wCtr", value);
      }
      return 1;

    case S_VAL:
      if (toExecute)
      {
        long value = cmd.getInt();
        if (value < 0)
        {
          item->setFlag(FLAG_FREEZED);
          item->SendStatus(FLAG_FLAGS);
        }
        else if (isFreeze())
        {
          item->clearFlag(FLAG_FREEZED);
          item->SendStatus(FLAG_FLAGS);
        }
      }
      return 1;

    default:
      break;
  }
  return 0;
}

int out_sprinkler::getChanType()
{
  return CH_RELAY;
}

#endif // SPRINKLER_ENABLE