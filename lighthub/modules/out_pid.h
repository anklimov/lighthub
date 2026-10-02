#pragma once
#include "options.h"
#ifndef PID_DISABLE
#include <abstractout.h>
#include <item.h>
#include <PID_v1.h>
#include "itemCmd.h"
#include "aJson.h"

/** Minimum absolute change of the PID output (in output units) required
 *  to trigger an update of the controlled output. */
#define OUTPUT_TRESHOLD 1.0

/**
 * @brief Lightweight holder for PID-related JSON objects.
 *
 * Stores a pointer to aJSON objects that carry the PID item values
 * (input, setpoint, output). Currently the accessors return NULL and
 * only the embedded PID controller pointer is retrieved from the object.
 */
class pidJson {
    public:
    /// Constructor, stores the JSON object pointer.
    pidJson(aJsonObject * _obj){obj=_obj;};

    /// Get the JSON object holding the PID input value (not used yet).
    aJsonObject * getValObj(){return NULL;};
    /// Get the JSON object holding the PID setpoint (not used yet).
    aJsonObject * getSetObj(){return NULL;};
    /// Get the JSON object holding the PID output (not used yet).
    aJsonObject * getPoObj(){return NULL;};
    /// Get the PID controller pointer stored inside the JSON object value.
    PID * getPID(){return (obj) ? (PID*) obj->valueint: NULL;};
    aJsonObject * obj;
};

/**
 * @brief Persistent (survives reboot) state of the PID driver.
 *
 * Stored in the channel persistent area: the PID controller instance,
 * the current input / output / setpoint process values, the last output
 * actually applied and the input-failure (alarm) timer state.
 */
class pidPersistent : public chPersistent  {
public:
  PID * pid;
  iotype output;
  iotype input;
  iotype setpoint;
  float  prevOut; 
  uint32_t alarmTimer;
  bool alarmArmed;
  uint16_t alarmTimeout; //in sec
};


/**
 * @brief PID controller output driver (CH_PID = 13).
 *
 * Runs a classic PID loop (PID_v1) and steers an external output item
 * (e.g. a dimmer) with the computed control value.
 *
 * JSON config (item argument array):
 * @code
 * [13,
 *   [kP, kI, kD, dT, alarmTO, alarmVal, outMin, outMax],  // PID params, 1..8 elems, or 6 elems
 *   outputExecObj,   // command object applied to the controlled output
 *   cascadeExecObj]  // optional: object receiving the setpoint (cascade)
 * @endcode
 * - PID params array, trailing elements are optional: kP, kI, kD,
 *   dT (sample time, sec), alarmTO (input-failure timeout, sec,
 *   default 600 s), alarmVal (forced output value applied on alarm),
 *   outMin, outMax (output limits, default 0..255). A negative kP
 *   selects REVERSE direction.
 * - The initial setpoint is taken from the item "val" entry
 *   (default 20.0).
 *
 * Control (Ctrl) suffixes:
 * - /val  - measured input value fed to the controller (also
 *   disarms the input-failure alarm);
 * - /set  - setpoint, additionally forwarded to cascadeExecObj;
 * - /cmd  - AUTO / HEAT / COOL / ON / OFF / ENABLE / DISABLE / RESET;
 * - /ctrl - ENABLE / DISABLE (freeze the control action).
 *
 * If no /val is received for alarmTO seconds the driver enters the
 * alarm state and forces the output to alarmVal.
 */
class out_pid : public abstractOut {
public:

    //out_pid(Item * _item):abstractOut(_item){store = (pidPersistent *) item->getPersistent();};
    /// Default constructor.
    out_pid():store(NULL){};
    /// Attach to an item and grab its persistent PID state.
    void link(Item * _item){abstractOut::link(_item); if (_item) {store = (pidPersistent *) item->getPersistent();} else store = NULL;};

    /// Allocate persistent state, load JSON config, initialize the PID controller.
    int Setup() override;
    /// Run the PID loop: compute output, apply it to the output item, watch the input-failure timer.
    int Poll(short cause) override;
    /// De-initialize and free the PID controller and persistent state.
    int Stop() override;
    /// True when the item command is not CMD_OFF.
    int isActive() override;
    /// Channel type of the driver.
    int getChanType() override;
    //int getDefaultStorageType(){return ST_FLOAT;};
    /// Handle /val (input), /set (setpoint) and /cmd|/ctrl (ON/OFF/ENABLE/DISABLE/RESET) commands.
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    /// Enter (state=true) or leave (state=false) the input-failure alarm; forces the alarmVal output on entry.
    void alarm(bool);


protected:
    pidPersistent * store;
    /// Parse the item argument array, fill store and create the PID controller.
    bool getConfig();
    /// Get the forced output value used on alarm (alarmVal, or 0/255 by control direction).
    float  getAlarmVal();
};
#endif
