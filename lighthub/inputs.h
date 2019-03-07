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

#include <aJSON.h>

#define IN_ACTIVE_HIGH   2      // High level = PUSHED/ CLOSED/ ON othervise :Low Level. Use INPUT mode instead of INPUT_PULLUP for digital pin
#define IN_ANALOG        64     // Analog input
#define IN_RE            32     // Rotary Encoder (for further use)

#define IN_PUSH_ON       0      // PUSH - ON, Release - OFF (ovverrided by pcmd/rcmd) - DEFAULT
#define IN_PUSH_TOGGLE   1      // Used for push buttons. Every physicall push toggle logical switch  on/off. Toggle on leading edge
#define IN_DHT22         4
#define IN_COUNTER       8
#define IN_UPTIME       16

#define SAME_STATE_ATTEMPTS 3
#define ANALOG_STATE_ATTEMPTS 6
#define ANALOG_NOIZE 1

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
    struct {
        int8_t logicState;
        int8_t bounce;
        int16_t currentValue;
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
    void onAnalogChanged(int newValue);

    int poll();

    static void inline onCounterChanged(int i);
    static void onCounterChanged0();
    static void onCounterChanged1();
    static void onCounterChanged2();
    static void onCounterChanged3();
    static void onCounterChanged4();
    static void onCounterChanged5();



protected:
    void Parse();

    void contactPoll();
    void analogPoll();

    void dht22Poll();


    void counterPoll();

    void attachInterruptPinIrq(int realPin, int irq);

    unsigned long nextPollTime() const;
    void setNextPollTime(unsigned long pollTime);


    void uptimePoll();

    bool publishDataToDomoticz(int , aJsonObject *, const char *format, ...);

    char* getIdxField();
};
