#pragma once
#include "Arduino.h"

class abstractCh {
public:
    abstractCh(){};
    virtual ~abstractCh(){};
    virtual int Poll() = 0;
    virtual int Setup() =0;
    virtual int Anounce () {};

protected:
virtual int publishTopic(char* topic, long value, char* subtopic = NULL);
virtual int publishTopic(char* topic, float value, char* subtopic = NULL );
virtual int publishTopic(char* topic, char * value, char* subtopic = NULL);
//friend Input;
};
