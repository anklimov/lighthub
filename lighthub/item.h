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
/**
 * @file item.h
 * @brief Item (virtual channel) system of LightHub.
 *
 * LightHub models every controllable device, sensor or service as an "item"
 * (virtual channel) stored as a JSON array inside the global #items object:
 * `[type, arg, val, cmd, ext, timestamp]` (element indexes I_TYPE..I_TIMESTAMP).
 * The item name is the key of the array; the type (CH_*) selects the driver
 * module (DMX dimmer, relay, Modbus, thermostat, AC, ...) that handles it.
 *
 * An item can be a single channel (CH_DIMMER, CH_RELAY, ...) or a CH_GROUP
 * pseudo-channel which fans commands out recursively to its members.
 * Commands and values travel to/from items as #itemCmd structures and are
 * published to MQTT topics with the suffixes of suffix_P
 * (cmd/set/val/del/HSV/RGB/fan/mode/ctrl/hue/sat/temp/raw).
 */
#pragma once
#include "options.h"
#include "abstractout.h"
#include "itemCmd.h"


/**
 * Suffix (subtopic) codes - indexes of the suffix_P table below.
 * S_NOTFOUND means "no suffix given" (universal/empty OpenHAB topic).
 */
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

/**
 * MQTT topic suffixes (subtopics) used per item:
 * - ""       - base item topic
 * - "cmd"    - current command (ON/OFF/...)
 * - "set"    - setpoint / target value
 * - "val"    - actual measured value
 * - "del"    - delayed (timed) command status, remaining time
 * - "HSV"    - HSV color notation for color channels
 * - "RGB"    - RGB color notation for color channels
 * - "fan"    - fan speed subchannel (AC/vent)
 * - "mode"   - device mode (AC modes)
 * - "ctrl"   - control flags (ENABLE/DISABLE/FREEZE)
 * - "hue"    - hue component only
 * - "sat"    - saturation component only
 * - "temp"   - color temperature component only
 * - "raw"    - raw (unparsed) payload passthrough
 */
typedef  char suffixstr[5];
const suffixstr suffix_P[] PROGMEM =
{"","cmd","set","val","del","HSV","RGB","fan","mode","ctrl","hue","sat","temp","raw"};
#define suffixNum sizeof(suffix_P)/sizeof(suffixstr)

#define S_ADDITIONAL 13

/**
 * Channel (item) types, indexes CH_DIMMER(0)..CH_SPRINKLER(23)=CH_MAX.
 * The type is stored in the I_TYPE element of the item array and selects
 * the output driver module that handles the item.
 *
 * - CH_DIMMER = 0 - DMX dimmer, 1-4 channels
 * - CH_RGBW = 1 - DMX 4-channel RGB+W
 * - CH_RGB = 2 - DMX 3-channel RGB
 * - CH_PWM = 3 - PWM output directly to PIN, 1-4 CH
 * - CH_MODBUS = 4 - Modbus AC dimmer
 * - CH_THERMO = 5 - simple ON/OFF thermostat
 * - CH_RELAY = 6 - ON/OFF relay output
 * - CH_GROUP = 7 - group pseudo-channel (fan-out to members)
 * - CH_VCTEMP = 8 - Vacom PID temperature regulator
 * - CH_VC = 9 - Vacom Modbus fan (motor) regulator
 * - CH_AC = 10 - Haier AC
 * - CH_SPILED = 11 - SPI LED strip
 * - CH_MOTOR = 12 - motorized gate with feedback resistor
 * - CH_PID = 13 - generic PID regulator
 * - CH_MBUS = 14 - universal Modbus master channel
 * - CH_UARTBRIDGE = 15 - bridge between 2 UARTs (PDU reporting via UDP)
 * - CH_RELAYX = 16 - slow PWM relay for objects with inertia
 * - CH_RGBWW = 17 - DMX RGBWW (warm and cold white)
 * - CH_MULTIVENT = 18 - multichannel ventilation
 * - CH_ELEVATOR = 19 - elevator controller
 * - CH_COUNTER = 20 - generic counter
 * - CH_HUMIDIFIER = 21 - humidifier
 * - CH_MERCURY = 22 - Mercury energy meter / RS485 interface
 * - CH_SPRINKLER = 23 - sprinkler controller
 */
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
#define CH_SPRINKLER 23
#define CH_MAX 23

/**
 * Polling rates/causes passed to Item::Poll() and driver Poll():
 * - POLLING_SLOW - periodic slow poll cycle
 * - POLLING_FAST - fast poll loop
 * - POLLING_INT  - called from an interrupt context (ISR-safe actions only)
 * - POLLING_1S   - once per second tick
 */
