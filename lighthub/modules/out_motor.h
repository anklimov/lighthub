#pragma once
#include "options.h"
#ifndef MOTOR_DISABLE
#include <abstractout.h>
#include <item.h>
#include "itemCmd.h"

#ifndef POS_ERR
#define POS_ERR 10
#endif

#define MIN_PWM 70
// The number of simultaniusly working motors
#ifndef MOTOR_QUOTE
#define MOTOR_QUOTE 2
#endif

static int8_t motorQuote = MOTOR_QUOTE;

/**
 * @brief Motorized gate/shutter driver with analog position feedback,
 * channel type CH_MOTOR = 12.
 *
 * Drives the up/down motor pins (with PWM speed control where the pins
 * support PWM) until the analog feedback reaches the target position
 * 0-255. No more than MOTOR_QUOTE (2) motors run simultaneously, and the
 * motor is stopped after maxOnTime ms regardless of position.
 *
 * JSON config (itemArg):
 *  [pinUp, pinDown, pinFeedback, fbOpen, fbClosed, maxOnTime_ms(default 10000)]
 *  - negative pin numbers invert the output; defaults pinUp=32, pinDown=33;
 *  - fbOpen / fbClosed: raw analog readings at fully open / fully closed.
 *
 * Ctrl: /set <0-255> - target position; /cmd on / off (off drives to 0).
 *
 * The current position (0-255) is derived from the analog feedback pin.
 */
class out_Motor : public abstractOut {
public:

    //out_Motor(Item * _item):abstractOut(_item){getConfig();};
    //out_Motor(){};
    /** @brief Links the channel item and reads the pin config from itemArg. */
    void link(Item * _item){abstractOut::link(_item);  if (_item) getConfig();};   
    /** @brief Sets the up/down pins as outputs, the feedback pin as input, resets the action flags. */
    int Setup() override;
    /** @brief Drives the motor toward the target position with speed ramp, enforces the max on-time, and stops at target. */
    int Poll(short cause) override;
    /** @brief Deactivates both motor outputs and resets the channel state. */
    int Stop() override;
    //int isActive() override;
    /** @brief Returns CH_PWM (3). */
    int getChanType() override;
    //int getDefaultStorageType(){return ST_PERCENTS255;};
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL,  int suffixCode=0, char* subItem=NULL) override;
    /** @brief Sets the target position via /set (0-255) and /cmd on/off, requesting a move (see class doc). */
    int Ctrl(itemCmd cmd, char* subItem=NULL, bool toExecute=true, bool authorized = false) override;

    int8_t pinUp;
    int8_t pinDown;
    int8_t pinFeedback;
    int16_t maxOnTime;
    uint16_t feedbackOpen;
    uint16_t feedbackClosed;
    bool inverted;
protected:
    void getConfig();
};
#endif
