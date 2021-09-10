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
        macAddress  mac;  //6 bytes
        union {
                uint16_t configFlags;
                struct
                      { 
                        uint8_t  serialDebugLevel:4; 
                        uint8_t  syslogDebugLevel:4; 
                        uint8_t  notGetConfigFromHTTP:1;
                        uint8_t  saveToFlash:1;
                      };
              };  
        uint32_t ip;
        uint32_t dns;
        uint32_t gw;
        uint32_t mask;

        flashstr configURL;
        flashpwd MQTTpwd;
        flashpwd OTApwd;

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
 
 bool             isValidSysConf();
 
 bool             getMAC();
 bool             setMAC(macAddress& mac);

 char *           getMQTTpwd(char * buffer, uint16_t bufLen);
 bool             setMQTTpwd(char * pwd = NULL);

 char *           getOTApwd(char * buffer, uint16_t bufLen);
 bool             setOTApwd(char * pwd = NULL);

 bool             setServer(char* url);
 char *           getServer(char * buffer, uint16_t bufLen);  

 bool             getIP(IPAddress& ip);
 bool             getMask(IPAddress& mask);
 bool             getDNS(IPAddress& dns);
 bool             getGW(IPAddress& gw);

 bool             setIP(IPAddress& ip);
 bool             setMask(IPAddress& mask);
 bool             setDNS(IPAddress& dns);
 bool             setGW(IPAddress& gw);

 bool             setSerialDebuglevel(short);
 bool             setUdpDebuglevel(short);
 uint8_t          getSerialDebuglevel();
 uint8_t          getUdpDebuglevel();

 bool             clear();
 bool             getSaveSuccedConfig();
 bool             setSaveSuccedConfig(bool);
 bool             getLoadHTTPConfig();
 bool             setLoadHTTPConfig(bool);
 //bool             Save();  
};