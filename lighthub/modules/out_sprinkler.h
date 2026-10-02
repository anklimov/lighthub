
#pragma once
#include "options.h"
#ifdef  SPRINKLER_ENABLE

#include <abstractout.h>
#include <item.h>

/** Time (ms) to wait for the drainage feedback before assuming the tank is empty. */
#define DRENAGE_ON_TIME 10000
/** Maximum time (ms) allowed to refill the tank (vin valve) before a refill fault. */
#define VIN_MAX_TIME 1200000UL
/** Maximum time (ms) allowed to drain the tank before a drainage fault. */
#define DRENAGE_MAX_TIME 4000000UL

/** Bit masks of the last published digital states, kept in "@lastVals". */
#define LASTWCTRLSTATE 1
#define LASTWCTRLSTATE_ALL 2
#define LASTWMAXSTATE 4
#define LASTWMINSTATE 8
#define LASTFBDRENSTATE 16
#define LASTFBPUMPSTATE 32
#define LASTPUMPSTATE 64
#define LASTDRENSTATE 128
#define LASTVINSTATE 256
#define LASTFAULTSTATE 512
#define LASTSTATE_MASK 0x3FF
#define LASTSTATE_INVALID 1024 

/**
 * @brief Water controller (sprinkler) state machine states.
 */
enum sprinklerState {
    SP_UNKNOWN = 0,     ///< Initial state, no information about the tank yet.
    SP_OFF = 1,         ///< System off, valves closed, waiting to start drainage.
    SP_DREN_ON = 2,     ///< Drainage valve opened, waiting for the drainage feedback.
    SP_DREN_OPERATE = 3,///< Drainage in progress, tank water level dropping.
    SP_DREN_EMPTY = 4,  ///< Tank drained (empty), waiting to refill.
    SP_VIN  = 5,        ///< Tank refill in progress (vin valve open).
    SP_FULL = 6,        ///< Tank is full, watering may proceed.
    SP_DRYING = 7,      ///< Drying mode: pump runs, no zone watering.
    SP_FAULT_VIN = -1,  ///< Fault: tank refill (vin) did not complete in time.
    SP_FAULT_DREN = -2  ///< Fault: drainage did not complete in time.
};

/**
 * @brief Water tank / sprinkler controller driver (CH_SPRINKLER = 23).
 *
 * Controls a water system: drainage (dren), refill (vin), pump and a
 * set of watering zones (valves), running a tank state machine
 * (see sprinklerState) based on the water level and feedback inputs.
 *
 * JSON config (item argument array):
 * @code
 * [23, {
 *   "": {                       // root config object
 *     "vIn": pin,               // refill valve output
 *     "rDren": pin,             // drainage valve output
 *     "rPump": pin,             // pump output
 *     "wMax": pin,              // tank "full" level input
 *     "wMin": pin,              // tank "low" level input
 *     "fbDren": pin,            // drainage (water flow) feedback input
 *     "fbPump": pin,            // pump feedback input
 *     "wCtr": pin,              // water flow counter input
 *     "onFault": execObj        // optional object executed on fault
 *   },
 *   "zone1": { "pin": pin },    // zone valve outputs (repeated per zone)
 *   "zone2": { "pin": pin }
 * }]
 * @endcode
 * Each zone automatically gets "cmd", "val", "set" and "@active"
 * members maintained by the driver.
 *
 * Control (Ctrl) commands:
 * - /cmd ON / OFF / DRY / RESET (RESET clears all zone counters);
 * - /val < 0 - sets the FREEZE flag (emergency stop, all valves close);
 * - /set - water counter target value;
 * - zone-scoped (subItem = zone name): /cmd ON/OFF/RESET, /set, /val.
 *
 * Published status (Poll):
 * - $wMax, $wMin, $fbDren, $fbPump - tank/feedback input states (ON/OFF);
 * - $rPump, $rDren, $rVIN - relay output states (ON/OFF);
 * - $state - state machine state: OFF / DREN_ON / DREN_OPERATE /
 *   DREN_EMPTY / VIN / FULL / DRYING / FAULT_VIN / FAULT_DREN / UNKNOWN;
 * - $fault - fault code: 0 = ok, 1 = vin (refill) fault, 2 = dren fault
 *   (or forwarded to the "onFault" object if configured);
 * - /<zone>/$state - per-zone valve state (ON/OFF).
 */
class out_sprinkler : public abstractOut {
public:

    //out_sprinkler(){ /*NO getConfig() here due Poll() optimization*/ };
    /// Parse the config object: root pins and the zone (gates) object.
    bool getConfig();
    /// Attach to an item and parse its configuration.
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};
    /// Initialize output/input pins and per-zone state entries.
    int Setup() override;
    /// Run the tank state machine, publish input states, water the next zone, control the pump.
    int Poll(short cause) override;
    /// Close all zones, valves and the pump, mark the driver uninitialized.
    int Stop() override;
    //int Status() override;
    /// Channel type of the driver.
    int getChanType() override;
    /// Handle controller /cmd (ON/OFF/DRY/RESET), /val (freeze) and /set, plus per-zone commands.
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

protected:
    aJsonObject * gatesObj;
    short vinPin, drenPin, pumpPin;
    short wMaxPin, wMinPin, fbDrenPin, fbPumpPin, wCtrPin;


    /// Switch the pump on/off and publish $rPump on change.
    void pump(bool state);
    /// Switch the drainage valve on/off and publish $rDren on change.
    void dren(bool state);
    /// Switch the refill (vin) valve on/off and publish $rVIN on change.
    void vin(bool state);
    /// Drive an output pin (negative pin = inverted).
    void setOutput(short pin, bool value);
    /// True when the pump is required by an active/queued zone.
    bool isNeedPump(bool steelNeed=false);
    /// Close all zone valves plus the vin and dren valves.
    void turnOffValves();
    /// Close the valves of all zones.
    void turnOffAllZones();
    /// Get a zone object by its name from the gates object.
    aJsonObject * getZone(const char * name);
    /// Find the next zone that must be watered (active, ON with val<set, or ON without set).
    aJsonObject * findNextZone();
    /// Mark a zone active/inactive and publish its /<zone>/$state.
    void setZoneActive(aJsonObject * zone, bool active);
    /// Increment a zone counter ("val") by value and publish the new value.
    void updateZoneValue(aJsonObject * zone, long value);
    /// Publish a boolean state as ON/OFF on the given subtopic.
    bool publishBooleanState(const char * subItem, bool state);
    /// Publish a numeric state on the given subtopic.
    void publishNumericState(const char * subItem, long value);
    /// Publish a boolean state only when it changed since the last publication.
    bool publishBooleanStateIfChanged(const char * subItem, bool state, uint32_t flag, uint32_t & lastState, bool forcePublish=false);
    /// True when the item is in the freeze (emergency stop) state.
    bool isFreeze();
    /// Store, publish ($state, $fault) and notify the onFault object about a state change.
    void notifyState(short state);
    /// Apply the physical outputs for the given state machine state and notify about it.
    int  moveToState(sprinklerState nextState);
    /// Increment the global water counter ("@wCtr") and publish it.
     void         updateCounterValue();
     /// Republish the current state machine status.
     void getAndNotifyStatus();
};
#endif