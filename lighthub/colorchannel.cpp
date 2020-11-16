#include "colorchannel.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"


int out_dmx::Ctrl(itemCmd cmd, char* subItem)
{

int chActive = item->isActive();
bool toExecute = (chActive>0); // execute if channel is active now
int suffixCode = cmd.getSuffix();
itemCmd st(ST_HSV);

if (!suffixCode) toExecute=true; //forced execute if no suffix
if (cmd.isCommand() && !suffixCode) suffixCode=S_CMD; //if some known command recognized , but w/o correct cmd suffix - threat it as command

switch(suffixCode)
{
case S_NOTFOUND:
  // turn on  and set
toExecute = true;
case S_SET:
case S_HSV:
          st.loadItem(item);
          st.assignFrom(cmd);
          PixelCtrl(st, subItem, toExecute);
          st.saveItem(item);

          if (!suffixCode)
          {
            if (chActive>0 && !st.getPercents()) item->setCmd(CMD_OFF);
            if (chActive==0 && st.getPercents()) item->setCmd(CMD_ON);
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS | SEND_DEFFERED);
          }
          else    item->SendStatus(SEND_PARAMETERS | SEND_DEFFERED);
          return 1;
/*
case S_HUE:
     st.setH(uint16_t);
     break;

case S_SAT:
     st.setS(uint8_t);
     break;
*/
case S_CMD:
      item->setCmd(cmd.getCmd());
      switch (cmd.getCmd())
          {
          case CMD_ON:
           //retrive stored values
           if (st.loadItem(item))
            {
            if (st.param.aslong && (st.param.v<MIN_VOLUME)) {
                                                            st.Percents(INIT_VOLUME);
                                                            }

              debugSerial<<F("Restored: ")<<st.param.h<<F(",")<<st.param.s<<F(",")<<st.param.v<<endl;
           }
            else // Not restored
            {
              st.setDefault();
              debugSerial<<st.param.aslong<<F(": No stored values - default\n");
            }

            st.saveItem(item, subItem, true);
            PixelCtrl(st);
            item->SendStatus(SEND_COMMAND | SEND_PARAMETERS );
            return 1;

            case CMD_OFF:
              st.Percents(0);
              PixelCtrl(st, subItem, true);
              item->SendStatus(SEND_COMMAND);
            return 1;
          } //switch cmd
} //switch suffix

debugSerial<<F("Unknown cmd")<<endl;
return 0;

}
