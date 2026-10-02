

#pragma once
#include "Arduino.h"
#include "abstractch.h"

class Input;
/**
 * @brief Base class of input driver modules (sensors, encoders, ...).
 *
 * abstractIn extends abstractCh for drivers attached to an Input. The `in`
 * pointer references the Input's config entry; the protected publish()
 * helpers convert the measured value to a string and publish it to the
 * MQTT topic given by the "emit" field of the input.
 */
class abstractIn : public abstractCh{
public:
    abstractIn(Input * _in):abstractCh(){in=_in;};

protected:
    /// Pointer to the Input (config entry) this driver is attached to
   Input * in;
    /** Publish an integer reading to the input's "emit" topic (retained). */
int publish(long value, const char* subtopic = NULL);
    /** Publish a float reading to the input's "emit" topic (retained). */
int publish(float value, const char* subtopic = NULL );
    /** Publish a string reading to the input's "emit" topic (retained). */
int publish(char * value, const char* subtopic = NULL);
friend Input;
};
