/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.

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
#pragma once
#include <aJSON.h>
#include "modules/in_ccs811_hdc1080.h"

#define IN_ACTIVE_HIGH   2      // High level = PUSHED/ CLOSED/ ON othervise :Low Level. Use INPUT mode instead of INPUT_PULLUP for digital pin
#define IN_ANALOG        64     // Analog input
#define IN_RE            32     // Rotary Encoder (for further use)

#define IN_PUSH_ON       0      // PUSH - ON, Release - OFF (ovverrided by pcmd/rcmd) - DEFAULT
#define IN_PUSH_TOGGLE   1      // Used for push buttons. Every physicall push toggle logical switch  on/off. Toggle on leading edge

#define IN_DHT22         4
#define IN_CCS811        5
#define IN_HDC1080       6

#define IN_COUNTER       8
#define IN_UPTIME       16

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
#define IS_REQSTATE 0xFF



#define SAME_STATE_ATTEMPTS 3
#define ANALOG_STATE_ATTEMPTS 6
#define ANALOG_NOIZE 1

#define CHECK_SENSOR 1
#define CHECK_INPUT  2
#define CHECK_INTERRUPT 3


#define T_LONG 1000
#define T_IDLE 600
#define T_RPT 300
#define T_RPT_PULSE 200



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


typedef union {
    long int aslong;
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

class Input {
public:
    aJsonObject *inputObj;
    uint8_t inType;
    uint8_t pin;
    inStore *store;

    Input(int pin);

    Input(aJsonObject *obj);

    Input(char *name);

    boolean isValid();

    void onContactChanged(int newValue);
    void onAnalogChanged(float newValue);

    int poll(short cause);
    void setup();

    static void inline onCounterChanged(int i);
    static void onCounterChanged0();
    static void onCounterChanged1();
    static void onCounterChanged2();
    static void onCounterChanged3();
    static void onCounterChanged4();
    static void onCounterChanged5();



protected:
    void Parse();

    void contactPoll(short cause);
    void analogPoll(short cause);

    void dht22Poll();


    void counterPoll();

    void attachInterruptPinIrq(int realPin, int irq);

    unsigned long nextPollTime() const;
    void setNextPollTime(unsigned long pollTime);


    void uptimePoll();

    bool publishDataToDomoticz(int , aJsonObject *, const char *format, ...);

    char* getIdxField();
    bool changeState(uint8_t newState, short cause);
    bool executeCommand(aJsonObject* cmd, int8_t toggle = -1, char* defCmd = NULL);
};
