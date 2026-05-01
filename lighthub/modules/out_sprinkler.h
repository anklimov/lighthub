
#pragma once
#include "options.h"
#ifdef  SPRINKLER_ENABLE

#include <abstractout.h>
#include <item.h>

#define DRENAGE_TIME 10000


enum sprinklerState {
    SP_UNKNOWN = 0,
    SP_OFF = 1,
    SP_DREN_ON = 2,
    SP_DREN_OPERATE = 3,
    SP_DREN_EMPTY = 4,
    SP_VIN  = 5,
    SP_FULL = 6,
    SP_FAULT_VIN = -1,
    SP_FAULT_DREN = -2  
};

class out_sprinkler : public abstractOut {
public:

    //out_sprinkler(){ /*NO getConfig() here due Poll() optimization*/ };
    bool getConfig();
   
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

protected:
    aJsonObject * gatesObj;
    short vinPin, drenPin, pumpPin;
    short wMaxPin, wMinPin, fbDrenPin, fbPumpPin, wCtrPin;
    bool lastWctrState;

    void pump(bool state);
    void setOutput(short pin, bool value);
    bool isNeedPump(bool steelNeed=false);
    void turnOffValves(); 
    void turnOffAllZones();
    aJsonObject * getZone(const char * name);
    aJsonObject * findNextZone();
    void setZoneActive(aJsonObject * zone, bool active);
    void updateZoneValue(aJsonObject * zone, long value);
    void publishBooleanState(const char * subItem, bool state);
    void publishNumericState(const char * subItem, long value);
    bool isFreeze();
    void notifyState(short state); 
    int  shutdown(sprinklerState nextState);
};
#endif