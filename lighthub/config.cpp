#include "config.h"


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
   stream->write ((const uint8_t *)&_mac,sizeof(_mac));
   memcpy(mac, _mac, sizeof(mac));

  return true;
 }

  char *             systemConfig::getMQTTpwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
     short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    if (bytes) 
            {
            buffer[bytes]=0;  
            return buffer;
            }
    return NULL;
 }
 
 bool             systemConfig::setMQTTpwd(char * pwd)
 {
    if (!stream || !isValidSysConf() || (strlen(pwd)>=sizeof(systemConfigData::MQTTpwd))) return false; 
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
    stream->print(pwd);   
    return stream->write((uint8_t)'\0');
 }


  char *             systemConfig::getOTApwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    if (bytes) 
            {
            buffer[bytes]=0;  
            return buffer;
            }
    return NULL;
 }
 
 bool             systemConfig::setOTApwd(char * pwd)
 {
     if (!stream || !isValidSysConf() || (strlen(pwd)>=sizeof(systemConfigData::OTApwd))) return false; 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    stream->print(pwd);   
    return stream->write((uint8_t)'\0');
 }
 

  char *          systemConfig::getServer(char * buffer, uint16_t bufLen)
 {
  if (!stream || !isValidSysConf()) return NULL; 
    stream->seek(offsetof(systemConfigData,configURL));   
    short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    if (bytes) 
            {
            buffer[bytes]=0;  
            return buffer;
            }
    return NULL;
 }
 
 bool             systemConfig::setServer(char* url)
 {
  if (!stream || !isValidSysConf() || (strlen(url)>=sizeof(systemConfigData::configURL))) return false; 
    stream->seek(offsetof(systemConfigData,configURL));   
    stream->print(url);   
    return stream->write((uint8_t)'\0');
 }
  

 bool             systemConfig::getIP(IPAddress& ip)
 {
    uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,ip));   
    stream->readBytes((uint8_t *) &addr,4);
    ip=addr;
    return (ip[0] && ((ip[0] != 0xff) || (ip[1] != 0xff) || (ip[2] != 0xff) || (ip[3] != 0xff)));
 }
 
 bool             systemConfig::getMask(IPAddress& mask)
 {
     uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,mask));   
    stream->readBytes((uint8_t *) &addr,4);
    mask=addr;
    return (mask[0] && ((mask[0] != 0xff) || (mask[1] != 0xff) || (mask[2] != 0xff) || (mask[3] != 0xff)));
 }
 
 bool             systemConfig::getDNS(IPAddress& dns)
 {   uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,dns));   
    stream->readBytes((uint8_t *) &addr,4);
    dns = addr;
    return (dns[0] && ((dns[0] != 0xff) || (dns[1] != 0xff) || (dns[2] != 0xff) || (dns[3] != 0xff)));
 }
 
 bool             systemConfig::getGW(IPAddress& gw)
 {   uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,gw));   
    stream->readBytes((uint8_t *) &addr,4);
    gw=addr;
    return (gw[0] && ((gw[0] != 0xff) || (gw[1] != 0xff) || (gw[2] != 0xff) || (gw[3] != 0xff)));  
}
 

 bool             systemConfig::setIP(IPAddress& ip)
 {  uint32_t addr=ip;
  if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,ip));   
    return stream->write((uint8_t *) &addr, 4);   
 }
 
 bool             systemConfig::setMask(IPAddress& mask)
 {  uint32_t addr = mask;
  if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,mask));   
    return stream->write((uint8_t *) &addr, 4);   
 }
 
 bool             systemConfig::setDNS(IPAddress& dns)
 {  uint32_t addr = dns;
  if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,dns));   
    return stream->write((uint8_t *) &addr, 4);  

 }
 
 bool             systemConfig::setGW(IPAddress& gw)
 { uint32_t addr = gw;
  if (!stream || !isValidSysConf()) return false; 
    stream->seek(offsetof(systemConfigData,gw));   
    return stream->write((uint8_t *) &addr, 4);   
 }
 

 void             systemConfig::clear()
 {
   if (!stream) return ; 
    stream->seek(0);
     for (unsigned int i = 0; i < stream->getSize(); i++) {
        stream->write((uint8_t)'\0');
    }
     stream->seek(offsetof(systemConfigData,signature));
     for (unsigned int i=0;i<sizeof(systemConfigData::signature);i++)
        if (stream->write(EEPROM_signature[i]));
    stream->flush();
 }
 
///
 bool             systemConfig::getSaveSuccedConfig()
 {
    return false;
 }

  bool             systemConfig::setSaveSuccedConfig(bool)
 {
    return false;
 }

///
 
bool             systemConfig::setSerialDebuglevel(short level)
{
return false;
}

bool             systemConfig::setUdpDebuglevel(short level)
{
return false;
}


uint8_t           systemConfig::getSerialDebuglevel()
{
return 7;
}

uint8_t           systemConfig::getUdpDebuglevel()
{
return 7;   
}

//
bool             systemConfig::setLoadHTTPConfig(bool load)
{
return false;
}

bool             systemConfig::getLoadHTTPConfig()
{
return false;
}


