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
     for (int i=0;i<sizeof(systemConfigData::signature);i++)
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
 
  bool             systemConfig::setMAC(macAddress& _mac)
 {
   if (!stream || !isValidSysConf()) return false;    
   stream->seek(offsetof(systemConfigData,mac));
   ///stream->write ((const uint8_t *)&_mac,sizeof(_mac));
   memcpy(mac, _mac, sizeof(mac));

  return true;
 }

  char *             systemConfig::getMQTTpwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
    if (stream->readBytesUntil(0,buffer,bufLen)) return buffer;
    return NULL;
 }
 
 bool             systemConfig::setMQTTpwd(char * pwd)
 {
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
    stream->print(pwd);   
    return stream->write(0);
 }


  char *             systemConfig::getOTApwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    if (stream->readBytesUntil(0,buffer,bufLen)) return buffer;
    return NULL;
 }
 
 bool             systemConfig::setOTApwd(char * pwd)
 {
     if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    stream->print(pwd);   
    return stream->write(0);
 }
 

  char *             systemConfig::getServer(char * buffer, uint16_t bufLen)
 {
  if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,configURL));   
    if (stream->readBytesUntil(0,buffer,bufLen)) return buffer;
    return NULL;
 }
 
 bool             systemConfig::setServer(char* url)
 {
  if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    stream->print(url);   
    return stream->write(0);

 }
  

 bool             systemConfig::getIP(IPAddress& ip)
 {
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,ip));   
    stream->readBytes((char *)&ip,sizeof(ip));
    return ip;

 }
 
 bool             systemConfig::getMask(IPAddress& mask)
 {
  return 0;

 }
 
 bool             systemConfig::getDNS(IPAddress& dns)
 {
  return 0;

 }
 
 bool             systemConfig::getGW(IPAddress& gw)
 {
      return 0;
  
         }
 

 bool             systemConfig::setIP(IPAddress& ip)
 {
  return 0;

 }
 
 bool             systemConfig::setMask(IPAddress& mask)
 {
  return 0;

 }
 
 bool             systemConfig::setDNS(IPAddress& dns)
 {
  return 0;

 }
 
 bool             systemConfig::setGW(IPAddress& gw)
 {
  return 0;

 }
 

 void             systemConfig::clear()
 {
   if (!stream) return ; 
    stream->seek(0);
     for (unsigned int i = 0; i < stream->getSize(); i++) {
        stream->write(0);
    }
     stream->seek(offsetof(systemConfigData,signature));
     for (unsigned int i=0;i<sizeof(systemConfigData::signature);i++)
        if (stream->write(EEPROM_signature[i]));
    stream->flush();
 }
 
 bool             systemConfig::getSaveSuccedConfig()
 {
    return false;
 }
 
  
