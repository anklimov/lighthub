#include "config.h"
#include "main.h"


String             systemConfig::getMACString()
{
String res;   
for (int i=0; i<6; i++) {res+= ((String(mac[i]>>4,HEX)));res+=((String(mac[i]&0xf,HEX)));}
return res;
}

int systemConfig::openStream(char mode) 
      {
            #if defined(FS_STORAGE)
            return stream->open("/config.bin",mode);
            #else
            return stream->open(FN_CONFIG_BIN,mode);
            #endif
            //stream->setSize(SYSCONF_SIZE);
        };

bool             systemConfig::isValidSysConf()
{    
     if (!stream) return false; 
     openStream('r');
     stream->seek(offsetof(systemConfigData,signature));
     for (int i=0;i<sizeof(systemConfigData::signature);i++)
        if (stream->read()!=EEPROM_signature[i])
                      {
                       stream->close();  
                       return false;
                       } 
    stream->close();                    
    return true;                   
}; 


  bool             systemConfig::getMAC()
 { 
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,mac));

    bool isMacValid = false; 
     for (short i = 0; i < 6; i++) {
        mac[i] = stream->read();
        if (mac[i] != 0 && mac[i] != 0xff) isMacValid = true;
    }
   stream->close();  
   return isMacValid;
 }
 
  bool             systemConfig::setMAC(macAddress& _mac)
 {
   if (!stream || !isValidSysConf()) return false;    
   openStream('a');
   stream->seek(offsetof(systemConfigData,mac));
   stream->write ((const uint8_t *)&_mac,sizeof(_mac));
   memcpy(mac, _mac, sizeof(mac));
   stream->close();
  return true;
 }

  char *             systemConfig::getMQTTpwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL;
    openStream('r');
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
    short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    stream->close(); 

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
    openStream('r'); 
    stream->seek(offsetof(systemConfigData,MQTTpwd));   
    stream->print(pwd); 
    int bytes = stream->write((uint8_t)'\0');
    stream->close();  
    return bytes;
 }


  char *             systemConfig::getOTApwd(char * buffer, uint16_t bufLen)
 {
    if (!stream || !isValidSysConf()) return NULL; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,OTApwd));   
    short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    stream->close(); 
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
    openStream('r'); 
    stream->seek(offsetof(systemConfigData,OTApwd));   
    stream->print(pwd);   
    int bytes = stream->write((uint8_t)'\0');
    stream->close();  
    return bytes;
 }
 

  char *          systemConfig::getServer(char * buffer, uint16_t bufLen)
 {
  if (!stream || !isValidSysConf()) return NULL; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,configURL));   
    short bytes=stream->readBytesUntil(0,buffer,bufLen-1);
    stream->close(); 
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
    openStream('r');
    stream->seek(offsetof(systemConfigData,configURL));   
    stream->print(url);   
    int bytes = stream->write((uint8_t)'\0');
    stream->close();  
    return bytes;
 }
  

 bool             systemConfig::getIP(IPAddress& ip)
 {
    uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,ip));   
    stream->readBytes((uint8_t *) &addr,4);
    ip=addr;
    stream->close(); 
    return (ip[0] && ((ip[0] != 0xff) || (ip[1] != 0xff) || (ip[2] != 0xff) || (ip[3] != 0xff)));
 }
 
 bool             systemConfig::getMask(IPAddress& mask)
 {
     uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,mask));   
    stream->readBytes((uint8_t *) &addr,4);
    mask=addr;
    stream->close(); 
    return (mask[0] && ((mask[0] != 0xff) || (mask[1] != 0xff) || (mask[2] != 0xff) || (mask[3] != 0xff)));
 }
 
 bool             systemConfig::getDNS(IPAddress& dns)
 {   uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,dns));   
    stream->readBytes((uint8_t *) &addr,4);
    dns = addr;
    stream->close(); 
    return (dns[0] && ((dns[0] != 0xff) || (dns[1] != 0xff) || (dns[2] != 0xff) || (dns[3] != 0xff)));
 }
 
 bool             systemConfig::getGW(IPAddress& gw)
 {   uint32_t addr;
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,gw));   
    stream->readBytes((uint8_t *) &addr,4);
    gw=addr;
    stream->close(); 
    return (gw[0] && ((gw[0] != 0xff) || (gw[1] != 0xff) || (gw[2] != 0xff) || (gw[3] != 0xff)));  
}
 

 bool             systemConfig::setIP(IPAddress& ip)
 {  uint32_t addr=ip;
  if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,ip));
    int bytes = stream->write((uint8_t *) &addr, 4);
    stream->close();  
    return bytes;      
 }
 
 bool             systemConfig::setMask(IPAddress& mask)
 {  uint32_t addr = mask;
  if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,mask));   
    int bytes = stream->write((uint8_t *) &addr, 4);
    stream->close();  
    return bytes;    
 }
 
 bool             systemConfig::setDNS(IPAddress& dns)
 {  uint32_t addr = dns;
  if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,dns));   
    int bytes = stream->write((uint8_t *) &addr, 4);
    stream->close();  
    return bytes;   

 }
 
 bool             systemConfig::setGW(IPAddress& gw)
 { uint32_t addr = gw;
  if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,gw));   
    int bytes = stream->write((uint8_t *) &addr, 4);
    stream->close();  
    return bytes;   
 }
 

 bool             systemConfig::clear()
 {
   if (!stream) return false; 
    openStream('w');
    stream->seek(0);
     for (unsigned int i = 0; i < stream->getSize(); i++) {
        stream->write((uint8_t)'\0');
    }
     stream->seek(offsetof(systemConfigData,signature));
     for (unsigned int i=0;i<sizeof(systemConfigData::signature);i++)
        if (stream->write(EEPROM_signature[i]));
    stream->close();
    setETAG("");
    setSerialDebuglevel(7);
    setUdpDebuglevel(7);
    return true;
 }
 