#define POLLING_SLOW 1
#define POLLING_FAST 2
#define POLLING_INT  3
#define POLLING_1S   4

//CTRL Execution flags
/**
 * Group execution flags passed to Item::Ctrl(itemCmd, ...):
 * - CTRL_DISABLE_RECURSION - do not recurse into group members
 * - CTRL_DISABLE_NON_GRP   - skip non-group items
 * - CTRL_SCHEDULED_CALL_RECURSION - both; used when a scheduled
 *   (delayed) group command fires back
 */
#define CTRL_DISABLE_RECURSION 1
#define CTRL_DISABLE_NON_GRP  2
#define CTRL_SCHEDULED_CALL_RECURSION (CTRL_DISABLE_RECURSION | CTRL_DISABLE_NON_GRP)

/**
 * Element indexes of the item JSON array:
 * - I_TYPE - channel type (CH_*)
 * - I_ARG - channel-type depended argument (pin, address or argument array)
 * - I_VAL - latest preset value (int, float or array of presets)
 * - I_CMD - latest command received (CMD_* packed with FLAG_* bits)
 * - I_EXT - channel-depended extension data
 * - I_TIMESTAMP - armed delayed command (ms timestamp, subtype = CMD_*)
 */
#define I_TYPE 0 //Type of item
#define I_ARG  1 //Chanel-type depended argument or array of arguments (pin, address etc)
#define I_VAL  2 //Latest preset (int or array of presets)
#define I_CMD  3 //Latest CMD received
#define I_EXT  4 //Chanell-depended extension - array
#define I_TIMESTAMP 5

/**
 * Argument array indexes of a CH_MODBUS item:
 * I_ARG = [addr, reg, mask, maxScale, regType].
 */
#define MODBUS_CMD_ARG_ADDR 0
#define MODBUS_CMD_ARG_REG 1
#define MODBUS_CMD_ARG_MASK 2
#define MODBUS_CMD_ARG_MAX_SCALE 3
#define MODBUS_CMD_ARG_REG_TYPE 4

/** Modbus register types: 0 - coils, 1 - discrete inputs, 2 - holding, 3 - input. */
#define MODBUS_COIL_REG_TYPE 0
#define MODBUS_DISCRETE_REG_TYPE 1
#define MODBUS_HOLDING_REG_TYPE 2
#define MODBUS_INPUT_REG_TYPE 3

#include "aJSON.h"

/** Global JSON object with all items (virtual channels) of this device. */
extern aJsonObject *items;
/** Store the current measured temperature of a CH_THERMO item by name. */
extern short thermoSetCurTemp(char *name, float t);

/**
 * Translate a text payload to a command code.
 * @param payload - text command word ("ON", "HALT", ...), number or color
 * @return CMD_* code; CMD_VOID for plain numbers, CMD_UNKNOWN if unrecognized
 */
int txt2cmd (char * payload);
/**
 * Translate a subtopic text to a suffix code (index of suffix_P).
 * @param payload - suffix text ("cmd", "set", "val", ...)
 * @return suffix code (S_*) or S_NOTFOUND
 */
int txt2subItem(char *payload);

/**
 * @brief A single LightHub item (virtual channel).
 *
 * Item wraps one entry of the global #items JSON object: a virtual channel
 * of a given type (CH_*) with its arguments, current value, last command,
 * optional extension data and a driver instance. Items are the core
 * abstraction of the system: physical outputs (relays, DMX, Modbus, AC),
 * groups and services are all items, addressed by name ("name",
 * "name/subItem" or "name/subItem/suffix").
 *
 * All external control is funneled through the Ctrl() overloads; the
 * resulting state is published to MQTT (and CAN when enabled) via
 * SendStatus()/SendStatusImmediate().
 */
class Item
{
  public:
  /// Pointers into the items JSON tree (root object, item array, its arg/val/ext elements)
  aJsonObject *rootItems, *itemArr, *itemArg,*itemVal,*itemExt;
  /// Channel type (CH_*) resolved from I_TYPE
  uint8_t itemType;
  /// Linked output driver instance (shared per channel type) or NULL
  abstractOut * driver;

  /**
   * @brief Create an item by name from the items object.
   * @param name - item name, optionally "item/subItem" or "item/subItem/suffix";
   *        the tail sets the default subItem/suffix used by later Ctrl() calls
   * @param _items - root items object (global #items by default)
   */
  Item(char * name, aJsonObject *_items = items);
  /**
   * @brief Adopt an existing items-array element as this item.
   * @param obj - pointer to the item array inside the items object
   */
  Item(aJsonObject * obj, aJsonObject *_items = items);
  /**
   * @brief Create an item by CAN id (CANDRV builds).
   * @param num - CAN identifier of the item
   * @param subItem - default subItem id, optionally OR'ed with SUBITEM_IS_COMMAND
   */
  Item(uint16_t num, uint8_t subItem, aJsonObject *_items = items);
  /** Destructor: releases the driver if it is not shared with another item. */
  ~Item();

