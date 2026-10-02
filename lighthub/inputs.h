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
 * @file inputs.h
 * @brief LightHub input system (digital/analog pins, sensors, encoders).
 *
 * Every physical input (button, switch, analog pin, rotary encoder,
 * DHT22/CCS811/HDC1080 sensor, counter, uptime) is an #Input entry of the
 * global #inputs JSON object. An input debounces its signal, runs an IS_*
 * state machine and executes the actions configured in the "act" field
 * (commands to items / emit topics). The runtime state of each input is
 * packed into the #inStore union stored in the JSON object itself.
 */
#pragma once
#include <aJSON.h>
#include "modules/in_ccs811_hdc1080.h"
#include "itemCmd.h"
#ifdef ROTARYENCODER
#include "RotaryEncoder.h"
#endif

/**
 * Input type flags (bitmask, combined with "|"):
 * - IN_PUSH_ON (0) - button: ON while pressed, OFF on release (overridden by scmd/rcmd) - default
 * - IN_PUSH_TOGGLE (1) - every physical push toggles the logical switch on/off (toggle on leading edge)
 * - IN_ACTIVE_HIGH (2) - high level = PUSHED/CLOSED/ON, otherwise low; use INPUT mode instead of INPUT_PULLUP
 * - IN_RE (32) - rotary encoder input
 * - IN_ANALOG (64) - analog input
 * - IN_I2C (128) - pin on an MCP23017 I2C expander
 *
 * Sensor types (no GPIO edge, polled in the CHECK_SENSOR cycle):
 * - IN_DHT22 (4), IN_CCS811 (5), IN_HDC1080 (6), IN_ULTRASONIC (7),
 *   IN_COUNTER (8), IN_UPTIME (16).
 *
 * Examples: 0 - plain button (default), 1 - toggle button, 2 - active-high
 * switch, 64 - analog input, 66 - active-high analog input,
 * 32 (IN_RE) combined with 0/1 - rotary encoder, 128 - I2C expander button.
 */
#define IN_ACTIVE_HIGH   2      // High level = PUSHED/ CLOSED/ ON othervise :Low Level. Use INPUT mode instead of INPUT_PULLUP for digital pin
#define IN_ANALOG        64     // Analog input
#define IN_RE            32     // Rotary Encoder (for further use)
#define IN_I2C           128    // MCP23017

#define IN_PUSH_ON       0      // PUSH - ON, Release - OFF (ovverrided by pcmd/rcmd) - DEFAULT
#define IN_PUSH_TOGGLE   1      // Used for push buttons. Every physicall push toggle logical switch  on/off. Toggle on leading edge

#define IN_DHT22         4      // DHT22 temperature/humidity sensor
#define IN_CCS811        5      // CCS811 eCO2 sensor
#define IN_HDC1080       6      // HDC1080 temperature/humidity sensor
#define IN_ULTRASONIC    7      // Ultrasonic distance sensor

#define IN_COUNTER       8      // Hardware counter (IRQ) input
#define IN_UPTIME       16      // Publishes system uptime

/**
 * Input state machine states (IS_*):
 * - IS_IDLE - initial state, contact not touched yet
 * - IS_PRESSED / IS_RELEASED - moment of press / release (commands fire here)
 * - IS_LONG - long press detected (after T_LONG ms)
 * - IS_REPEAT - auto-repeat while held (T_RPT period)
 * - IS_WAITPRESS / IS_WAITRELEASE - waiting for the edge (delayed state)
 * - IS_PRESSED2/3, IS_LONG2/3, IS_REPEAT2/3 - 2nd/3rd contact states (multi-pin inputs)
 * - IS_REQSTATE - request: apply the stored reqState on the next poll
 * - IS_NOP - no state change
 */
#define IS_IDLE 0
#define IS_PRESSED 1
#define IS_RELEASED 2
#define IS_LONG 3
#define IS_REPEAT 4
#define IS_WAITPRESS 5
#define IS_PRESSED2 6
#define IS_RELEASED2 7
#define IS_LONG2 8u
#define IS_REPEAT2 9u
#define IS_PRESSED3 10u
#define IS_LONG3 11u
#define IS_REPEAT3 12u
#define IS_WAITRELEASE 13u
#define IS_REQSTATE 0xFF
#define IS_NOP 0xF



/// Debounce: number of identical reads / analog samples before a state change
#define SAME_STATE_ATTEMPTS 2
#define ANALOG_STATE_ATTEMPTS 6
/// Analog noise threshold (ADC steps)
#define ANALOG_NOIZE 1

