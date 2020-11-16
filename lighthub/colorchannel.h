
#pragma once
#include "options.h"


#include <abstractout.h>
#include <item.h>

class colorChannel : public abstractOut {
public:

    colorChannel(Item * _item):abstractOut(_item){};
    int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true ) =0;

protected:
};
