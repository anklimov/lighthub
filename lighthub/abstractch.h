#pragma once
#include "Arduino.h"

#define CST_UNKNOWN  0
#define CST_FAILED 1
#define CST_INITIALIZED  2
#define CST_USER 3

class abstractCh {
public:
    abstractCh(){};
    virtual ~abstractCh(){};
    virtual int Poll(short cause) {return 0;}
    virtual int Setup() =0;        //Should initialize hardware and reserve resources
   // virtual int Anounce () {return 0;};
    virtual int Stop() {return 0;};         //Should free resources
    virtual int Status() {return CST_UNKNOWN;}
    virtual void setStatus(uint8_t status) {}


protected:
int publishTopic(const char* topic, long value, const char* subtopic = NULL);
int publishTopic(const char* topic, float value, const char* subtopic = NULL );
int publishTopic(const char* topic, const char * value, const char* subtopic = NULL);

};
