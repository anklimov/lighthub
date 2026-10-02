#pragma once
#include "options.h"
#ifdef MERCURY_ENABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <utils.h>
#include <Arduino.h>



/**
 * @brief Persistent per-item state of the meter channel, stored in the config tree.
 */
class mercuryPersistent : public chPersistent  {

public:
  uint32_t timestamp;
  uint32_t lastSuccessTS;
};

/** execObj subtype markers (shared with the Modbus driver). */
#define MB_NEED_SEND  8
#define MB_SEND_ERROR 4
#define MB_SEND_ATTEMPTS 3

/**
 * @brief Session states of the meter polling state machine:
 * M_CONNECTING (login), M_CONNECTED (idle between polls) and
 * M_POLLING1..M_POLLING8 (voltage, current, angles, counters, power, cosf
 * steps of one polling cycle).
 */
#define M_CONNECTING  CST_USER+0 
#define M_CONNECTED CST_USER+1 
#define M_POLLING1 CST_USER+2
#define M_POLLING2 CST_USER+3
#define M_POLLING3 CST_USER+4
#define M_POLLING4 CST_USER+5
#define M_POLLING5 CST_USER+6
#define M_POLLING6 CST_USER+7
#define M_POLLING7 CST_USER+8
#define M_POLLING8 CST_USER+9

/** Transaction result codes returned by the meter commands. */
#define RET_SUCCESS 0
#define RET_INVALID_PARAM 1
#define RET_INTERROR 2
#define RET_ACCESS_LEVEL 3
#define RET_RTC_ERR 4
#define RET_NOT_CONNECTED 3


/**
 * @brief "Mercury" 3-phase electricity meter driver (Owen Modbus protocol),
 * channel type CH_MERCURY = 22.
 *
 * Connects to the meter over RS485, opens a session with an access level and
 * a 6-character password, and polls the measurements every
 * pollingInterval. The session is dropped and re-established after
 * 240 s without a successful transaction.
 *
 * JSON config (itemArg):
 *  [addr, baud(default 9600), "8N1", accessLevel, password,
 *   pollingInterval_ms(default 10000)]
 *  - password: a 6-character string or an array of 6 digits.
 *
 * Session states: M_CONNECTING, M_CONNECTED, M_POLLING1..M_POLLING8 (see above).
 *
 * Published status topics: /voltage/A|B|C, /current/A|B|C, /angles/A|B|C,
 * /counters/Active, /counters/Reactive, /power/A|B|C, /cosf/sum|A|B|C.
 *
 * Ctrl: not implemented (read-only channel).
 */
class out_Mercury : public abstractOut {
public:

    //out_Mercury(Item * _item):abstractOut(_item){store = (mercuryPersistent *) item->getPersistent();};
    /** @brief Default constructor; binds nothing until link() is called. */
    out_Mercury():store(NULL){};
    /** @brief Links the channel item and attaches persistent storage. */
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (mercuryPersistent *) item->getPersistent();} else store = NULL;};

    /** @brief Creates persistent storage, loads the config and initializes the RS485 line. */
    int Setup() override;
    /** @brief Runs the connect/poll state machine; on each successful step publishes the measured values (see class doc). */
    int Poll(short cause) override;
    /** @brief Closes the meter session, releases the persistent storage. */
    int Stop() override;
    /** @brief Returns CH_MBUS (14). */
    int getChanType() override;
    /** @brief Not implemented, always returns -1 (read-only channel). */
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;


protected:
    mercuryPersistent * store;
    uint32_t pollingInterval;
    bool getConfig();
    void initLine(bool full = false);
    short connectMercury();
    short disconnectMercury();
    short getCurrentVal12(byte param, String topic,int divisor=1);
    short getCurrentVal15(byte param, String topic,int divisor=1);
    short getCounters  (byte when, String topic,int divisor=1);  

};
#endif
