#pragma once
#include "options.h"
#ifndef MBUS_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <utils.h>



/**
 * @brief Persistent per-item state of a Modbus channel, stored in the config tree.
 *
 * Holds the line parameters, the polling schedule/interval and the template
 * references: polled register lists (holding regs, input regs, coils,
 * discrete inputs) and the template parameter table.
 */
class mbPersistent : public chPersistent  {

public:

  int baud;
  serialParamType serialParam;
  uint16_t pollingInterval;
  uint32_t timestamp;
  aJsonObject * pollingRegisters;
  aJsonObject * pollingIrs;
  aJsonObject * pollingCoils;
  aJsonObject * poolingDiscreteIns;
  aJsonObject * parameters;
};

//execObj subtype
#define MB_NEED_SEND  8
#define MB_SEND_ERROR 4
#define MB_SEND_ATTEMPTS 3

//lastMeasusement subtype
#define LM_VALUE_OUTDATED 1
#define LM_VALUE_EMPTY 2


/**
 * @brief Generic Modbus register channel driver, channel type CH_MBUS = 14.
 *
 * The channel is described by a template from the global Modbus template
 * table: registers are polled periodically, mapped to item parameters and
 * fired as execution actions, and control commands are written back to the
 * registers. Last sent / last measured values are tracked per parameter in
 * the "@V" / "@S" marks to suppress redundant writes and re-executions.
 *
 * JSON config (itemArg): [addr, "template", {params}]
 *  - addr: Modbus unit address;
 *  - "template": name of a template in the global modbus settings
 *    (keys: serial, baud, poll{regs, irs, coils, dins, delay}, par);
 *  - {params}: per-parameter objects; a template parameter supports
 *    reg/ir/coil/din (address), type, id (suffix code), map, def, prefetch.
 *
 * Register value types: i16, i32, u16, u32, i8h, i8l, u8h, u8l, x10, 100.
 *
 * Ctrl: the subitem (template parameter name) takes priority, otherwise the
 * suffix code is matched against the "id" of the template parameters
 * (e.g. S_CMD / S_SET or a custom numeric id).
 */
class out_Modbus : public abstractOut {
public:
    //out_Modbus(Item * _item):abstractOut(_item){store = (mbPersistent *) item->getPersistent();};
    /** @brief Default constructor; binds nothing until link() is called. */
    out_Modbus():store(NULL){};
    /** @brief Links the channel item and attaches persistent storage. */
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (mbPersistent *) item->getPersistent(); } else store = NULL;};

    /** @brief Creates persistent storage, loads the template config and prepares the @S marks. */
    int Setup() override;
    /** @brief Sends queued (MB_NEED_SEND) parameter values, then polls the configured register lists and executes mapped actions. */
    int Poll(short cause) override;
    /** @brief Releases the persistent storage. */
    int Stop() override;
    /** @brief Returns CH_MBUS (14). */
    int getChanType() override;
    /** @brief Queues a command for a template parameter (by subitem name or by suffix id), see class doc. */
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    //int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    mbPersistent * store;
    bool getConfig();
    itemCmd  findRegister(uint16_t registerNum, uint16_t posInBuffer, uint8_t regType, uint16_t registerFrom, uint16_t registerTo, bool doExecution = true, bool * submitParam = NULL);
    void pollModbus(aJsonObject * reg, int regType);
    void initLine();
    int  sendModbus(char * paramName, aJsonObject * outValue);
    int  sendItemCmd(aJsonObject *templateParamObj, itemCmd cmd);
    int  createLastMeasured(char * name);
    int  createLastMeasured(aJsonObject * execObj);
    aJsonObject * getLastMeasured(char * name);
    aJsonObject * getLastMeasured(aJsonObject * execObj);
    void setLastMeasured(aJsonObject * execObj, int val);
};
#endif
