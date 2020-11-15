
#pragma once
#include "options.h"
#ifndef DMX_DISABLE

#include <abstractout.h>
#include <item.h>

class out_dmx : public abstractOut {
public:

    out_dmx(Item * _item):abstractOut(_item){};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int getChanType() override;
    int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    int PixelCtrl(itemCmd cmd);

protected:
};
#endif