/**
 * Poll causes (see Input::Poll()):
 * - CHECK_SENSOR - slow cycle (sensors: DHT22, CCS811, counters, ...)
 * - CHECK_INPUT  - fast cycle (contacts, analog)
 * - CHECK_INTERRUPT - realtime (ISR-driven)
 * - CHECK_ULTRASONIC - ultrasonic ping
 */
#define CHECK_SENSOR 1
#define CHECK_INPUT  2
#define CHECK_INTERRUPT 3
#define CHECK_ULTRASONIC 4


/** Timing parameters (ms): long press, idle, auto-repeat period, repeat pulse, max repeat. */
#define T_LONG 1000
#define T_IDLE 600
#define T_RPT 300
#define T_RPT_PULSE 150
#define T_REPEAT 30000



// in syntaxis
// "pin": { "T":"N", "emit":"out_emit", item:"out_item", "scmd": "ON,OFF,TOGGLE,INCREASE,DECREASE", "rcmd": "ON,OFF,TOGGLE,INCREASE,DECREASE", "rcmd":"repeat_command" }

//
//Switch/Restore all
//"pin": { "T":"1", "emit":"/all", item:"local_all", "scmd": "OFF", "rcmd": "RESTORE"}

//
//Normal (not button) Switch (toggled mode)
//"pin": { "T":"0", "emit":"/light1", item:"light1", "scmd": "TOGGLE", "rcmd": "TOGGLE"}
// or
// "pin": { "T":"xx", "emit":"/light1", item:"light1"}

//Use Button
//"pin": { "T":"1", "emit":"/light1", item:"light1", "scmd": "ON", "rcmd": "OFF"}
// or
// "pin": { "T":"1", "emit":"/light1", item:"light1"}
//or
// "pin": { "emit":"/light1", item:"light1"}

//1-Button dimmer
//"pin": { "T":"1", "emit":"/light1", item:"light1", "scmd": "ON", srcmd:"INCREASE",rrcmd:"DECREASE",  "rcmd": "OFF"}
// or
// "pin": { "T":"xx", "emit":"/light1", item:"light1"}

//2-Buttons dimmer
//"pin1": { "T":"0", "emit":"/light1", item:"light1", "scmd": "ON", repcmd:"INCREASE"}
//"pin2": { "T":"0", "emit":"/light1", item:"light1", "scmd": "OFF", repcmd:"INCREASE"}


extern aJsonObject *inputs;


/**
 * @brief Per-input runtime state, packed into the JSON object's valueint.
 *
 * Digital layout: toggle flags of 3 contacts, last raw value, delayed-state
 * flag, bounce counter, current/required state (IS_*) and a 16-bit timestamp.
 * Analog layout: logic state plus a 16-bit current ADC value.
 */
typedef union {
    uint32_t aslong;
    uint32_t timestamp;
    // Analog input structure
    struct {
        uint8_t reserved;
        uint8_t logicState;
        int16_t currentValue;
    };
    // Digital input structure
    struct {
        uint8_t  toggle1:1;
        uint8_t  toggle2:1;
        uint8_t  toggle3:1;
        uint8_t  lastValue:1;
        uint8_t  delayedState:1;
        uint8_t  bounce:3;
        uint8_t  state:4;
        uint8_t  reqState:4;
        uint16_t timestamp16;

    };

} inStore;

/**
 * @brief A single LightHub input (digital/analog pin, sensor, encoder, ...).
 *
 * Input reads one physical input, debounces it, runs the IS_* state machine
 * and executes the actions (commands) configured in the "act" field of the
 * input's JSON object. Supported types are defined by the IN_* flags:
 * buttons, switches, analog inputs, rotary encoders, DHT22/CCS811/HDC1080
 * sensors, hardware counters and uptime.
 */
class Input {
public:
    /// Pointer to the input's JSON object in the global inputs tree
    aJsonObject *inputObj;
    /// Input type (IN_* flags)
    uint8_t inType;
    /// First pin number
    uint8_t pin;
    /// Second pin number (rotary encoder, ...)
    uint8_t pin2;
    /// Runtime state, packed in the JSON object's valueint
    inStore *store;

    /**
     * @brief Create an input from its JSON object.
     * @param obj - the input object (or a nested "act" sub-object)
     * @param configObj - object holding the "T"/"#" fields (defaults to obj)
     */
    Input(aJsonObject *obj, aJsonObject * configObj = NULL);
    /** @brief Create an input by name from the global inputs object. */
    Input(char *name);

