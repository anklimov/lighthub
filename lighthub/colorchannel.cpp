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

int colorChannel::Ctrl(itemCmd cmd, char* subItem, bool toExecute)
{
debugSerial<<F("clrCtr: ");
cmd.debugOut();
int suffixCode = cmd.getSuffix();

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
case S_HSV:
          PixelCtrl(cmd, subItem, toExecute);
          return 1;
case S_CMD:
      item->setCmd(cmd.getCmd());
      switch (cmd.getCmd())
          {
          case CMD_ON:
            PixelCtrl(cmd,subItem, true);
    //        item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            return 1;

            case CMD_OFF:
              PixelCtrl(cmd, subItem, true);
    //          item->SendStatus(SEND_COMMAND);
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
