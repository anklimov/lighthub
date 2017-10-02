#include "aJSON.h"

#define IN_ACTIVE_HIGH   128    // High level = PUSHED/ CLOSED/ ON othervise :Low Level
#define IN_ANALOG        64     // Analog input
#define IN_RE            32     // Rotary Encoder (for further use)

#define IN_PUSH_ON       0      // PUSH - ON, Release - OFF (ovverrided by pcmd/rcmd) - DEFAULT
#define IN_PUSH_TOGGLE   1      // Every physicall push toggle logical switch  on/off


// in syntaxis
// "pin": { "T":"N", "emit":"out_emit", item:"out_item", "scmd": "ON,OFF,TOGGLE,INCREASE,DECREASE", "rcmd": "ON,OFF,TOGGLE,INCREASE,DECREASE", "rcmd":"repeat_command" }

// 
//Switch/Restore all
//"pin": { "T":"1", "emit":"/all", item:"local_all", "scmd": "OFF", "rcmd": "RESTORE"}

// 
//Normal (not button) Switch (toggled mode)
//"pin": { "T":"1", "emit":"/light1", item:"light1", "scmd": "TOGGLE", "rcmd": "TOGGLE"}
// or
// "pin": { "T":"xx", "emit":"/light1", item:"light1"}

//Normal (not button) Switch 
//"pin": { "T":"0", "emit":"/light1", item:"light1", "scmd": "ON", "rcmd": "OFF"}
// or
// "pin": { "T":"0", "emit":"/light1", item:"light1"}
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
 

typedef union 
{
  long int aslong;
  struct
      {
        int8_t  reserve;
        int8_t  logicState;
        int8_t  bounce;
        int8_t  cur;      
      };
} inStore;

class Input
{
  public:
  aJsonObject *inputObj;
  uint8_t inType;
  uint8_t pin;
  inStore * store;

  Input(int pin);
  Input(aJsonObject * obj);
  Input(char * name);
  
  boolean isValid ();
  void Changed (int val);

  int Pool ();
  protected:  
  void Parse();
};




