#pragma once
#include "options.h"
#ifndef PID_DISABLE
#include <abstractout.h>
#include <item.h>
#include <PID_v1.h>
#include "itemCmd.h"

#define OUTPUT_TRESHOLD 1.0

class pidPersistent : public chPersistent  {
public:
  PID * pid;
  double output;
  double input;
  double setpoint;
  float  prevOut; 
  int driverStatus;
  uint32_t alarmTimer;
  bool alarmArmed;
  uint16_t alarmTimeout; //in sec
};



class out_pid : public abstractOut {
public:

    out_pid(Item * _item):abstractOut(_item){store = (pidPersistent *) item->getPersistent();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int getDefaultStorageType(){return ST_FLOAT;};
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
    void alarm(bool);


protected:
    pidPersistent * store;
    bool getConfig();
    float  getAlarmVal();
};
#endif
