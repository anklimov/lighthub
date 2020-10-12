#pragma once
#include "Arduino.h"
#include "abstractch.h"

class Item;
class chPersistent {};
class abstractOut  : public abstractCh{
public:
    abstractOut(Item * _item):abstractCh(){item=_item;};
    virtual int Ctrl(short cmd, short n=0, int * Parameters=NULL, int suffixCode=0, char* subItem=NULL) =0;
    virtual int isActive(){return 0;};
    virtual int getDefaultOnVal(){return 100;};
    virtual int getChanType(){return 0;}
protected:
      Item * item;
};
