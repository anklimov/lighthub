#pragma once
#include "Arduino.h"
#include "abstractch.h"
#include "itemCmd.h"

class Item;
class chPersistent {};
class abstractOut  : public abstractCh{
public:
    abstractOut(Item * _item):abstractCh(){item=_item;};
    virtual int Ctrl(itemCmd cmd,  char* subItem=NULL, bool toExecute=true) =0;
    virtual int isActive(){return 0;};
    virtual itemCmd getDefaultOnVal(){return itemCmd(ST_PERCENTS,CMD_VOID).Percents(100);};
    virtual int getChanType(){return 0;}
    virtual int getDefaultStorageType(){return ST_PERCENTS;}
protected:
      Item * item;
};