systemConfigFlags systemConfig::getConfigFlags()
{   
   systemConfigFlags flags;
   flags.configFlags32bit=0;
   flags.serialDebugLevel=7;
   flags.udpDebugLevel=7;
   
    if (stream && isValidSysConf()) 
    { 
      openStream('r');
      stream->seek(offsetof(systemConfigData,configFlags));  
      stream->readBytes((uint8_t *) &flags,sizeof (flags));
      stream->close();
    } 
    return flags;

}

bool systemConfig::setConfigFlags(systemConfigFlags flags)
{
    if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,configFlags));   

    int bytes = stream->write((uint8_t *) &flags, sizeof (flags));
    stream->close();  
    return bytes;
   
}

 bool             systemConfig::getSaveSuccedConfig()
 {
  systemConfigFlags flags = getConfigFlags();
  return !flags.notSaveSuccedConfig;
 }

  bool             systemConfig::setSaveSuccedConfig(bool flag)
 {
  systemConfigFlags flags = getConfigFlags();
  flags.notSaveSuccedConfig=!flag;   
  return setConfigFlags(flags);
 }

bool             systemConfig::getDHCPfallback()
{
  systemConfigFlags flags = getConfigFlags();
  return flags.dhcpFallback;
}

bool             systemConfig::setDHCPfallback(bool flag)
 {
  systemConfigFlags flags = getConfigFlags();
  flags.dhcpFallback=flag;   
  return setConfigFlags(flags);
 } 

bool             systemConfig::getLoadHTTPConfig()
{
  systemConfigFlags flags = getConfigFlags();
  return !flags.notGetConfigFromHTTP;
}

bool             systemConfig::setLoadHTTPConfig(bool load)
{
  systemConfigFlags flags = getConfigFlags();
  flags.notGetConfigFromHTTP=!load;   
  return setConfigFlags(flags);
}

 
bool             systemConfig::setSerialDebuglevel(short level)
{
  systemConfigFlags flags = getConfigFlags();
  flags.serialDebugLevel=level;   
  return setConfigFlags(flags);
}

bool             systemConfig::setUdpDebuglevel(short level)
{
  systemConfigFlags flags = getConfigFlags();
  flags.udpDebugLevel=level;   
  return setConfigFlags(flags);
}


uint8_t           systemConfig::getSerialDebuglevel()
{
  systemConfigFlags flags = getConfigFlags();
  return flags.serialDebugLevel;   
}

uint8_t           systemConfig::getUdpDebuglevel()
{
  systemConfigFlags flags = getConfigFlags();
  return flags.udpDebugLevel;    
}

 String           systemConfig::getETAG()
{
debugSerial<<F("Get ETAG: ")<<currentConfigETAG<<endl;     
return String("\"")+currentConfigETAG+String("\"");
}

bool             systemConfig::setETAG(String etag)
{
int firstPos = etag.indexOf('"');
int lastPos =  etag.lastIndexOf('"');

if ((firstPos>=0) && (lastPos>0)) currentConfigETAG=etag.substring(firstPos+1,lastPos);
   else currentConfigETAG=etag;   
debugSerial<<F("Set ETAG: ")<<currentConfigETAG<<endl;   
return 1;
}

bool             systemConfig::saveETAG()
{
  if (!stream || !isValidSysConf() || (currentConfigETAG.length()>=sizeof(systemConfigData::ETAG))) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,ETAG));   
    stream->print(currentConfigETAG);   
    int bytes = stream->write((uint8_t)'\0');
    stream->close();  
    if (bytes) debugSerial<<F("Saved ETAG:")<<currentConfigETAG<<endl;   
    return bytes;   
}

bool             systemConfig::loadETAG()
{
  if (!stream || !isValidSysConf()) return false; 
    openStream('r');
    stream->seek(offsetof(systemConfigData,ETAG));   
    currentConfigETAG=stream->readStringUntil(0);
    stream->close(); 
    debugSerial<<F("Loaded ETAG:")<<currentConfigETAG<<endl;   
    return currentConfigETAG.length();
}