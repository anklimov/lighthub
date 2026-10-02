
#pragma once
#include "options.h"
#ifndef DMX_DISABLE

#include <abstractout.h>
#include <item.h>
#include "colorchannel.h"

/**
 * @brief DMX512 output driver.
 *
 * Channel type depends on the number of configured DMX channels:
 * 1-2 -> CH_DIMMER = 0, 3 -> CH_RGB = 2, 4 -> CH_RGBW = 1, 5 -> CH_RGBWW = 17.
 *
 * JSON config (itemArg): [dch1, dch2, ...] - 1 to 5 DMX channel numbers
 * starting at the channel base address.
 */
class out_dmx : public colorChannel {
public:
    /** @brief Initializes the channel state (CST_INITIALIZED). */
    int Setup() override;
    /** @brief Marks the channel as unknown. */
    int Stop() override;

    /** @brief Returns CH_RGB / CH_RGBW / CH_RGBWW based on the number of configured channels (otherwise the item type). */
    int getChanType() override;
    /** @brief Writes the command value to the DMX channels: single-channel dimmer percents, or RGB/RGBW/RGBWW color (color temperature split into the two white channels for RGBWW). */
    virtual int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false) override;

protected:
};
#endif
