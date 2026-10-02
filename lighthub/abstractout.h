#pragma once
#include "Arduino.h"
#include "abstractch.h"
#include "itemCmd.h"

class Item;
/** @brief Placeholder for driver-persistent data attached to an item (I_EXT field). */
class chPersistent {};
/**
 * @brief Base class of output driver modules (one per channel type).
 *
 * abstractOut is the interface between the item core and a concrete device
 * driver (out_dmx, out_relay, out_Modbus, ...). The driver is created by
 * driverFactory, linked to the current item via link(), controls the
 * hardware in Ctrl() and reports activity via isActive(). Drivers are
 * shared between items of the same channel type, so all state must be
 * derived from the linked item rather than stored per-instance.
 */
class abstractOut  : public abstractCh{
public:
    //abstractOut(Item * _item):abstractCh(){item=_item;};
    abstractOut():item(NULL){};
    /** Attach (or detach with NULL) the item this driver currently operates on. */
    virtual void link(Item * _item){item=_item;};
    /**
     * @brief Execute a command on the hardware. Pure virtual - driver-specific.
     * @param cmd - command and/or value to execute
     * @param subItem - subchannel for multi-channel drivers
     * @param toExecute - false means bookkeeping only (e.g. channel already inactive)
     * @param authorized - true if the caller is authorized
     */
    virtual int Ctrl(itemCmd cmd,  char* subItem=NULL, bool toExecute=true, bool authorized = false) =0;
    /** @return channel activity: 1 ON, 0 OFF, <0 unknown (core falls back to stored command). */
    virtual int isActive();
    /** Veto a command (driver-specific restrictions). @return true if the command is allowed. */
    virtual bool isAllowed(itemCmd cmd){return true;};
    /** @return the value used when the channel is turned ON without an explicit value. */
    virtual itemCmd getDefaultOnVal(){return itemCmd().Percents255(255);};
    /** @return the channel type (CH_*) handled by this driver. */
    virtual int getChanType(){return 0;}
   // virtual int getDefaultStorageType(){return 0;}  /// Remove?? Now getChanType used instead
    /** @return current CST_* status of the driver. */
    virtual int Status() override;
    /** Set the CST_* status of the driver. */
    virtual void setStatus(uint8_t status) override;
    /** Driver-level setup wrapper (validates the linked item before Setup of hardware). */
    int Setup()  override;  
    /** @return the item currently linked to this driver, or NULL. */
    Item * getItem() {return item;}
protected:
    /** Publish an on/off state change to the MQTT topics of the linked item. */
    int pubAction(bool state);

      /// Item currently linked to this driver
      Item * item;
};

