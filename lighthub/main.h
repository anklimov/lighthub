#pragma once

#include "options.h"

#if defined(M5STACK)
#include <M5Stack.h>
#endif

#if defined(__SAM3X8E__)
#include <watchdog.h>
#include <ArduinoHttpClient.h>
//#include "TimerInterrupt_Generic.h"
#endif

#if defined(ARDUINO_ARCH_AVR)
#include "HTTPClient.h"
//#include <ArduinoHttpClient.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#ifndef FS_STORAGE
#include <ESP_EEPROM.h>
#endif
//#include <ESP8266HTTPClient.h>
//#include <ArduinoHttpClient.h>
//#include "HttpClient.h"
//#include <WiFiManager.h>
//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266WiFi.h>
#include <user_interface.h>
//#define Ethernet WiFi
#endif

#if defined ARDUINO_ARCH_ESP32
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
//#include "SPIFFS.h"
//#include <EEPROM.h>
//#include <NRFFlashStorage.h>
//#include "HttpClient.h"
//#include <ArduinoHttpClient.h>
//#include <HTTPClient.h>
//#include <WiFi.h>
//#include <WiFiClient.h>
//#include <WiFiClientSecure.h>
//#include <WebServer.h>
//#include <WiFiManager.h>
//include <DNSServer.h>

//#define Ethernet WiFi
#endif

#ifdef NRF5
//#include <NRFFlashStorage.h>
#include <ArduinoHttpClient.h>
#endif

#ifdef ARDUINO_ARCH_STM32
#include "HttpClient.h"
//#include "UIPEthernet.h"
//#include <NRFFlashStorage.h>
//#include <EEPROM.h>
#endif

#include "streamlog.h"
extern Streamlog  debugSerial;
extern Streamlog  infoSerial;
extern Streamlog  errorSerial;

#if defined(__SAM3X8E__)
#define wdt_res() watchdogReset()
#define wdt_en()
#define wdt_dis()
#endif

//#ifndef DHCP_RETRY_INTERVAL
//#define DHCP_RETRY_INTERVAL 60000
//#endif

#if defined(ESP8266)
#define wdt_en()   wdt_enable(WDTO_8S)
#define wdt_dis()  wdt_disable()
#define wdt_res()  wdt_reset()
#endif

#ifdef ARDUINO_ARCH_ESP32
#define wdt_res()
#define wdt_en()
#define wdt_dis()
#endif

#if defined(NRF5)
#define wdt_res()
#define wdt_en()
#define wdt_dis()
#endif

#if defined(ARDUINO_ARCH_STM32)
#define wdt_res()
#define wdt_en()
#define wdt_dis()
#endif

//#if defined(ESP8266)
//#define wdt_res()
//#define wdt_en()
//#define wdt_dis()
//#endif
#if defined(ARDUINO_ARCH_AVR)
#if defined(WATCH_DOG_TICKER_DISABLE)
#define wdt_en() wdt_disable()
#define wdt_dis() wdt_disable()
#define wdt_res() wdt_disable()
#else
#define wdt_en()   wdt_enable(WDTO_8S)
#define wdt_dis()  wdt_disable()
#define wdt_res()  wdt_reset()
#endif
#endif

#ifndef OWIRE_DISABLE
#include "DallasTemperature.h"
#endif

#ifdef _modbus
#include <ModbusMaster.h>
#endif

#ifdef _owire
#include "owTerm.h"
#endif

#if defined(_dmxin) || defined(_dmxout) || defined (_artnet)
#include "dmx.h"
#endif

#ifdef WIFI_ENABLE

  #if defined(ARDUINO_ARCH_ESP32)
        #include <WiFi.h>
        #include <HTTPClient.h>
        #include <WiFiClient.h>
        #include <WiFiClientSecure.h>
        #include <WiFiManager.h>
        #include <WebServer.h>
        #include <ESPmDNS.h>
  #else
        #include <ESP8266WiFi.h>
        #include <ESP8266HTTPClient.h>
        #include <WiFiManager.h>
        #include <DNSServer.h>
        #include <ESP8266WebServer.h>
        #include <ESP8266mDNS.h>
  #endif
