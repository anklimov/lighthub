#pragma once
#if defined(ESP8266) || defined(ESP32)
#include "FS.h"
#endif

#include <Arduino.h>
#include "flashstream.h"
#include <IPAddress.h>
#include "systemconfigdata.h"
    

/**
 * @brief Persistent system configuration manager.
 *
 * Reads and writes the fixed-layout system configuration block
 * (see systemConfigData) stored in flash/EEPROM through a flashStream:
 * MAC address, static network parameters, MQTT/OTA passwords, the remote
 * configuration server URL and assorted control flags. It also keeps track
 * of the ETag of the last downloaded remote config, which is sent back as
 * "If-None-Match" so that the portal server can answer 304 (not modified)
 * and skip the download. The full JSON config load/save/submit workflow
 * (HTTP download from MY_CONFIG_SERVER / CONFIG_SERVER, CRC validation,
 * save to flash) is driven from main.cpp via loadConfigFromHttp(),
 * cmdFunctionSave() and cmdFunctionLoad().
 */
class systemConfig {
 private:
 /** @brief Flash (or SPIFFS) stream that backs the config storage. */
 flashStream * stream;
 /** @brief ETag of the currently stored remote config, in memory. */
 String currentConfigETAG;
 /**
  * @brief Opens the backing stream in the given mode.
  * @param mode Stream open mode ('r', 'w', ...).
  * @return Stream open result.
  */
 int openStream(char mode = '\0'); 

 public:  
 /** @brief MAC address of the controller, kept in memory after load. */
 macAddress mac;

 /** @brief Constructs a config manager without a backing storage. */
 systemConfig():currentConfigETAG() {stream=NULL;};
 /**
  * @brief Constructs a config manager over the given flash stream.
  * @param fs Pointer to the flashStream holding the config block.
  */
 systemConfig(flashStream * fs):currentConfigETAG() {stream=fs;};

 /**
  * @brief Checks that the stored config block has a valid signature.
  * @return true if the "LHC3" signature in flash is intact.
  */
 bool             isValidSysConf();

 /**
  * @brief Loads the MAC address from flash into the mac member.
  * @return true if a non-blank MAC was read.
  */
 bool             getMAC();
 /**
  * @brief Returns the MAC address of the controller as a hex string.
  * @return MAC in "AA:BB:CC:DD:EE:FF" form.
  */
 String           getMACString();
 /**
  * @brief Stores the given MAC address into flash.
  * @param mac MAC address to write.
  * @return true on success.
  */
 bool             setMAC(macAddress& mac);

 /**
  * @brief Reads the MQTT password from flash into a caller buffer.
  * @param buffer Output buffer for the password.
  * @param bufLen Size of the buffer.
  * @return Pointer to buffer if a password is stored, NULL otherwise.
  */
 char *           getMQTTpwd(char * buffer, uint16_t bufLen);
 /**
  * @brief Stores the MQTT password in flash (NULL clears it).
  * @param pwd Null-terminated password, or NULL to erase.
  * @return true on success.
  */
 bool             setMQTTpwd(char * pwd = NULL);

 /**
  * @brief Reads the OTA (HTTP update) password from flash.
  * @param buffer Output buffer for the password.
  * @param bufLen Size of the buffer.
  * @return Pointer to buffer if a password is stored, NULL otherwise.
  */
 char *           getOTApwd(char * buffer, uint16_t bufLen);
 /**
  * @brief Stores the OTA password in flash (NULL clears it).
  * @param pwd Null-terminated password, or NULL to erase.
  * @return true on success.
  */
 bool             setOTApwd(char * pwd = NULL);

 /**
  * @brief Stores the URL of the remote configuration server in flash.
  * Falls back to the compile-time MY_CONFIG_SERVER / CONFIG_SERVER
  * when nothing is stored.
  * @param url Null-terminated server URL.
  * @return true on success.
  */
 bool             setServer(char* url);
 /**
  * @brief Reads the stored configuration server URL into a buffer.
  * @param buffer Output buffer.
  * @param bufLen Size of the buffer.
  * @return Pointer to buffer if a URL is stored, NULL otherwise.
  */
 char *           getServer(char * buffer, uint16_t bufLen);  

