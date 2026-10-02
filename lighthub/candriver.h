#pragma once
/** @brief Sub-item index meaning "no sub-item" (top of 6-bit field). */
#define NO_SUBITEM 63
/** @brief Flag in the sub-item field: value is a command code, not an index. */
#define SUBITEM_IS_COMMAND 0x20
#ifdef  CANDRV

#if defined(ARDUINO_ARCH_STM32)
#if !defined(HAL_CAN_MODULE_ENABLED)
#define HAL_CAN_MODULE_ENABLED
#endif
#include <STM32_CAN.h>
#endif

#include <itemCmd.h>

#include <Stream.h>
#include <aJSON.h>
#include <streamlog.h>
//#include <config.h> NO!

/** @brief 6-byte MAC address used for device identification over CAN. */
typedef uint8_t macAddress[6];

/**
 * @brief 29-bit extended CAN identifier layout for the LightHub protocol.
 *
 * Bit layout (29-bit extended ID):
 * - bits  0..5   subItemId (6 bits)
 * - bits  6..15  itemId (10 bits); together bits 0..15 form subjId
 * - bits 16..23  deviceId (1 byte)
 * - bits 24..27  payloadType (4 bits)
 * - bit  28      status: 0 = request, 1 = response
 * - bits 29..    reserve (must be 0)
 */
#pragma pack(push, 1)
typedef union 
{
  uint32_t id;
  struct 
  {   union
     {
        struct
         {
           uint16_t subItemId:6;
           uint16_t itemId:10; 
          };
      uint16_t subjId;    
      };
      uint8_t deviceId;
      uint8_t payloadType:4;
      uint8_t status:1; 
      uint8_t reserve:3; //0

  };
} canid_t;

/** @brief Payload (protocol) types carried in the CAN ID payloadType field. */
enum payloadType
{   unknown=0,
    itemCommand=1,   /**< Item command or status (itemCmd payload). */
    lookupMAC=2,     /**< MAC address lookup / device ID exchange. */
    configFrame=3,   /**< Fragment of the JSON config transfer. */
    OTAFrame=4,      /**< Fragment of an OTA firmware transfer. */
    auth=5,          /**< Authentication frame. */
    metric=6,        /**< Metric value (see metricType). */
    sysCmd=7,        /**< System command (see commandType). */
    rawPinCtrl=8     /**< Raw GPIO control of the remote controller. */
};

/** @brief Metric identifiers used in the metric payloadType frames. */
enum metricType
{
    MAC=1,     /**< MAC address. */
    IP=2,      /**< IP address. */
    NetMask=3, /**< Network mask. */
    GW=4,      /**< Default gateway. */
    DNS=5,     /**< DNS server. */
    UpTime=6,  /**< Uptime in seconds. */
    Salt=7     /**< Random salt for authentication. */
};

/** @brief System commands accepted in the sysCmd payloadType frames. */
enum commandType
{
    reboot=1,  /**< Reboot the remote controller. */
    get=2,     /**< Fetch the remote config/state. */
    save=3,    /**< Persist the current config on the remote. */
    load=4     /**< Reload the config on the remote. */
};


#define MAXCANID 0x1FFFFFFF
// Request item status: id.status=1;deviceId=[0xFF | deviceId];itemId=x; RTR bit=true 
// Request on config: id.status=0;deviceId=0;itemId=x payload.mac=mac;crc16=crc_current_config 



/**
 * @brief 8-byte CAN payload, interpreted per payloadType.
 *
 * Overlays the possible frame bodies: item command + parameter,
 * MAC + config CRC, system command + data, or two 32-bit metrics.
 */
typedef union {
    uint8_t data[8];
    char payload[8];
    struct {
     itemCmdStore      cmd;       
     itemArgStore      param;       
    };
    struct {
      macAddress        mac;
      uint16_t          currentConfCRC;       
    };    
    struct {
      uint8_t           sysCmd;       
      uint8_t           sysCmdData[7];        
    };    
    struct {
      uint32_t   metric1;       
      uint32_t   metric2;      
    };
} datagram_t;