  /** @return true if the item was found in config and is a valid array. */
  boolean isValid ();
  /**
   * Initialize the driver hardware.
   * @return true on success (driver present and started)
   */
  boolean Setup();
  /** Stop the driver and release its hardware resources. */
  void Stop();
  //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL);
  /**
   * @brief Main control entry point: execute a command/value on this item.
   * @param cmd - command and/or value (see #itemCmd)
   * @param subItem - subchannel name or NULL (default subItem is used)
   * @param flags - CTRL_* group execution flags
   * @param authorized - true if the caller is authorized (unlocks DISABLE, etc.)
   * @return 1 - executed, 3 - no action needed (target state reached),
   *         -1 - error, -4 - invalid argument, -5 - unauthorized, -6 - disabled
   */
  int Ctrl(itemCmd cmd, char* subItem=NULL, uint8_t flags = 0, bool authorized=false);
  /**
   * Parse a text payload (command word, number, color, optional
   * "token@command" authorization prefix) and route it to Ctrl(itemCmd, ...).
   * @param remoteID - if non-zero, forward the command to that remote node via CAN
   */
  int Ctrl(char * payload,  char * subItem=NULL, int remoteID = 0);
  /**
   * Forward a command to a remote LightHub node over CAN.
   * @param cmd - command to send, @param remoteID - target node id
   * @param authToken - optional authorization token
   */
  int remoteCtrl(itemCmd cmd, int remoteID, char* subItem=NULL, char * authToken=NULL);
  /** @return n-th argument (I_ARG) as int; 0 if absent. */
  int getArg(short n=0);
  /** @return n-th argument (I_ARG) as float; 0.0 if absent. */
  float getFloatArg(short n=0);
  /** @return number of arguments in I_ARG (0, 1 or array size). */
  short getArgCount();
  //int getVal(short n); //From VAL array. Negative if no array
  /** @return latest stored value (I_VAL) as int, from a scalar or first array element. */
  long int getVal(); //From int val OR array
  /** @return storage subtype (ST_*) of the stored value. */
  uint8_t getSubtype();
  /** @return last command code (CMD_*) stored in I_CMD, or -1. */
  uint8_t getCmd();
  /** @return channel extension data (I_EXT) as int. */
  long int getExt(); //From int val OR array
  /** Store an integer extension value in I_EXT. */
  void setExt(long int par);
  /** @return pointer to driver-persistent data attached to I_EXT, or NULL. */
  chPersistent * getPersistent();
  /** Attach driver-persistent data to I_EXT. @return the stored pointer. */
  chPersistent * setPersistent(chPersistent * par);
  /** Store a command code in I_CMD, preserving the FLAG_* bits. */
  void setCmd(uint8_t cmdValue);
  /** @return subset of stored FLAG_* bits matching `flag` (default: all). */
  uint32_t getFlag   (uint32_t flag=FLAG_MASK);
  /** Set FLAG_* bits in I_CMD, preserving the CMD_* bits. */
  void setFlag   (uint32_t flag);
  /** Clear FLAG_* bits in I_CMD, preserving the CMD_* bits. */
  void clearFlag (uint32_t flag);
  /** Store an integer value in I_VAL (only if I_VAL is a scalar). */
  void setVal(long int par);
  /** Store a float value in I_VAL (only if I_VAL is a scalar). */
  void setFloatVal(float par);
  /** Store the storage subtype (ST_*) of I_VAL. */
  void setSubtype(uint8_t par);
  /**
   * Periodic poll of the item.
   * Fires armed delayed (S_DELAYED) commands, retries failed transactions
   * and delegates to the driver.
   * @param cause - POLLING_SLOW/POLLING_FAST/POLLING_INT/POLLING_1S
   */
  int Poll(int cause);
  /**
   * Publish current command/value/flags to MQTT.
   * @param sendFlags - FLAG_COMMAND/FLAG_PARAMETERS/FLAG_FLAGS and
   *        FLAG_SEND_IMMEDIATE/FLAG_SEND_DEFFERED options
   * Defers the publish (sets a pending flag) if the network is busy or RAM is low.
   */
  int SendStatus(long sendFlags, char * subItem=NULL);
  /** Publish the given status to MQTT immediately without deferral. */
  int SendStatusImmediate(itemCmd st, long sendFlags, char * subItem=NULL, bool tetain = true);
  /** @return 1 if the channel is active (ON), 0 if OFF, -1 if invalid. */
  int isActive();
  /** @return effective channel type (CH_*) - from the driver if present, else I_TYPE. */
  int getChanType();
  /** Turn the channel ON. */
  inline int On (){return Ctrl(itemCmd(ST_VOID,CMD_ON));};
  /** Turn the channel OFF. */
  inline int Off(){return Ctrl(itemCmd(ST_VOID,CMD_OFF));};
  /** Toggle the channel ON/OFF. */
  inline int Toggle(){return Ctrl(itemCmd(ST_VOID,CMD_TOGGLE));};
  /**
   * Arm a delayed command: execute `cmd`'s command code after the delay
   * stored in cmd parameter (ms). @param authorized - stored with the timer.
   */
  int scheduleCommand(itemCmd cmd, bool authorized);
  /**
   * Arm the opposite command after the given delay (momentary action):
   * e.g. CMD_ON -> CMD_OFF after N ms, CMD_XON -> CMD_XOFF, etc.
   * @param isActiveNow - current activity state of the channel
   */
  int scheduleOppositeCommand(itemCmd cmd,short isActiveNow,bool authorized);
  /** @return the scheduled command code (CMD_*), 0 if nothing is armed. */
  int isScheduled();
  /** @return name of the subItem with the given id, or NULL. */
  char * getSubItemStrById(uint8_t subItem);
  /** @return id of the subItem with the given name, or NO_SUBITEM. */
  uint8_t getSubitemId(char * subItem);

