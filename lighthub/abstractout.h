#pragma once
#include "Arduino.h"
#include "abstractch.h"

class Item;
class abstractOut  : public abstractCh{
public:
    abstractOut(Item * _item):abstractCh(){item=_item;};
    virtual int Ctrl(short cmd, short n=0, int * Parameters=NULL, boolean send=true, int suffixCode=0, char* subItem=NULL) =0;
    virtual int isActive(){return 0;};
    virtual int getDefaultOnVal(){return 100;};
protected:
      Item * item;
};
