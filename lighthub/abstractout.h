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
    virtual int isActive();
    virtual bool isAllowed(itemCmd cmd){return true;};
    virtual itemCmd getDefaultOnVal(){return itemCmd().Percents255(255);};
    virtual int getChanType(){return 0;}
    virtual int getDefaultStorageType(){return 0;}  /// Remove?? Now getChanType used instead
    int Setup()  override;        
protected:
      Item * item;
};
