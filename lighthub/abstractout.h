#pragma once
#include "Arduino.h"
#include "abstractch.h"
#include "itemCmd.h"

class Item;
class chPersistent {};
class abstractOut  : public abstractCh{
public:
    abstractOut(Item * _item):abstractCh(){item=_item;};
    virtual int Ctrl(itemCmd cmd,  char* subItem=NULL) =0;
    virtual int isActive(){return 0;};
    virtual int getDefaultOnVal(){return 100;};
    virtual int getChanType(){return 0;}
protected:
      Item * item;
};
