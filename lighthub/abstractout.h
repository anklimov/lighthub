#pragma once
#include "Arduino.h"

class Input;
class abstractOut {
public:
    abstractOut(Input * _in){in=_in;};
    virtual int Setup(int addr) = 0;
    virtual int Poll() = 0;
protected:

};