/** @brief States of the canDriver / canStream state machine. */
enum canState
{
stateUnknown,       /**< Not yet initialized. */
MACLookup,          /**< Waiting for a lookupMAC response. */
Idle,               /**< Idle, controller ID known. */
StreamOpenedWrite,  /**< canStream open for writing. */
StreamOpenedRead,   /**< canStream open for reading. */
FrameRequested,     /**< A frame was requested, awaiting it. */
FrameReceived,      /**< Requested frame arrived. */
ReadConfig,         /**< Reading the remote JSON config. */
ConfigLoaded,       /**< Remote config loaded successfully. */
waitingConfirm,     /**< Write frame sent, awaiting confirmation. */
Error               /**< Error state (retry / recoverable). */
};
#pragma pack(pop)

/**
 * @brief CAN bus driver for the LightHub distributed-control protocol.
 *
 * Drives a 29-bit extended-ID CAN bus at 50 kbit/s and implements the
 * LightHub CAN protocol: device discovery (MAC lookup to resolve the
 * local controllerId), item command/status exchange, JSON config
 * transfer, metrics and system commands. It keeps the local and remote
 * CAN configuration objects from the JSON config and runs a small state
 * machine (see canState) advanced by Poll(). A single instance (LHCAN)
 * is used by the whole controller.
 */
