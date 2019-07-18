
#pragma once
#ifndef SPILED_DISABLE
#include <abstractout.h>

class out_SPILed : public abstractOut {
public:

    out_SPILed(Item * _item):abstractOut(_item){};
    int Setup() override;
    int Poll() override;
    virtual int Ctrl(short cmd, short n=0, int * Parameters=NULL, boolean send=true, int suffixCode=0, char* subItem=NULL) override;

protected:
};
#endif
