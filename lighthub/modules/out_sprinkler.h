
#pragma once
#include "options.h"
#ifdef  SPRINKLER_ENABLE

#include <abstractout.h>
#include <item.h>

#define DRENAGE_ON_TIME 10000
#define VIN_MAX_TIME 1200000UL
#define DRENAGE_MAX_TIME 2000000UL

#define LASTWCTRLSTATE 1
#define LASTWCTRLSTATE_ALL 2
#define LASTWMAXSTATE 4
#define LASTWMINSTATE 8
#define LASTFBDRENSTATE 16
#define LASTFBPUMPSTATE 32
#define LASTPUMPSTATE 64
#define LASTDRENSTATE 128
#define LASTVINSTATE 256

enum sprinklerState {
    SP_UNKNOWN = 0,
    SP_OFF = 1,
    SP_DREN_ON = 2,
    SP_DREN_OPERATE = 3,
    SP_DREN_EMPTY = 4,
    SP_VIN  = 5,
    SP_FULL = 6,
    SP_DRYING = 7,
    SP_FAULT_VIN = -1,
    SP_FAULT_DREN = -2  
};

class out_sprinkler : public abstractOut {
public:

    //out_sprinkler(){ /*NO getConfig() here due Poll() optimization*/ };
    bool getConfig();
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};   
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    //int Status() override;  
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

protected:
    aJsonObject * gatesObj;
    short vinPin, drenPin, pumpPin;
    short wMaxPin, wMinPin, fbDrenPin, fbPumpPin, wCtrPin;


    void pump(bool state);
    void dren(bool state);
    void vin(bool state);
    void setOutput(short pin, bool value);
    bool isNeedPump(bool steelNeed=false);
    void turnOffValves(); 
    void turnOffAllZones();
    aJsonObject * getZone(const char * name);
    aJsonObject * findNextZone();
    void setZoneActive(aJsonObject * zone, bool active);
    void updateZoneValue(aJsonObject * zone, long value);
    bool publishBooleanState(const char * subItem, bool state);
    void publishNumericState(const char * subItem, long value);
    void publishBooleanStateIfChanged(const char * subItem, bool state, uint32_t flag, uint32_t & lastState);
    bool isFreeze();
    void notifyState(short state); 
    int  moveToState(sprinklerState nextState);
     void         updateCounterValue();     
};
#endif