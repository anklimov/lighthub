
#pragma once
#include "options.h"
#ifndef DMX_DISABLE

#include <abstractout.h>
#include <item.h>
#include "colorchannel.h"

class out_dmx : public colorChannel {
public:
    int Setup() override;
    int Stop() override;
  
    int getChanType() override;
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false) override;

protected:
};
#endif
