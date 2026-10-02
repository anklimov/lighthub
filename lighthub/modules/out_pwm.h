
#pragma once
#include "options.h"
#ifndef PWM_DISABLE

#include <abstractout.h>
#include <item.h>
#include "colorchannel.h"

/**
 * @brief Hardware PWM output driver (config channel type CH_PWM = 3).
 *
 * Drives one or more pins with analogWrite() PWM. The number of
 * configured pins defines the color channel type:
 * - 1 pin  - single PWM (percent) output;
 * - 3 pins - CH_RGB (3), R/G/B;
 * - 4 pins - CH_RGBW (1), R/G/B + white;
 * - 5 pins - CH_RGBWW (17), R/G/B + warm/white.
 *
 * JSON config (item argument array):
 * @code
 * [3, [pin0, pin1, pin2, (pin3, (pin4))]]
 * @endcode
 *
 * On Mega2560 the hardware PWM of timers 3 and 4 is reconfigured to a
 * 4000 Hz frequency (prescaler = 2) so the white channels stay in sync
 * with the RGB ones. On ESP32 the driver is a stub (no PWM yet).
 *
 * Accepts the standard colorChannel control suffixes: /set (percent or
 * color value), /HSV, /cmd ON/OFF.
 */
class out_pwm : public colorChannel {
public:

//    out_pwm():numChannels(0){};
    /// Configure pin directions and (on Mega2560) the 4000 Hz PWM timers.
    int Setup() override;
    /// Release the pins back to INPUT and mark the driver uninitialized.
    int Stop() override;

    
    /// Channel type derived from the number of configured pins (RGB/RGBW/RGBWW) or the raw item type.
    int getChanType() override;
    //int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    /// Apply the command to the PWM pins (single percent or R/G/B/(W) values).
    int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false ) override; 

//protected:
//    short numChannels;
};
#endif
