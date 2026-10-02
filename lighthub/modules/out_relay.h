
#pragma once
#include "options.h"
#ifndef RELAY_DISABLE

#include <abstractout.h>
#include <item.h>

/**
 * @brief Relay (ON/OFF output) driver (CH_RELAY = 6 / CH_RELAYX = 16).
 *
 * Switches a digital output pin. In the simple CH_RELAY form the relay
 * is latched; in the CH_RELAYX form it is a pulse output: after each
 * turn-on the relay stays active for `val`/255 of the configured period
 * and is switched off by the polling loop.
 *
 * JSON config (item argument array):
 * - CH_RELAY:   `[6, pin, val, cmd]`
 * - CH_RELAYX:  `[16, [pin, period_sec], val, cmd]`, period in seconds
 *   (default 5).
 *
 * A negative pin number selects an inverted output (active-low).
 *
 * Commands (/cmd): ON / OFF / TOGGLE / XON / XOFF / HALT / REST,
 * plus a /set percent value which defines the ON duration for CH_RELAYX.
 */
class out_relay : public abstractOut {
public:
    /// Attach to an item and read its pin/period configuration.
    void link(Item * _item){abstractOut::link(_item); if (_item) getConfig();};
    /// Read pin and pulse period from the item arguments (negative pin = inverted).
    void getConfig();
    /// Physically switch the relay and publish the new action state.
    void relay(bool state);
    /// Initialize the pin (OUTPUT, inactive) and remember the state if the item is active.
    int Setup() override;
    /// For CH_RELAYX: toggle the relay on/off according to the configured period and /set value.
    int Poll(short cause) override;
    /// Release the pin and mark the driver uninitialized.
    int Stop() override;
    
    /// Channel type of the driver.
    int getChanType() override;
    /// Handle /set (duration/turn-on) and /cmd (ON/OFF/TOGGLE/XON/XOFF/HALT/REST) commands.
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

protected:
    short pin;
    bool inverted;
    uint32_t period;
};
#endif
