#pragma once
#include "Arduino.h"

class abstractCh {
public:
    abstractCh(){};
//    virtual int Setup(int addr) = 0;
    virtual int Poll() = 0;

protected:
//   Input * in;
int publish(char* topic, long value, char* subtopic = NULL);
int publish(char* topic, float value, char* subtopic = NULL );
int publish(char* topic, char * value, char* subtopic = NULL);
//friend Input;
};
