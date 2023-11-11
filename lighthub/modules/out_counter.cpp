#ifndef COUNTER_DISABLE
#include "modules/out_counter.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"



int  out_counter::Setup()
{
  abstractOut::Setup();    
  setStatus(CST_INITIALIZED);
return 1;
}

int  out_counter::Stop()
{
setStatus(CST_UNKNOWN); 
return 1;
}



int out_counter::Poll(short cause)
{
  if (cause==POLLING_SLOW || cause==POLLING_INT) return 0;
  if (!item) return 0;

   uint32_t timer = item->getExt(); 
   uint32_t period  = item->getFloatArg(1)*1000.0;  
   if (timer && period && isTimeOver(timer,millis(),period))
    {
      uint32_t impulse = item->getFloatArg(0)*TENS_BASE; 
      item->setExt(millisNZ());

      itemCmd st;
      st.loadItem(item,FLAG_PARAMETERS|FLAG_COMMAND);
      uint32_t val = st.getTens_raw();
      debugSerial<<"CTR: tick val:"<<val<< " + "<< impulse << endl; 
      val+=impulse;
      st.Tens_raw(val);
      st.saveItem(item);
      debugSerial<<"CTR: tick saved val:"<<val<<endl; 
      item->SendStatus(FLAG_PARAMETERS);
    }   

    return 0;
};


int out_counter::Ctrl(itemCmd cmd, char* subItem, bool toExecute,bool authorized)
{
debugSerial<<F("Counter: ");
cmd.debugOut();

int suffixCode;
if (cmd.isCommand()) suffixCode = S_CMD;
   else suffixCode = cmd.getSuffix();

switch(suffixCode)
{ 
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
          if (toExecute)
          {
          if (cmd.getPercents255()) 
            {
             if (!item->getExt()) 
                                    {
                                    item->setExt(millisNZ());
                                    }
            }
          else        
                    {     
                    item->setExt(0);
                    } 
          }         
          return 1;
case S_CMD:

      switch (cmd.getCmd())
          {
          case CMD_ON:
          case CMD_HEAT:
          case CMD_COOL:
          case CMD_AUTO:
          case CMD_FAN:
          case CMD_DRY:
             if (!item->getExt())  
                                      {
                                      item->setExt(millisNZ());
                                      }
            return 1;

            case CMD_OFF:
             item->setExt(0);
            return 1;

            default:
            debugSerial<<F("Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd

    default:
  debugSerial<<F("Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

return 0;

}

int out_counter::getChanType()
{
   return CH_COUNTER;
}
#endif