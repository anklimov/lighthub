#pragma once
#if defined(ESP8266) || defined(ESP32)
#include "FS.h"
#endif

#include <Arduino.h>
#include "flashstream.h"
#include <IPAddress.h>

#define MAXFLASHSTR 32
#define PWDFLASHSTR 16
#define EEPROM_SIGNATURE "LHCF"
#define EEPROM_SIGNATURE_LENGTH 4
const char EEPROM_signature[] = EEPROM_SIGNATURE;

#define SYSCONF_OFFSET 0

#define EEPROM_offset_NotAlligned SYSCONF_OFFSET+sizeof(systemConfigData)
#define EEPROM_offsetJSON EEPROM_offset_NotAlligned + (4 -(EEPROM_offset_NotAlligned & 3))
//#define EEPROM_offsetJSON IFLASH_PAGE_SIZE
#define EEPROM_FIX_PART_LEN EEPROM_offsetJSON-SYSCONF_OFFSET

 typedef char flashstr[MAXFLASHSTR];
 typedef char flashpwd[PWDFLASHSTR];
 typedef uint8_t macAddress[6];

 #pragma pack(push, 1)
 typedef struct
      { 
        char    signature[4];  
        macAddress  mac;
        uint32_t ip;
        uint32_t dns;
        uint32_t gw;
        uint32_t mask;
        union {
                uint8_t configFlags;
                struct
                      { uint8_t  notGetConfigFromHTTP:1;
                        uint8_t  saveToFlash:1;
                      };
              };  
        flashstr configURL;
        flashpwd MQTTpwd;
        flashpwd OTApwd;
        uint8_t  serialDebugLevel; 
        uint16_t sysConfigHash;
        uint16_t JSONHash;
            
      } systemConfigData;
 #pragma (pop)     

class systemConfig {
 private:
 flashStream * stream;
 public:  
 macAddress mac;
 systemConfig() {stream=NULL;};
 systemConfig(flashStream * fs){stream=fs;};
 //systemConfigData data;
 bool             isValidSysConf();
 //bool             isValidJSON();
 bool             getMAC();
 //inline macAddress *     getMAC() {return &mac;};

 bool             getMQTTpwd(char * buffer, uint16_t bufLen);
 bool             setMQTTpwd(char * pwd = NULL);
 bool             setMAC(macAddress mac);
 bool             setServer(char* url);
 bool             getServer(char* url);  

 bool             getIP(IPAddress& ip);
 bool             getMask(IPAddress& mask);
 bool             getDNS(IPAddress& dns);
 bool             getGW(IPAddress& gw);

 bool             setIP(IPAddress& ip);
 bool             setMask(IPAddress& mask);
 bool             setDNS(IPAddress& dns);
 bool             setGW(IPAddress& gw);

 void             clear();
 bool             getSaveSuccedConfig();
 bool             getLoadHTTPConfig();
 //bool             Save();  
};