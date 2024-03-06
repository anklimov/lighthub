#pragma once
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

typedef uint8_t macAddress[6];
#pragma pack(push, 1)
typedef union 
{
  uint32_t id;
  struct 
  {
      uint16_t itemId; 
      uint8_t deviceId;
      uint8_t payloadType:4;
      uint8_t status:1; 
      uint8_t reserve:3; //0

  };
} canid_t;

enum payloadType
{   unknown=0,
    itemCommand=1,
    lookupMAC=2,
    configFrame=3,
    OTAFrame=4,
    auth=5,
    metric=6,
    sysCmd=7
};

enum metricType
{
    MAC=1,
    IP=2,
    NetMask=3,
    GW=4,
    DNS=5,
    UpTime=6,
    Salt=7
};

enum commandType
{
    reboot=1,
    get=2,
    save=3,
    load=4
};


#define MAXCANID 0x1FFFFFFF
// Request item status: id.status=1;deviceId=[0xFF | deviceId];itemId=x; RTR bit=true 
// Request on config: id.status=0;deviceId=0;itemId=x payload.mac=mac;crc16=crc_current_config 



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


enum canState
{
Unknown,
MACLookup,
Idle,
StreamOpenedWrite,
StreamOpenedRead,
FrameRequested,
FrameReceived,
ReadConfig,
ConfigLoaded,
waitingConfirm,
Error
};
#pragma pack(pop)

class canDriver 
{
public:
canDriver(){ready=false; controllerId=0; responseTimer=0; state=canState::Unknown;};
uint8_t getMyId();
bool sendStatus(uint8_t itemNum, itemCmd cmd);
bool sendCommand(uint8_t devID, uint16_t itemID, itemCmd cmd, bool status=false);
bool sendCommand(aJsonObject * can,itemCmd cmd, bool status = false);
bool upTime(uint32_t ut);
bool salt(uint32_t salt);
bool lookupMAC();
bool requestFrame(uint8_t devId, payloadType _payloadType );
int  readFrame();
bool sendRemoteID(macAddress mac);
bool begin();
void Poll();
bool processPacket(canid_t id, datagram_t *packet, uint8_t len, bool rtr=false);
bool write(uint32_t msg_id, datagram_t * buf = NULL, uint8_t size=0);      
uint8_t getControllerID(){return controllerId;};    
uint8_t getIdByMac(macAddress mac);

    datagram_t RXpacket;
    canid_t RXid;
    uint8_t RXlen;

private:
   aJsonObject * findConfbyID(uint8_t devId);

    #if defined(ARDUINO_ARCH_STM32)
    CAN_message_t CAN_RX_msg;
    CAN_message_t CAN_TX_msg;
    #endif
    bool ready;
    


    uint8_t controllerId;
    canState state;
    uint32_t responseTimer;

};



extern aJsonObject * topics;

class canStream : public Stream
{
public:
    canStream(canDriver * _driver) : readPos(0),writePos(0),devId(0), pType(payloadType::unknown),state(canState::Unknown){driver=_driver; }
    int open(uint8_t controllerID, payloadType _pType, char _mode) 
                {  
                    if (mode) close();
                    devId=controllerID; 
                    pType = _pType; 
                    mode = _mode;
                    if (mode == 'w') state=canState::StreamOpenedWrite;
                        else state=canState::StreamOpenedRead;
                    return 1;
                    }; 
    int close ()
                {
                    if ((mode == 'w') && writePos) flush();
                    mode = '\0';
                    state=canState::Unknown;
                    return 1;
                }


    // Stream methods
    virtual int available();
    virtual int read();
    virtual int peek();

    virtual void flush();
    // Print methods
    virtual size_t write(uint8_t c) ;
    virtual int availableForWrite();
    


private:
    int send(uint8_t len);
    int checkState();
    canDriver * driver;
    unsigned int readPos;
    unsigned int writePos;

    datagram_t writeBuffer;

    uint8_t devId;
    char mode;
    payloadType pType;   
    canState state; 
    //bool writeBlocked;
};






#endif // 