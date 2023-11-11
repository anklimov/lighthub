

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

int abstractOut::Status() 
{
  if (item && item->itemArr)
     return item->itemArr->subtype;
  return CST_UNKNOWN;
  }

void abstractOut::setStatus(uint8_t status) 
{
 if (item && item->itemArr) item->itemArr->subtype = status & 0xF;
}