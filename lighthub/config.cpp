#include "config.h"

//#if defined(ARDUINO_ARCH_AVR)
//#include <EEPROM.h>
//#endif

/*
void saveFlash(short n, char *str) {
    short len=strlen(str);
    if (len>MAXFLASHSTR-1) len=MAXFLASHSTR-1;
    for(int i=0;i<len;i++) EEPROM.write(n+i,str[i]);
    EEPROM.write(n+len,0);

   #if defined(ARDUINO_ARCH_ESP8266)
  // write the data to EEPROM
   short res  = EEPROM.commitReset();
  Serial.println((res) ? "EEPROM Commit OK" : "Commit failed");
  #endif
}


int loadFlash(short n, char *str, short l) {
    short i;
    uint8_t ch = EEPROM.read(n);
    if (!ch || (ch == 0xff)) return 0;
    for (i=0;i<l-1 && (str[i] = EEPROM.read(n++));i++);
    str[i]=0;
    return 1;
}

void saveFlash(short n, IPAddress& ip) {
    for(int i=0;i<4;i++) EEPROM.write(n++,ip[i]);

    #if defined(ARDUINO_ARCH_ESP8266)
   // write the data to EEPROM
    short res  = EEPROM.commitReset();
   Serial.println((res) ? "EEPROM Commit OK" : "Commit failed");
   #endif
}

int ipLoadFromFlash(short n, IPAddress &ip) {
    for (int i = 0; i < 4; i++)
        ip[i] = EEPROM.read(n++);
    return (ip[0] && ((ip[0] != 0xff) || (ip[1] != 0xff) || (ip[2] != 0xff) || (ip[3] != 0xff)));
}
*/

bool             systemConfig::isValidSysConf()
{    
     if (!stream) return false; 
     stream->seek(offsetof(systemConfigData,signature));
     for (int i=0;i<EEPROM_SIGNATURE_LENGTH;i++)
        if (stream->read()!=EEPROM_signature[i])
                      {
                       return false;
                       } 
    return true;                   
}; 


 bool             systemConfig::getMAC()
 { 
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,mac));

    bool isMacValid = false; 
     for (short i = 0; i < 6; i++) {
        mac[i] = stream->read();
        if (mac[i] != 0 && mac[i] != 0xff) isMacValid = true;
    }
   return isMacValid;
 }
 

 bool             systemConfig::getMQTTpwd(char * buffer, uint16_t bufLen)
 {

 }
 
 bool             systemConfig::setMQTTpwd(char * pwd)
 {

 }
 
 bool             systemConfig::setMAC(macAddress mac)
 {

 }
 
 bool             systemConfig::setServer(char* url)
 {

 }
 
 bool             systemConfig::getServer(char* url)
 {

 }
  

 bool             systemConfig::getIP(IPAddress& ip)
 {

 }
 
 bool             systemConfig::getMask(IPAddress& mask)
 {

 }
 
 bool             systemConfig::getDNS(IPAddress& dns)
 {

 }
 
 bool             systemConfig::getGW(IPAddress& gw)
 {
         }
 

 bool             systemConfig::setIP(IPAddress& ip)
 {

 }
 
 bool             systemConfig::setMask(IPAddress& mask)
 {

 }
 
 bool             systemConfig::setDNS(IPAddress& dns)
 {

 }
 
 bool             systemConfig::setGW(IPAddress& gw)
 {

 }
 

 void             systemConfig::clear()
 {

     return;

   if (!stream) return ; 
    stream->seek(0);
     for (unsigned int i = 0; i < stream->getSize(); i++) {
        mac[i] = stream->write(255);
    }
 }
 
 bool             systemConfig::getSaveSuccedConfig()
 {
    return false;
 }
 
  
