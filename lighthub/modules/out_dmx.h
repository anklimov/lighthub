
#pragma once
#include "options.h"
#ifndef DMX_DISABLE

#include <abstractout.h>
#include <item.h>
#include "colorchannel.h"

class out_dmx : public colorChannel {
public:

    out_dmx(Item * _item):colorChannel(_item){};
    int Setup() override;
    int Poll(short cause) override;
    int Stop() override;
    int Status() override;
  
    int getChanType() override;
//    int Ctrl(itemCmd cmd, char* subItem=NULL) override;
//    int PixelCtrl(itemCmd cmd) override;
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true ) override;

protected:
};
#endif
