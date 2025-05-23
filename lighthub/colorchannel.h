
#pragma once
#include "options.h"


#include <abstractout.h>
#include <item.h>

class colorChannel : public abstractOut {
public:

    colorChannel():iaddr(0),numArgs(0) {};
    void link (Item * _item) {
                          abstractOut::link(_item); 
                          iaddr = item->getArg();        //Once retrieve and store base address
                          if (iaddr<0) iaddr=-iaddr;
                          numArgs = item->getArgCount(); // and how many addresses is configured
                        };                       
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized=false) override;
    //int getDefaultStorageType()override;
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false ) =0;
    short getChannelAddr(short n =0);
//    int isActive() override;
protected:
  short iaddr;
  short numArgs;
};