 /**
  * @brief Loads the static IP address from flash.
  * @param ip Output IPAddress.
  * @return true if a usable (non-blank) address was read.
  */
 bool             getIP(IPAddress& ip);
 /**
  * @brief Loads the static subnet mask from flash.
  * @param mask Output IPAddress.
  * @return true if a usable (non-blank) mask was read.
  */
 bool             getMask(IPAddress& mask);
 /**
  * @brief Loads the static DNS server address from flash.
  * @param dns Output IPAddress.
  * @return true if a usable (non-blank) address was read.
  */
 bool             getDNS(IPAddress& dns);
 /**
  * @brief Loads the static default gateway address from flash.
  * @param gw Output IPAddress.
  * @return true if a usable (non-blank) address was read.
  */
 bool             getGW(IPAddress& gw);

 /**
  * @brief Stores the static IP address in flash.
  * @param ip Address to write.
  * @return true on success.
  */
 bool             setIP(IPAddress& ip);
 /**
  * @brief Stores the static subnet mask in flash.
  * @param mask Mask to write.
  * @return true on success.
  */
 bool             setMask(IPAddress& mask);
 /**
  * @brief Stores the static DNS server address in flash.
  * @param dns Address to write.
  * @return true on success.
  */
 bool             setDNS(IPAddress& dns);
 /**
  * @brief Stores the static default gateway address in flash.
  * @param gw Address to write.
  * @return true on success.
  */
 bool             setGW(IPAddress& gw);

 /**
  * @brief Sets the serial (debug port) log level in flash.
  * @param level Log level to store.
  * @return true on success.
  */
 bool             setSerialDebuglevel(short);
 /**
  * @brief Sets the UDP (syslog) log level in flash.
  * @param level Log level to store.
  * @return true on success.
  */
 bool             setUdpDebuglevel(short);
 /**
  * @brief Returns the stored serial log level.
  * @return Current serial log level.
  */
 uint8_t          getSerialDebuglevel();
 /**
  * @brief Returns the stored UDP log level.
  * @return Current UDP log level.
  */
 uint8_t          getUdpDebuglevel();

 /**
  * @brief Wipes the whole config block and restores defaults.
  * @return true on success.
  */
 bool             clear();
 /**
  * @brief Returns whether a successfully downloaded config is auto-saved.
  * @return true if saving after HTTP load is enabled.
  */
 bool             getSaveSuccedConfig();
 /**
  * @brief Enables/disables auto-save of a successfully downloaded config.
  * @param flag true to enable auto-save.
  * @return true on success.
  */
 bool             setSaveSuccedConfig(bool);
 /**
  * @brief Returns whether the config is fetched from the portal over HTTP.
  * @return true if HTTP config loading is enabled.
  */
 bool             getLoadHTTPConfig();
 /**
  * @brief Enables/disables fetching the config from the portal over HTTP
  * (disabled permanently after a local config change).
  * @param load true to allow HTTP config loading.
  * @return true on success.
  */
 bool             setLoadHTTPConfig(bool);
 /**
  * @brief Returns the ETag of the current remote config, quoted for HTTP.
  * @return ETag surrounded by double quotes (may be empty).
  */
 String           getETAG();
 /**
  * @brief Sets the in-memory ETag of the remote config (strips quotes).
  * @param etag ETag string.
  * @return true on success.
  */
 bool             setETAG(String etag); 
 /**
  * @brief Persists the in-memory ETag to flash.
  * @return true on success.
  */
 bool             saveETAG(); 
 /**
  * @brief Loads the stored ETag from flash into memory.
  * @return true if a non-empty ETag was loaded.
  */
 bool             loadETAG(); 

 /**
  * @brief Returns whether static-IP-to-DHCP fallback is enabled.
  * @return true if the controller may fall back to DHCP.
  */
 bool             getDHCPfallback();
 /**
  * @brief Enables/disables static-IP-to-DHCP fallback in flash.
  * @param flag true to allow DHCP fallback.
  * @return true on success.
  */
 bool             setDHCPfallback(bool flag);

 /**
  * @brief Returns the packed system configuration flags from flash.
  * @return Current flags (log levels, behavior bits, config hash).
  */
 systemConfigFlags getConfigFlags();
 /**
  * @brief Writes the packed system configuration flags to flash.
  * @param flags Flags to store.
  * @return true on success.
  */
 bool setConfigFlags(systemConfigFlags flags);

 //bool             Save();  
};