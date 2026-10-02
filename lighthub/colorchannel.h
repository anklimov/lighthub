
#pragma once
#include "options.h"


#include <abstractout.h>
#include <item.h>

/**
 * @brief Base class for multi-channel color output drivers (PWM, LED strip).
 *
 * Holds the base pin address and the number of configured channel
 * addresses and implements the common color control logic; the actual
 * output is delegated to the derived PixelCtrl().
 *
 * Color state is stored in HSV: hue 0..365, saturation 0..100,
 * value 0..255, plus an optional color temperature (153..500 mireds)
 * used by RGBWW channels to mix the two white channels.
 *
 * Control (Ctrl) suffixes: /set (percent or color value), /HSV,
 * /cmd ON/OFF (ON restores the stored value, OFF clears it).
 */
class colorChannel : public abstractOut {
public:

    /// Default constructor.
    colorChannel():iaddr(0),numArgs(0) {};
    /// Attach to an item; retrieve and store the base address and the number of channel addresses.
    void link (Item * _item) {
                          abstractOut::link(_item); 
                          iaddr = item->getArg();        //Once retrieve and store base address
                          if (iaddr<0) iaddr=-iaddr;
                          numArgs = item->getArgCount(); // and how many addresses is configured
                        };                       
    /// Dispatch /set, /HSV and /cmd ON/OFF commands to PixelCtrl.
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized=false) override;
    //int getDefaultStorageType()override;
    /// Apply the color/brightness command to the output (implemented by the driver).
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false ) =0;
    /// Get the pin address of the n-th channel (n=0 - base address).
    short getChannelAddr(short n =0);
//    int isActive() override;
protected:
  short iaddr;
  short numArgs;
};
