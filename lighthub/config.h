#pragma once
#if defined(ESP8266) || defined(ESP32)
#include "FS.h"
#endif

#include <Arduino.h>
#include "flashstream.h"
#include <IPAddress.h>
#include "systemconfigdata.h"
    

class systemConfig {
 private:
 flashStream * stream;
 int openStream(char mode = '\0'); 
  
 public:  
 macAddress mac;
 systemConfig() {stream=NULL;};
 systemConfig(flashStream * fs){stream=fs;};
 
 bool             isValidSysConf();
 
 bool             getMAC();
 String           getMACString();
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
 String           getETAG();
 bool             setETAG(String etag); 

 //bool             Save();  
};