#pragma once
#include "options.h"
#ifndef MOTOR_DISABLE
#include <abstractout.h>
#include <item.h>

#ifndef POS_ERR
#define POS_ERR 2
#endif

// The number of simultaniusly working motors
#ifndef MOTOR_QUOTE
#define MOTOR_QUOTE 2
#endif

static int8_t motorQuote = MOTOR_QUOTE;

class out_Motor : public abstractOut {
public:

    out_Motor(Item * _item):abstractOut(_item){getConfig();};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(short cmd, short n=0, int * Parameters=NULL,  int suffixCode=0, char* subItem=NULL) override;

    int8_t pinUp;
    int8_t pinDown;
    int8_t pinFeedback;
    int16_t maxOnTime;
    uint16_t feedbackOpen;
    uint16_t feedbackClosed;
protected:
    void getConfig();
};
#endif
