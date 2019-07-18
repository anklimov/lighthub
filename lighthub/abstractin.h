#pragma once
#include "Arduino.h"
#include "abstractch.h"

class Input;
class abstractIn : public abstractCh{
public:
    abstractIn(Input * _in):abstractCh(){in=_in;};

protected:
   Input * in;
int publish(long value, char* subtopic = NULL);
int publish(float value, char* subtopic = NULL );
int publish(char * value, char* subtopic = NULL);
friend Input;
};