#define Ethernet WiFi
#else  //Wired connection
        #ifdef Wiz5500
        #include <Ethernet2.h>
        #else
            #ifdef UIPETHERNET
            #include "UIPEthernet.h"
            #else
            #include <Ethernet.h>
            #endif
        #endif
#endif


#ifdef _artnet
#include <Artnet.h>
#endif

#ifdef SD_CARD_INSERTED
#include "sd_card_w5100.h"
#endif

#ifdef MDNS_ENABLE
    #ifndef WIFI_ENABLE
    #include <ArduinoMDNS.h>
    #endif
#endif

#ifdef IPMODBUS
#include "ipmodbus.h"
#endif

#include "Arduino.h"
#include "utils.h"
#include "textconst.h"
#include <PubSubClient.h>
#include <SPI.h>
#include <string.h>
#include "aJSON.h"
#include <Cmd.h>
#include "stdarg.h"
#include "item.h"
#include "inputs.h"

#ifdef _artnet
extern Artnet *artnet;
#endif

enum lan_status {
    INITIAL_STATE = 0,
    AWAITING_ADDRESS = 1,
    HAVE_IP_ADDRESS = 2,
    LIBS_INITIALIZED = 3,
    IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER = 4,
    RETAINING_COLLECTING = 5,
    OPERATION = 6,
    OPERATION_NO_MQTT = 7,
    DO_REINIT = -10,
    REINIT = - 11,
    DO_RECONNECT =  12,
    RECONNECT = 13,
    READ_RE_CONFIG = 14,
    DO_READ_RE_CONFIG = 15,
    DO_NOTHING = -15,
    DO_GET = -16,
    GET = -17
};

extern lan_status lanStatus;

typedef union {
    uint32_t  UID_Long[5];
    uint8_t   UID_Byte[20];
} UID;

bool isNotRetainingStatus();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void printMACAddress();

lan_status lanLoop();

#ifndef OWIRE_DISABLE
void Changed(int i, DeviceAddress addr, float currentTemp);
#endif

void modbusIdle(void);

int cmdFunctionHelp(int arg_cnt, char **args);

int cmdFunctionKill(int arg_cnt, char **args);

void applyConfig();

int cmdFunctionLoad(int arg_cnt, char **args);

int loadConfigFromEEPROM();

int cmdFunctionSave(int arg_cnt, char **args);

int cmdFunctionSetMac(int arg_cnt, char **args);

int cmdFunctionGet(int arg_cnt, char **args);

int cmdFunctionLoglevel(int arg_cnt, char **args);

void printBool(bool arg);
/*
void saveFlash(short n, char *str);

int loadFlash(short n, char *str, short l=MAXFLASHSTR);

void saveFlash(short n, IPAddress& ip);

int ipLoadFromFlash(short n, IPAddress &ip);
*/

int loadConfigFromHttp();

void preTransmission();

void postTransmission();

void setup_main();

void loop_main();

void owIdle(void);

void modbusIdle(void);

void inputLoop(short);

void inputSensorsLoop();

void inputSetup(void);

void pollingLoop(void);

void thermoLoop(void);

short thermoSetCurTemp(char *name, float t);

void modbusIdle(void);

void printConfigSummary();

void setupCmdArduino();

void setupMacAddress();

void printFirmwareVersionAndBuildOptions();

bool IsThermostat(const aJsonObject *item);

bool disabledDisconnected(const aJsonObject *thermoExtensionArray, int thermoLatestCommand);

void resetHard();

void onInitialStateInitLAN();

void ip_ready_config_loaded_connecting_to_broker();

void printCurentLanConfig();

void onMQTTConnect();

int16_t attachMaturaTimer();

//void printFreeRam();
