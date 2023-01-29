#include "colorchannel.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"

//colorChannel::colorChannel(Item * _item)


short colorChannel::getChannelAddr(short n)
{
  if (!n) return iaddr;
  if (n+1>numArgs) return iaddr+n;
  return item->getArg(n);
}


int colorChannel::getDefaultStorageType()
    {

      if (item)
          switch (numArgs)
          {
            case 3:
            case 4:
            case 5:
              return ST_HSV255;
            case 1:
              return ST_PERCENTS255;  
          }
          return ST_VOID;
        }

/*
int colorChannel::isActive()
{
itemCmd st;
st.loadItem(item);
int val = st.getInt();
debugSerial<< F(" val:")<<val<<endl;
return val;
}
*/

int colorChannel::Ctrl(itemCmd cmd, char* subItem, bool toExecute)
{
debugSerial<<F("clrCtr: ");
cmd.debugOut();
int suffixCode;
if (cmd.isCommand()) suffixCode = S_CMD;
   else suffixCode = cmd.getSuffix();

switch(suffixCode)
{ int vol;
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
//case S_ESET:
case S_HSV:
          PixelCtrl(cmd, subItem, toExecute);
          return 1;
case S_CMD:
      //item->setCmd(cmd.getCmd());
      switch (cmd.getCmd())
          {
          case CMD_ON:
            if (vol=cmd.getPercents()<MIN_VOLUME && vol>=0) 
                {
                cmd.setPercents(INIT_VOLUME);
                cmd.saveItem(item);
                item->SendStatus(FLAG_PARAMETERS | FLAG_SEND_DEFFERED);
                };
            PixelCtrl(cmd,subItem, true);
    //        item->SendStatus(FLAG_COMMAND | FLAG_PARAMETERS );
            return 1;

            case CMD_OFF:
              cmd.param.asInt32=0;
              PixelCtrl(cmd, subItem, true);
    //          item->SendStatus(FLAG_COMMAND);
            return 1;

            default:
            debugSerial<<F("Unknown cmd ")<<cmd.getCmd()<<endl;
          } //switch cmd

    default:
  debugSerial<<F("Unknown suffix ")<<suffixCode<<endl;
} //switch suffix

cmd.debugOut();
return 0;

}
