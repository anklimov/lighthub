#ifndef DMX_DISABLE

#include "modules/out_dmx.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "dmx.h"

static int driverStatus = CST_UNKNOWN;

int  out_dmx::Setup()
{
abstractOut::Setup();  
debugSerial<<F("DMX-Out Init")<<endl;
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_dmx::Stop()
{
debugSerial<<F("DMX-Out stop")<<endl;
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_dmx::Status()
{
return driverStatus;
}


int out_dmx::Poll(short cause)
{
return 0;
};

int out_dmx::getChanType()
{
  if (item)
  {
  switch (numArgs)
  {
    case 3:
      return CH_RGB;
    case 4:
      return CH_RGBW; 
    case 5:
      return CH_RGBWW;
    default:
      return item->itemType;
  }
  return 0;
 }
return 0; 
}

int out_dmx::PixelCtrl(itemCmd cmd, char* subItem, bool show)
//int out_dmx::PixelCtrl(itemCmd cmd)
{
if (!item || !show) return 0;

short   cType=getChanType();
uint8_t storageType;

/*
switch (cmd.getCmd()){
  case CMD_OFF:
    cmd.Percents(0);
  break;
}
*/


debugSerial<<F("DMX ctrl: "); cmd.debugOut();


if (cType==CH_DIMMER) //Single channel
  {
    DmxWrite(iaddr, cmd.getPercents255());
    return 1;
  }

 int colorTemp = cmd.getColorTemp(); 

  switch (cType)
  {
    case CH_RGB:
     storageType=ST_RGB;
     break;
    case CH_RGBWW:    
    cmd.setColorTemp(0); //Supress cold conversoin
    case CH_RGBW:
     storageType=ST_RGBW;
     break;
    default:
     storageType=ST_PERCENTS255;
   }

itemCmd st(storageType,CMD_VOID);
st.assignFrom(cmd);

debugSerial<<F("Assigned:");st.debugOut();
    switch (cType)
    {      
      case CH_RGBWW:    
      if (colorTemp == -1) colorTemp = 255/2;
            else colorTemp=map(colorTemp,153,500,0,255);
      DmxWrite(getChannelAddr(3), map(st.param.w,0,255,0,colorTemp));
      DmxWrite(getChannelAddr(4), map(st.param.w,0,255,0,255-colorTemp));
      DmxWrite(iaddr, st.param.r);
      DmxWrite(getChannelAddr(1), st.param.g);
      DmxWrite(getChannelAddr(2), st.param.b);
     
      break;
      case CH_RGBW:
          DmxWrite(getChannelAddr(3), st.param.w);
      case CH_RGB:
      DmxWrite(iaddr, st.param.r);
      DmxWrite(getChannelAddr(1), st.param.g);
      DmxWrite(getChannelAddr(2), st.param.b);
      break;
      default: ;
    }
return 1;
}


#endif