  protected:
  /**
   * Recursively walk a group's member list.
   * If cmd is given - execute it on every member (non-recursively);
   * otherwise check members' activity and return true if any is active.
   */
  bool digGroup (aJsonObject *itemArr, itemCmd *cmd = NULL, char* subItem = NULL, bool authorized = false, uint8_t ctrlFlags = 0);
  /** @return maximum settable value for this channel type (255, 80 for thermostat, 0 for counter). */
  long int limitSetValue();
  /** (Legacy) Set Vacom fan speed over Modbus. */
  int VacomSetFan (itemCmd st);
  /** (Legacy) Set Vacom heat temperature over Modbus. */
  int VacomSetHeat(itemCmd st);
  /** (Legacy) Write dimmer value to the Modbus register of this item. */
  int modbusDimmerSet(itemCmd st);
  /** (Legacy) Low-level Modbus write of `value` to register `_reg` of slave `addr`. */
  int modbusDimmerSet(int addr, uint16_t _reg, int _regType, int _mask, uint16_t value);
  /** Mark the last Modbus transaction as failed (sets FLAG_SEND_RETRY). */
  void mb_fail(int result=0);
  /** Parse the item array: resolve type, arg/val/cmd/ext elements and the driver. */
  void Parse();
  /** (Legacy) Poll the Modbus dimmer register and update stored value/status. */
  int checkModbusDimmer();
  /** (Legacy) Compare a polled Modbus register `data` with the stored value and resync status. */
  int checkModbusDimmer(int data);
  /** Retry the last failed transaction if FLAG_SEND_RETRY is set. */
  int checkRetry();
  /** Publish a pending (deferred) status if the network is up. */
  void sendDelayedStatus();
  /** (Legacy) Poll Vacom fan motor registers and publish the state as JSON. */
  int checkFM();
  char defaultSubItem[16];
  int  defaultSuffixCode;
};

/**
 * @brief Creates and shares output drivers for items.
 *
 * One driver instance is kept per channel type; getDriver() links the
 * driver to the current item (abstractOut::link) and creates it on demand.
 * Items of the same type share the same driver, saving RAM.
 */
class driverFactory {
    public:
    driverFactory(){memset(drivers,0,sizeof(drivers));};
    /** @return item currently linked to the driver of this item, or NULL. */
    Item * getItem(Item * item);
    /** @return driver for the item's channel type (created on demand), linked to the item. */
    abstractOut * getDriver(Item * item);
    /** @return driver for the item's channel type without creating it (RGB types map to CH_DIMMER). */
    abstractOut * findDriver(Item * item);
    /** Unlink the item from its driver; delete the driver if no item uses it. */
    void freeDriver(Item * item);
    /** @return a new driver instance for the given channel type, or NULL if unsupported. */
    abstractOut * newDriver(uint8_t itemType);
    private:
    /// Shared driver instance per channel type (CH_*)
    abstractOut * drivers[CH_MAX+1];
};

/**
 * @brief CH_THERMO runtime state packed into one int32 (stored in I_EXT).
 *
 * tempX100 - last measured temperature x 100 (low word),
 * timestamp16 - millisNZ() timestamp of the measurement (high word).
 */
typedef union
{
struct
  {
  int16_t  tempX100;
  uint16_t timestamp16;
  };
int32_t  asint;   
} thermostatStore;