class canDriver 
{
public:
/** @brief Default constructor: driver not ready, unknown state. */
canDriver(){ready=false; controllerId=0; responseTimer=0; state=canState::stateUnknown;canConfigObj=NULL;canRemoteConfigObj=NULL;confCRC=0xFFFF;};
/**
 * @brief Returns the local controller ID from the CAN config ("addr").
 * @return Controller ID, or 0 if not configured.
 */
uint8_t getMyId();
/**
 * @brief Sends the status of a local item to the local controller ID.
 * @param itemNum Item number to report.
 * @param cmd Command/parameters to report.
 * @param subItem Sub-item index, or NO_SUBITEM.
 * @return true if the frame was sent.
 */
bool sendStatus(uint16_t itemNum, itemCmd cmd, int subItem = NO_SUBITEM);
/**
 * @brief Sends an item command (or status) to a remote CAN device.
 * @param devID Target device ID.
 * @param itemID Target item ID.
 * @param cmd Command/parameters to send.
 * @param status true for a status report, false for a command.
 * @param subItemID Sub-item index, or NO_SUBITEM.
 * @return true if the frame was sent.
 */
bool sendCommand(uint8_t devID, uint16_t itemID, itemCmd cmd, bool status=false, int subItemID=NO_SUBITEM );
/**
 * @brief Sends a command described by a config array
 * [devID, itemID, suffix, subItem].
 * @param can Config array describing the target and sub-item.
 * @param cmd Command/parameters to send.
 * @param status true for a status report, false for a command.
 * @return true if the frame was sent.
 */
bool sendCommand(aJsonObject * can,itemCmd cmd, bool status = false);
/**
 * @brief Broadcasts the uptime metric over the CAN bus.
 * @param ut Uptime value to send.
 * @return true if the frame was sent.
 */
bool upTime(uint32_t ut);
/**
 * @brief Broadcasts the salt metric over the CAN bus.
 * @param salt Salt value to send.
 * @return true if the frame was sent.
 */
bool salt(uint32_t salt);
/**
 * @brief Broadcasts a lookupMAC request with the local MAC and waits
 * for a response carrying the controller ID.
 * @return true if the request frame was sent.
 */
bool lookupMAC();
/**
 * @brief Requests a frame of the given type/sequence from a device.
 * @param devId Target device ID.
 * @param _payloadType Payload type to request.
 * @param seqNo Frame sequence number.
 * @return true if the request frame was sent.
 */
bool requestFrame(uint8_t devId, payloadType _payloadType, uint16_t seqNo );
/**
 * @brief Reads one frame from the CAN bus into RXid/RXpacket/RXlen.
 * @return Frame length in bytes, or -1 if no frame is available.
 */
int  readFrame();
/**
 * @brief Responds to a lookupMAC request with the device ID found by MAC.
 * @param mac MAC address of the queried device.
 * @return true if the response frame was sent.
 */
bool sendRemoteID(macAddress mac);
/**
 * @brief Initializes the CAN hardware (50 kbit/s, extended IDs) and
 * loads the local/remote CAN config objects from the JSON root.
 * @return true on success.
 */
bool begin();
/**
 * @brief One driver iteration: reads pending frames, processes them and
 * advances the state machine (timeouts, config load).
 */
void Poll();
/**
 * @brief Processes one received CAN frame (request or response).
 * @param id Frame identifier.
 * @param packet Pointer to the received payload.
 * @param len Payload length in bytes.
 * @param rtr true if it was a remote-transmission request.
 * @return true if the frame was handled.
 */
bool processPacket(canid_t id, datagram_t *packet, uint8_t len, bool rtr=false);
/**
 * @brief Writes one extended-ID frame to the CAN bus.
 * @param msg_id 29-bit extended identifier.
 * @param buf Payload to send, or NULL for a dataless frame.
 * @param size Payload length (max 8).
 * @return true if the frame was sent.
 */
bool write(uint32_t msg_id, datagram_t * buf = NULL, uint8_t size=0);    
/**
 * @brief Finds a remote device config object by its "name".
 * @param devName Device name to search for.
 * @param devAddr Optional output for the device address.
 * @return Config object pointer, or NULL if not found.
 */
aJsonObject * findConfbyName(char* devName, int * devAddr=NULL);
#if not defined (NOIP)   
/**
 * @brief Subscribes to the MQTT topics of all remote CAN devices
 * named in the remote config.
 * @param root Topic prefix buffer to extend.
 * @param buflen Length of the buffer.
 * @return true on success.
 */
bool subscribeTopics(char * root, size_t buflen);
#endif

/**
 * @brief Returns the local controller ID.
 * @return Controller ID assigned by the CAN gateway.
 */
uint8_t getControllerID(){return controllerId;};    
/**
 * @brief Resolves a device ID from its MAC address in the remote config.
 * @param mac MAC address to look up.
 * @return Device ID, or 0 if not found.
 */
uint8_t getIdByMac(macAddress mac);
/** @brief Local CAN section of the JSON config ("can"). */
aJsonObject * canConfigObj;
/** @brief Remote controllers' CAN configs ("can.conf"). */
aJsonObject * canRemoteConfigObj;
/** @brief CRC16 of the current JSON config. */
uint16_t confCRC;
    /** @brief Payload of the last received frame. */
    datagram_t RXpacket;
    /** @brief Identifier of the last received frame. */
    canid_t RXid;
    /** @brief Length of the last received frame. */
    uint8_t RXlen;

private:
   /**
    * @brief Finds a remote device config object by its device ID.
    * @param devId Device ID to look up.
    * @return Config object pointer, or NULL if not found.
    */
   aJsonObject * getConfbyID(uint8_t devId);

    #if defined(ARDUINO_ARCH_STM32)
    CAN_message_t CAN_RX_msg;
    CAN_message_t CAN_TX_msg;
    #endif

    #if defined(__SAM3X8E__)
    //CAN_FRAME CAN_RX_msg;
    #endif

    /** @brief True once the CAN hardware is initialized. */
    bool ready;

    /** @brief Local controller ID resolved on the bus. */
    uint8_t controllerId;
    /** @brief Current driver/stream state machine state. */
    canState state;
    /** @brief millis() timestamp of the last response awaited. */
    uint32_t responseTimer;

};



/** @brief MQTT topics tree shared with the CAN driver. */
extern aJsonObject * topics;

