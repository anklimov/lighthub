
#pragma once
#include "options.h"


#include <abstractout.h>
#include <item.h>

class colorChannel : public abstractOut {
public:

    colorChannel(Item * _item):abstractOut(_item) {
                          iaddr = item->getArg();        //Once retrieve and store base address
                          if (iaddr<0) iaddr=-iaddr;
                          numArgs = item->getArgCount(); // and how many addresses is configured
                        };
    int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true ) =0;
    short getChannelAddr(short n =0);
protected:
  short iaddr;
  short numArgs;
};
