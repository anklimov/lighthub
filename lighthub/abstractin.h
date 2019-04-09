#pragma once
#include "Arduino.h"

class Input;
class abstractIn {
public:
    abstractIn(Input * _in){in=_in;};
    virtual int Setup(int addr) = 0;
    virtual int Poll() = 0;

protected:
   Input * in;
int publish(long value, char* subtopic = NULL);
int publish(float value, char* subtopic = NULL );
int publish(char * value, char* subtopic = NULL);
friend Input;
};