/**
 * @brief Stream-over-CAN: reads/writes a byte stream as a sequence of
 * 8-byte CAN frames to/from a remote controller.
 *
 * Used to transfer JSON config (and other payloads) reliably over the
 * 50 kbit/s CAN bus. The sequence number starts at 0xFFFF (first frame)
 * and then increments; writes buffer 8 bytes and wait for a
 * confirmation, reads request frames and wait with timeout/retry.
 * Behaves as an Arduino Stream (read/peek/available) and Print (write).
 */
class canStream : public Stream
{
public:
    /**
     * @brief Constructs a stream bound to the given driver.
     * @param _driver Driver used to send/receive the frames.
     */
    canStream(canDriver * _driver) : readPos(0),writePos(0),devId(0), pType(payloadType::unknown),state(canState::stateUnknown),seqNo(0),failedCount(0){driver=_driver; }
    /**
     * @brief Opens the stream to a remote controller in the given mode.
     * @param controllerID Target device ID.
     * @param _pType Payload type for the frames.
     * @param _mode 'r' to read, 'w' to write.
     * @return 1 on success.
     */
    int open(uint8_t controllerID, payloadType _pType, char _mode) 
                {  
                    if (mode) close();
                    devId=controllerID; 
                    pType = _pType; 
                    mode = _mode;
                    seqNo=0xFFFF;
                    failedCount=0;
                    if (mode == 'w') state=canState::StreamOpenedWrite;
                        else state=canState::StreamOpenedRead;
                    return 1;
                    }; 
    /**
     * @brief Closes the stream, flushing any pending write data.
     * @return 1 on success.
     */
    int close ()
                {
                    if ((mode == 'w') && writePos) flush();
                    mode = '\0';
                    state=canState::stateUnknown;
                    return 1;
                }


    // Stream methods
    /**
     * @brief Number of bytes available for reading.
     * @return Bytes ready in the current frame, or -1 on error.
     */
    virtual int available();
    /**
     * @brief Reads one byte from the stream.
     * @return Byte read, or -1 on error.
     */
    virtual int read();
    /**
     * @brief Peeks at the next byte without consuming it.
     * @return Next byte, or -1 on error.
     */
    virtual int peek();

    /**
     * @brief Sends the partially filled write buffer as a frame.
     */
    virtual void flush();
    // Print methods
    /**
     * @brief Writes one byte; sends a frame when the buffer is full.
     * @param c Byte to write.
     * @return 1 on success, -1 on error.
     */
    virtual size_t write(uint8_t c) ;
    /**
     * @brief Checks if the stream accepts writes right now.
     * @return 1 if writable, 0 while waiting for confirmation.
     */
    virtual int availableForWrite();
    


private:
    /**
     * @brief Sends one buffered frame with the given length/sequence.
     * @param len Payload length to send (max 8).
     * @param _seqNo Frame sequence number.
     * @return 1 on success, 0 on failure.
     */
    int send(uint8_t len, uint16_t _seqNo);
    /**
     * @brief Advances the stream state machine (request frames, wait
     * for confirmations, time out).
     * @return Bytes available for reading, or -1 if none yet.
     */
    int checkState();
    /** @brief Driver used for the underlying CAN I/O. */
    canDriver * driver;
    /** @brief Read offset within the current frame. */
    unsigned int readPos;
    /** @brief Write offset within the current frame buffer. */
    unsigned int writePos;

    /** @brief Write buffer for the in-progress frame. */
    datagram_t writeBuffer;

    /** @brief Target device ID. */
    uint8_t devId;
    /** @brief Current frame sequence number (starts at 0xFFFF). */
    uint16_t seqNo;
    /** @brief Consecutive frame failures before the stream errors out. */
    int8_t failedCount;
    /** @brief Access mode ('\0', 'r' or 'w'). */
    char mode;
    /** @brief Payload type of the frames. */
    payloadType pType;   
    /** @brief Current stream state. */
    canState state; 
    //bool writeBlocked;
};






#endif // 