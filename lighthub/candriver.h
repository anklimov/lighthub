#pragma once
#ifdef  CANDRV

#if defined(ARDUINO_ARCH_STM32)
#if !defined(HAL_CAN_MODULE_ENABLED)
#define HAL_CAN_MODULE_ENABLED
#endif
#include <STM32_CAN.h>
#endif

#include <itemCmd.h>
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
{
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
Unknown=0,
MACLookup=2,
HaveId=3,
ConfigFrameRequested=4,
ConfigFrameReceived=5,
ConfigLoaded=6,
Error=7
};
#pragma pack(pop)

class canDriver 
{
public:
canDriver(){ready=false; controllerId=0; responseTimer=0; state=canState::Unknown;};
bool upTime(uint32_t ut);
bool salt(uint32_t salt);
bool lookupMAC();
bool sendRemoteID(macAddress mac);
bool begin();
void Poll();
bool processPacket(uint32_t rawid, datagram_t *packet, uint8_t len, bool rtr=false);
bool write(uint32_t msg_id, datagram_t * buf = NULL, uint8_t size=0);          
private:


    #if defined(ARDUINO_ARCH_STM32)
    CAN_message_t CAN_RX_msg;
    CAN_message_t CAN_TX_msg;
    #endif
    bool ready;
    uint8_t controllerId;
    canState state;
    uint32_t responseTimer;

};
#endif // 