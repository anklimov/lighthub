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
//int chActive = item->isActive();
//bool toExecute = (chActive>0); // execute if channel is active now
int suffixCode = cmd.getSuffix();
/*
// Since this driver working both, for single-dimmed or PWM channel and color - define storage type
uint8_t storageType;
switch (getChanType())
{
  case CH_RGB:
  case CH_RGBW:
  storageType=ST_HSV;
  break;
  default:
  storageType=ST_PERCENTS;
}
itemCmd st(storageType,CMD_VOID);

if (!suffixCode) toExecute=true; //forced execute if no suffix
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command recognized , but w/o correct cmd suffix - threat it as command
*/

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
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            return 1;

            case CMD_OFF:
              cmd.Percents(0);
              PixelCtrl(cmd, subItem, true);
              item->SendStatus(SEND_COMMAND);
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
