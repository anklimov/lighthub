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
    virtual int Anounce () {return 0;};
    virtual int Stop() {return 0;};         //Should free resources
    virtual int Status() {return CST_UNKNOWN;}


protected:
virtual int publishTopic(const char* topic, long value, const char* subtopic = NULL);
virtual int publishTopic(const char* topic, float value, const char* subtopic = NULL );
virtual int publishTopic(const char* topic, const char * value, const char* subtopic = NULL);
//friend Input;
};
