#pragma once
#include "Arduino.h"

#define CST_UNKNOWN  0
#define CST_INITIALIZED  1

class abstractCh {
public:
    abstractCh(){};
    virtual ~abstractCh(){};
    virtual int Poll() = 0;
    virtual int Setup() =0;        //Should initialize hardware and reserve resources
    virtual int Anounce () {};
    virtual int Stop() {};         //Should free resources
    virtual int Status() {return CST_UNKNOWN;}


protected:
virtual int publishTopic(char* topic, long value, char* subtopic = NULL);
virtual int publishTopic(char* topic, float value, char* subtopic = NULL );
virtual int publishTopic(char* topic, char * value, char* subtopic = NULL);
//friend Input;
};
