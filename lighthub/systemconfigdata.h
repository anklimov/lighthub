#define SYSCONF_OFFSET 0
#define EEPROM_offset_NotAlligned SYSCONF_OFFSET+sizeof(systemConfigData)
#define SYSCONF_SIZE EEPROM_offsetJSON
#define EEPROM_offsetJSON EEPROM_offset_NotAlligned + (4 -(EEPROM_offset_NotAlligned & 3))

#define MAXFLASHSTR 32
#define PWDFLASHSTR 16
#define EEPROM_SIGNATURE "LHCF"
#define EEPROM_SIGNATURE_LENGTH 4

//#define EEPROM_offsetJSON IFLASH_PAGE_SIZE
#define EEPROM_FIX_PART_LEN EEPROM_offsetJSON-SYSCONF_OFFSET

const char EEPROM_signature[] = EEPROM_SIGNATURE;

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