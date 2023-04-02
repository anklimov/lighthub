 
#include "item.h"
#include "abstractout.h"
#include "itemCmd.h"
int  abstractOut::isActive()
 
 {itemCmd st;  
    switch (item->getCmd())
        { 
            case CMD_OFF:
            case CMD_HALT:
            case CMD_VOID:
              return 0;
              break;
            case CMD_ON:  //trying (PWM ON set=0 issue)
              return 1;  
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