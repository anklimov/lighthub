 
#include "item.h"
#include "abstractout.h"
#include "itemCmd.h"
int  abstractOut::isActive()
 
 {itemCmd st;  
    switch (item->getCmd())
        { 
            case CMD_OFF:
            case CMD_HALT:
              return 0;
              break;
            default:
              st.loadItem(item);
              return st.getPercents255();
        }  
    };

int abstractOut::Setup() 
{  
    if (item && (item->getCmd()==-1)) item->setCmd(CMD_OFF);
    return 1;
  }     