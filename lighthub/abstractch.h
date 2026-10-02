#pragma once
#include "Arduino.h"

/**
 * Channel status codes:
 * - CST_UNKNOWN - not initialized yet
 * - CST_FAILED - initialization failed
 * - CST_INITIALIZED - hardware ready
 * - CST_USER - user-defined state
 */
#define CST_UNKNOWN  0
#define CST_FAILED 1
#define CST_INITIALIZED  2
#define CST_USER 3

/**
 * @brief Base class of all abstract channels (input and output drivers).
 *
 * abstractCh defines the lifecycle contract of a driver module:
 * Setup() initializes the hardware and reserves resources, Poll() is
 * called periodically with a cause (POLLING_SLOW/POLLING_FAST/...),
 * Status()/setStatus() track the initialization state (CST_*) and Stop()
 * releases the resources. The class also provides protected publishTopic()
 * helpers that publish a value to a fixed MQTT topic as a retained message.
 */
class abstractCh {
public:
    abstractCh(){};
    virtual ~abstractCh(){};
    /**
     * Periodic poll hook.
     * @param cause - POLLING_SLOW/POLLING_FAST/POLLING_INT/POLLING_1S
     */
    virtual int Poll(short cause) {return 0;}
    /** Initialize hardware and reserve resources. @return 0 on success. */
    virtual int Setup() =0;        //Should initialize hardware and reserve resources
   // virtual int Anounce () {return 0;};
    /** Free resources allocated in Setup(). */
    virtual int Stop() {return 0;};         //Should free resources
    /** @return current CST_* status of the channel. */
    virtual int Status() {return CST_UNKNOWN;}
    /** Set the CST_* status of the channel. */
    virtual void setStatus(uint8_t status) {}


protected:
    /**
     * Publish an integer value to a topic as a retained MQTT message.
     * @param topic - full topic or a name to be prefixed with the T_OUT topic
     * @param subtopic - optional subtopic/suffix appended to the topic
     */
int publishTopic(const char* topic, long value, const char* subtopic = NULL);
    /** Publish a float value to a topic as a retained MQTT message. */
int publishTopic(const char* topic, float value, const char* subtopic = NULL );
    /** Publish a string value to a topic as a retained MQTT message. */
int publishTopic(const char* topic, const char * value, const char* subtopic = NULL, const char* suffix = NULL);

};
