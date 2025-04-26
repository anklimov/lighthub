#include "item.h"
#include "abstractout.h"
#include "itemCmd.h"
#include "Arduino.h"
#include "textconst.h"

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


int abstractOut::pubAction(bool state)
{
char subtopic[10]="/";
char val[10];  

strcat_P(subtopic,action_P);
short cmd=item->getCmd();
if (state) 
 switch(cmd)
 {
  case CMD_COOL:
     strcpy_P(val,cooling_P);
     break;
  //case CMD_AUTO:
  //case CMD_HEAT:
  //case CMD_ON:
  //   
  //   break;
  case CMD_DRY:
     strcpy_P(val,drying_P);
     break;
  case CMD_FAN:
      strcpy_P(val,fan_P);
     break;
  default:
     strcpy_P(val,heating_P);
  }
   else //turned off
       if (cmd==CMD_OFF)  strcpy_P(val,off_P);
          else strcpy_P(val,idle_P);
return publishTopic(item->itemArr->name,val,subtopic);

}