    /** @return true if the input was parsed (store != NULL). */
    boolean isValid();

    /** ISR entry: raw digital edge; the state change is applied on the next Poll(). */
    void onContactChanged(int newValue);
    /** ISR entry for an analog/encoder change carrying the new value. */
    void onAnalogChanged(itemCmd newValue);

    /**
     * Poll the input by cause: CHECK_INPUT/CHECK_INTERRUPT for contacts,
     * CHECK_SENSOR for sensors, CHECK_ULTRASONIC for ultrasonic.
     */
    int Poll(short cause);
    /** Initialize pin modes / sensor drivers. */
    void setup();
    /** Release resources (e.g. delete the rotary encoder instance). */
    void stop();

    /** ISR handler: increment a hardware counter value. */
    static void inline onCounterChanged(int i);
    /** ISR handlers for counter interrupt numbers 0..5. */
    static void onCounterChanged0();
    static void onCounterChanged1();
    static void onCounterChanged2();
    static void onCounterChanged3();
    static void onCounterChanged4();
    static void onCounterChanged5();



protected:
    /** Parse the "T" (type) and "#" (pin / [pin, pin2]) fields of the input object. */
    void Parse(aJsonObject * configObj = NULL);

    /** Debounce the contact and advance the IS_* state machine; fire actions on state change. */
    #ifdef ROTARYENCODER
    void contactPoll(short cause, RotaryEncoder * re = NULL);
    #else
    void contactPoll(short cause);
    #endif

    /** Sample the analog value with noise filtering; fire actions on change. */
    void analogPoll(short cause);

    /** Read the DHT22 temperature/humidity, execute the temp/hum actions and emit. */
    void dht22Poll();


    /** Publish the hardware counter value to the emit topic. */
    void counterPoll();

    /** Attach a rising-edge ISR to a counter pin. */
    void attachInterruptPinIrq(int realPin, int irq);

    /** @return the timestamp of the next allowed poll for this pin. */
    unsigned long nextPollTime() const;
    /** Set the timestamp of the next allowed poll for this pin. */
    void setNextPollTime(unsigned long pollTime);


    /** Publish the system uptime (millis) to the emit topic. */
    void uptimePoll();

    /** Publish a sensor value in the Domoticz JSON format to the emit topic. */
    bool publishDataToDomoticz(int , aJsonObject *, const char *format, ...);

    /** @return the Domoticz "idx" field of the input, or NULL. */
    char* getIdxField();
    /**
     * Apply a new IS_* state: debouncing, delayed-state handling and
     * execution of the actions configured for the state.
     */
    bool changeState(uint8_t newState, short cause, aJsonObject * currentInputObject, bool contactState, bool calledOnTimer = false);
    /** Create the RotaryEncoder instance for an IN_RE input. */
    void setupRotaryEncoder();

    /** @return the currently active sub-input object of an "act" group (or the input itself). */
    aJsonObject * getCurrentInput();

    /** Select the active sub-input of an "act" group by object. @return true on success. */
    bool setCurrentInput(aJsonObject * obj);
    /** Select the active sub-input of an "act" group by 1-based index. @return true on success. */
    bool setCurrentInput(int n);
    /** Select the active sub-input of an "act" group by name. @return true on success. */
    bool setCurrentInput(char * name);
    /** Process the "activate" instruction: switch to another input. @return true if handled. */
    bool checkInstructions(aJsonObject * obj);

    //bool executeCommand(aJsonObject* cmd, int8_t toggle = -1, char* defCmd = NULL);
};



/**
 * @brief Cache of raw pin reads to avoid multiple GPIO/I2C accesses
 *        within one poll cycle.
 */
class readCache {
public:
  readCache();
  /** Cached analogRead (with an optional trigger pin, e.g. for ultrasonic). */
  uint16_t analogReadCached (uint8_t pin, uint8_t trigPin=0, uint8_t _type = IN_ANALOG);
  /** Cached digitalRead. */
  uint8_t  digitalReadCached(uint8_t pin);
  #ifdef MCP23017
  /** Cached MCP23017 bit read. */
  uint8_t  I2CReadBit(uint8_t type, uint8_t addr, uint8_t pin);
  #endif

  /** Invalidate the cached read so the next read hits the hardware. */
  void invalidateInputCache();
protected:
  /// Cached I2C address / type / value
  uint8_t   addr;
  uint8_t   type;
  uint16_t  cached_data;
};

extern readCache inCache;
