
#pragma once
#ifndef AC_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"

/** @brief Byte length of the AC status frame. */
#define LEN_B   37
#define B_CUR_TMP   13  //Текущая температура
#define B_CMD       17  // 00-команда 7F-ответ ???
#define B_MODE      23  //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
#define B_FAN_SPD   25  //Скорость 02 - min, 01 - mid, 00 - max, 03 - auto
#define B_SWING     27  //01 - верхний и нижний предел вкл. 00 - выкл. 02 - левый/правый вкл. 03 - оба вкл
#define B_LOCK_REM  28  //80 блокировка вкл. 00 -  выкл
#define B_POWER     29  //on/off 01 - on, 00 - off (10, 11)-Компрессор??? 09 - QUIET
#define B_FRESH     31  //fresh 00 - off, 01 - on
#define B_SET_TMP   35  //Установленная температура

/** Additional suffix codes (beyond the standard suffix_P table). */
#define S_LOCK S_ADDITIONAL+1
#define S_QUIET S_ADDITIONAL+2
#define S_SWING S_ADDITIONAL+3
//#define S_RAW S_ADDITIONAL+4

extern void modbusIdle(void) ;

/**
 * @brief Persistent per-item state of an AC channel, stored in the config tree.
 *
 * Holds the last 37-byte status frame received from the AC, decoded power
 * and mode bytes, the last frame CRC value (inCheck, used to detect
 * updates) and a timestamp of the last transmission.
 */
class acPersistent : public chPersistent  {
public:
  byte data[37];
  byte power;
  byte mode;
  byte inCheck;
  uint32_t timestamp;
};

/**
 * @brief Air conditioner driver (Haier IR-over-UART protocol), channel type CH_AC = 10.
 *
 * Talks to a Haier AC over UART at 9600 baud using 13-byte request frames
 * and 37-byte status frames (frame check = sum of bytes). Periodically
 * polls the AC unit and applies control commands.
 *
 * JSON config (itemArg): [portNum(0-3), callbacksObj]
 *  - portNum: 0 = default port (Serial3, or USB-CDC Serial), 1..3 = Serial1..Serial3;
 *  - callbacksObj (optional): object with keys "fan", "set", "cmd", "temp"
 *    holding execution objects invoked on every polled status update.
 *
 * Ctrl commands:
 *  - /set 16-40 - set temperature;
 *  - /cmd - AUTO / HEAT / COOL / FAN_ONLY / DRY, plus ON / OFF;
 *  - /fan - LOW / MEDIUM / HIGH / AUTO / OFF, or a 0-255 value;
 *  - /mode <number> - raw mode code;
  *  - subitems: "lock" ON/OFF, "swing" ON/OFF, "quiet" ON/OFF (legacy
  *    "queit" accepted for backward compatibility), "raw".
 *
 * Published status topics: /set, /temp, /mode, /cmd, /fan, /fresh, /lock,
 * /quiet, /compressor, /swing.
 */
class out_AC : public abstractOut {
public:

    /** @brief Default constructor; binds nothing until link() is called. */
    out_AC():store(NULL),portNum(0),ACSerial(NULL){};
    /** @brief Links the channel item, attaches persistent storage and reads the config. */
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (acPersistent *) item->getPersistent(); getConfig();} else store = NULL;};
    /** @brief Reads itemArg and selects the UART port (0-3) for the AC line. */
    void getConfig();
    /** @brief Initializes persistent storage and starts the UART at 9600 baud. */
    int Setup() override;
    /** @brief Polls the AC every 5 s, parses the 37-byte status frame and publishes the status topics. */
    int Poll(short cause) override;
    /** @brief Releases the persistent storage and marks the channel as unknown. */
    int Stop() override;
    /** @brief Returns 1 if the AC power bit is set, 0 otherwise. */
    int isActive() override;
    /** @brief Returns CH_THERMO (5). */
    int getChanType() override;
   // int getDefaultStorageType(){return ST_FLOAT_CELSIUS;};
    /** @brief Applies a control command: /set, /cmd, /fan, /mode, and lock/swing/quiet/raw subitems (see class doc). */
    int Ctrl(itemCmd cmd,  char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    /** @brief Invokes the named callback object ("fan"/"set"/"cmd"/"temp") from itemArg with a polled value. */
    void SubmitParameters(const char * name, itemCmd value);
    
protected:
    acPersistent * store;
    void InsertData(byte data[], size_t size);
    void SendData(byte req[], size_t size);
    uint8_t portNum;
    #if  defined (__SAM3X8E__)
    UARTClass *ACSerial;
    #else 
    HardwareSerial *ACSerial;
    #endif

};
#endif
