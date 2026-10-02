#pragma once
#include "options.h"
#ifdef UARTBRIDGE_ENABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include "utils.h"

#if defined(ESP32)
#define serialParamType uint32_t
#else
#define serialParamType uint16_t
#endif

#define SOURCE_PORT_A 5551
#define SOURCE_PORT_B 5552
#define MAX_PDU 64

#ifndef MODULE_UATRBRIDGE_UARTA
#define MODULE_UATRBRIDGE_UARTA Serial1
#endif

#ifndef MODULE_UATRBRIDGE_UARTB
#define MODULE_UATRBRIDGE_UARTB Serial2
#endif

#ifndef MODULE_UATRBRIDGE_UARTA_RX_PIN
#define MODULE_UATRBRIDGE_UARTA_RX_PIN 15
#endif

#ifndef MODULE_UATRBRIDGE_UARTA_TX_PIN
#define MODULE_UATRBRIDGE_UARTA_TX_PIN 2
#endif

#ifndef MODULE_UATRBRIDGE_UARTB_RX_PIN
#define MODULE_UATRBRIDGE_UARTB_RX_PIN -1
#endif

#ifndef MODULE_UATRBRIDGE_UARTB_TX_PIN
#define MODULE_UATRBRIDGE_UARTB_TX_PIN -1
#endif

/**
 * @brief Persistent state of the UART bridge driver.
 *
 * Stores the UART configuration (baud rate and serial format) restored
 * after reboot.
 */
class ubPersistent : public chPersistent  {

public:

  int baud;
  serialParamType serialParam;
  //uint16_t pollingInterval;
  //uint32_t timestamp;
  //aJsonObject * pollingRegisters;
  //aJsonObject * pollingIrs;
  //aJsonObject * parameters;
};

/** Timeout (ms) of inactivity used to split the byte stream into PDUs. */
#define PDELAY 10 

/**
 * @brief Two-way UART bridge driver (CH_UARTBRIDGE = 15).
 *
 * Transparently bridges two fixed hardware UARTs
 * (MODULE_UATRBRIDGE_UARTA / MODULE_UATRBRIDGE_UARTB, Serial1/Serial2
 * by default): every byte received on A is forwarded to B and vice
 * versa. The byte stream is split into PDUs of at most MAX_PDU (64)
 * bytes using PDELAY inactivity timeout, '*' or '\r' boundaries;
 * each PDU is reported as a UDP datagram to ip:port (default port
 * 5555, source ports 5551/5552 for directions A/B).
 *
 * JSON config (item argument object):
 * @code
 * [15, {
 *   "serial": "8N1",   // serial format, default 8N1
 *   "baud": 9600,      // baud rate, default 9600
 *   "ip": "1.2.3.4",   // optional UDP dump destination
 *   "port": 5555,      // optional UDP dump destination port
 *   "hd": 1,           // half-duplex mode (flush opposite direction)
 *   "log": 0,          // log level: 0 off, 1 hex, 2 printable
 *   "bond": "string"   // optional PDU boundary characters
 * }]
 * @endcode
 */
class out_UARTbridge : public abstractOut {
public:

   // out_UARTbridge(Item * _item):abstractOut(_item){store = (ubPersistent *) item->getPersistent();};

    /// Default constructor.
    out_UARTbridge():store(NULL){};
    /// Attach to an item and grab its persistent state.
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (ubPersistent *) item->getPersistent();} else store = NULL;};


    /// Allocate persistent state, load config and start both UARTs.
    int Setup() override;
    /// Pump bytes in both directions, split PDUs and report them via UDP.
    int Poll(short cause) override;
    /// Stop the UDP dump sockets and free the persistent state.
    int Stop() override;
    /// Channel type of the driver.
    int getChanType() override;
    /// Control commands; currently only ON/OFF are accepted (no-op).
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    //int getDefaultStorageType(){return ST_INT32;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) override;

protected:
    ubPersistent * store;
    /// Read the JSON config object (serial/baud/ip/port/hd/log/bond) and start the UARTs.
    bool getConfig();
//    int findRegister(int registerNum, int posInBuffer, int regType);
//    void pollModbus(aJsonObject * reg, int regType);
};
#endif
