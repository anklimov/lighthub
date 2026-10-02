/**
 * @file systemconfigdata.h
 * @brief Fixed-layout binary system configuration block stored in flash.
 *
 * Defines the on-flash layout (systemConfigData) that the systemConfig
 * manager reads/writes: signature, MAC, config flags, static network
 * addresses, config server URL, passwords and the remote-config ETag.
 */
#pragma once
/** @brief Offset of the system config block in the storage. */
#define SYSCONF_OFFSET 0
/** @brief End offset of the fixed (unaligned) config block. */
#define EEPROM_offset_NotAlligned SYSCONF_OFFSET+sizeof(systemConfigData)
/** @brief Total size of the fixed config area. */
#define SYSCONF_SIZE EEPROM_offsetJSON
/** @brief 4-byte aligned offset where the JSON config storage starts. */
#define EEPROM_offsetJSON EEPROM_offset_NotAlligned + (4 -(EEPROM_offset_NotAlligned & 3))

/** @brief Max length of flash-stored strings (URLs, ETag). */
#define MAXFLASHSTR 32
/** @brief Max length of flash-stored passwords. */
#define PWDFLASHSTR 16
/** @brief Signature marking a valid config block. */
#define EEPROM_SIGNATURE "LHC3"
/** @brief Length of the config block signature. */
#define EEPROM_SIGNATURE_LENGTH 4

//#define EEPROM_offsetJSON IFLASH_PAGE_SIZE
#define EEPROM_FIX_PART_LEN EEPROM_offsetJSON-SYSCONF_OFFSET

const char EEPROM_signature[] = EEPROM_SIGNATURE;

   /** @brief Fixed-size flash string buffer. */
 typedef char flashstr[MAXFLASHSTR];
 /** @brief Fixed-size flash password buffer. */
 typedef char flashpwd[PWDFLASHSTR];
 /** @brief 6-byte MAC address. */
 typedef uint8_t macAddress[6];

 #pragma pack(push, 1)
 /** @brief Packed 32-bit system configuration flags. */
 typedef union
 {
                uint32_t configFlags32bit;
                struct
                      { 
                        uint8_t  serialDebugLevel:4; 
                        uint8_t  udpDebugLevel:4;   

                        uint8_t  notGetConfigFromHTTP:1;                
                        uint8_t  notSaveSuccedConfig:1;
                        uint8_t  dhcpFallback:1;
                        uint8_t  spare2:5;

                        uint16_t sysConfigHash;
                      };      
 } systemConfigFlags;
 
 /** @brief Fixed-layout system configuration block persisted in flash. */
 typedef struct
      { 
        char    signature[4];  
        macAddress  mac;  //6 bytes
        uint16_t spare; //2 bytes
        systemConfigFlags configFlags; //4 bytes
        uint32_t ip;
        uint32_t dns;
        uint32_t gw;
        uint32_t mask;

        flashstr configURL;
        flashpwd MQTTpwd;
        flashpwd OTApwd; 
        flashstr ETAG;          
      } systemConfigData;
 #pragma (pop) 