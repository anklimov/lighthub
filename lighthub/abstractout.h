#pragma once
#include "Arduino.h"


class abstractOut  : public abstractCh{
public:
    abstractOut(Input * _in):abstractCh(){in=_in;};
    virtual int Setup(int addr) = 0;
    virtual int Poll() = 0;
protected:

};
