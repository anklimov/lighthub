#pragma once
#include "options.h"
#ifndef MULTIVENT_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"
#include <PID_v1.h>

/**
 * @brief Boost parameters of the shared AC node.
 *
 * When the balance of the zone PID outputs exceeds AC_BOOST_TRESHOLD (230),
 * the AC setpoint is boosted to AC_BOOST_HIGH_TEMP (30) for heating or
 * AC_BOOST_LOW_TEMP (17) for cooling, and restored to AC_PRESET_TEMP (21)
 * when the balance falls back within AC_BOOST_DEADBAND (10).
 * AC_HEATCOOL_DEADBAND is the temperature deadband of HEATCOOL zones.
 */
#define AC_BOOST_HIGH_TEMP 30
#define AC_PRESET_TEMP 21
#define AC_BOOST_LOW_TEMP 17
#define AC_BOOST_TRESHOLD 230
#define AC_BOOST_DEADBAND 10
#define AC_HEATCOOL_DEADBAND 1.0

/** @brief Bitmask flags (zone "supress") suppressing redundant commands to the AC. */
#define AC_SUPPRESS_FAN 1
#define AC_SUPPRESS_SET 2
#define AC_SUPPRESS_CMD 4
#define AC_SUPPRESS_ALL 7


/**
 * @brief Multi-zone ventilation controller for a shared AC unit,
 * channel type CH_MULTIVENT = 18.
 *
 * Manages several ventilation zones (per-zone setpoints, PID control and
 * airflow capacity) and steers the shared AC unit (mode, fan level and
 * setpoint), including boost on PID imbalance and suppression of redundant
 * commands to the AC.
 *
 * JSON config (itemArg = OBJECT of zones). The key "" is the shared AC node.
 * Zone keys: "set" (setpoint), "fan", "cmd", "V" (airflow capacity,
 * default 60), "pid" ([kP, kI, kD, dT(default 5 s)]), "AlarmTime"
 * (default 120 s), "AlarmValue". AC node keys: "boost" (default 230),
 * "supress" (bitmask: 1 fan, 2 set, 4 cmd, 7 all; default 0xF),
 * "turnbyfan", "overridecmd", "onextcmd", "cas" (cascade execution object).
 *
 * Ctrl commands:
 *  - /fan - LOW / MEDIUM / HIGH (64/128/255), a 0-255 value, AUTO (passive
 *    mode) or OFF;
 *  - /cmd - ON, OFF, HEAT, COOL, HEATCOOL, DRY, FAN, AUTO, FREEZE, UNFREEZE;
 *  - /set - zone temperature setpoint;
 *  - /val - zone temperature (PID input).
 *  - Without a subitem the call is AC feedback (/val, /fan, /set, /mode, /cmd).
 *
 * Poll: runs the per-zone PID loops, redistributes airflow by V capacity,
 * applies boost/suppress and sends the resulting fan level and mode to the AC.
 */
class out_Multivent : public abstractOut {
public:

    //out_Multivent(Item * _item):abstractOut(_item){getConfig();};
    //out_Multivent(){};
    /** @brief Links the channel item and reads the zones object from itemArg. */
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};    
    /** @brief Creates the per-zone state (fan/cmd/out) and allocates the PID controllers. */
    int Setup() override;
    /** @brief Per-zone PID control, airflow redistribution, boost/suppress and AC fan/mode control (see class doc). */
    int Poll(short cause) override;
    /** @brief Deletes the PID controllers and marks the channel as unknown. */
    int Stop() override;
    /** @brief Returns 1 if any zone requests an active command, 0 otherwise. */
    int isActive() override;
    /** @brief Returns CH_THERMO (5). */
    int getChanType() override;
    //int getDefaultStorageType(){return ST_PERCENTS255;};
    /** @brief Applies a command to a zone (or the AC without subitem) and forwards the resulting fan level to the AC, see class doc. */
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    /** @brief Zone fan/mode/set/val handling with passive mode, freeze and PID interlocks; returns the resulting AC fan level (0-255), -1 (no change, OK) or -2 (rejected). */
    int fanCtrl(itemCmd cmd, char* subItem=NULL, bool force = false);
protected:
    void getConfig();
    int  sendACcmd (itemCmd cmd);
    void checkACcmd (int acCmd);
    void checkACfan (int acFan);
    void checkACset (int acSet);
    void setPassiveMode(aJsonObject* zone, bool mode);
    uint32_t getFlag   (aJsonObject* zone, uint32_t flag);
    void setFlag   (aJsonObject* zone, uint32_t flag);
    void clearFlag (aJsonObject* zone, uint32_t flag);
    void enablePid(aJsonObject* zone, int enable, int direction = -1);
    bool pidEnabled(aJsonObject* pidObj);
    void setBoost(itemCmd);
    void resetBoost();
    void notifyState(itemCmd state);
    void stopAllzones();
    void restoreAllzones();


    aJsonObject * gatesObj;
    aJsonObject * acObj;
    //float acTemp;
};
#endif
