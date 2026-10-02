
#pragma once
#include "options.h"
#ifndef COUNTER_DISABLE

#include <abstractout.h>
#include <item.h>

/**
 * @brief Auto-summing (virtual) counter driver, channel type CH_COUNTER = 20.
 *
 * No GPIO involved: every configured period the driver adds a fixed impulse
 * value to the stored channel value (software "tick" generator).
 *
 * JSON config (itemArg): [impulse_coef, period_sec]
 *  - impulse_coef: value added to the counter on each tick (may be fractional);
 *  - period_sec: period between ticks in seconds.
 *
 * Ctrl:
 *  - /set - a non-zero value starts accumulation, zero stops it;
 *  - /cmd - on-like commands (on/heat/cool/auto/fan/dry/heatcool) start, off stops.
 *
 * Status: the accumulated counter value, republished after every tick.
 */
class out_counter : public abstractOut {
public:
    /** @brief Initializes the channel state (CST_INITIALIZED); no hardware. */
    int Setup() override;
    /** @brief When the tick period expires, adds impulse_coef to the stored value and sends the status. */
    int Poll(short cause) override;
    /** @brief Marks the channel as unknown. */
    int Stop() override;
    /** @brief Returns CH_COUNTER (20). */
    int getChanType() override;
    /** @brief Starts/stops accumulation via /set (non-zero value) and /cmd on/off (see class doc). */
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;
};
#endif
