#pragma once

#include "options.h"

#if defined(M5STACK)
#include <M5Stack.h>
#endif

#if defined(__SAM3X8E__)
#include <DueFlashStorage.h>
#include <watchdog.h>
#include <ArduinoHttpClient.h>
#endif

#if defined(ARDUINO_ARCH_AVR)
#include "HTTPClientAVR.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP_EEPROM.h>
#include <ESP8266HTTPClient.h>
//#include <ArduinoHttpClient.h>
//#include "HttpClient.h"
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <user_interface.h>
#define Ethernet WiFi
#endif

#if defined ARDUINO_ARCH_ESP32
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
//#include <EEPROM.h>
#include <NRFFlashStorage.h>
//#include "HttpClient.h"
//#include <ArduinoHttpClient.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <DNSServer.h>

#define Ethernet WiFi
#endif

#ifdef NRF5
#include <NRFFlashStorage.h>
#include <ArduinoHttpClient.h>
#endif

#ifdef ARDUINO_ARCH_STM32
#include "HttpClient.h"
#include "UIPEthernet.h"
#include <NRFFlashStorage.h>
//#include <EEPROM.h>
#endif

#if defined(__SAM3X8E__)
#define wdt_res() watchdogReset()
#define wdt_en()
#define wdt_dis()
#endif

#ifndef DHCP_RETRY_INTERVAL
#define DHCP_RETRY_INTERVAL 60000
#endif

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

#ifndef MODBUS_DISABLE
#include <ModbusMaster.h>
#endif

//#ifndef DMX_DISABLE
//#include "FastLED.h"
//#endif

#ifdef _owire
#include "owTerm.h"
#endif

#if defined(_dmxin) || defined(_dmxout) || defined (_artnet)
#include "dmx.h"
#endif


#ifdef Wiz5500
#include <Ethernet2.h>
#else
#if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__) || defined(NRF5)
#include <Ethernet.h>
#endif
#endif


#ifdef _artnet
#include <Artnet.h>
#endif

#ifdef SD_CARD_INSERTED
#include "sd_card_w5100.h"
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
    HAVE_IP_ADDRESS = 1,
    IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER = 2,
    RETAINING_COLLECTING = 3,
    OPERATION = 4,
    AWAITING_ADDRESS = -10,
    RECONNECT = 12,
    READ_RE_CONFIG = -11,
    DO_NOTHING = -14
};

typedef union {
    uint32_t  UID_Long[5];
    uint8_t   UID_Byte[20];
} UID;

//void watchdogSetup(void);

void mqttCallback(char *topic, byte *payload, unsigned int length);

void printMACAddress();

lan_status lanLoop();

#ifndef OWIRE_DISABLE
void Changed(int i, DeviceAddress addr, float currentTemp);
#endif

void modbusIdle(void);

void cmdFunctionHelp(int arg_cnt, char **args);

void cmdFunctionKill(int arg_cnt, char **args);

void applyConfig();

void cmdFunctionLoad(int arg_cnt, char **args);

int loadConfigFromEEPROM();

void cmdFunctionReq(int arg_cnt, char **args);

int mqttConfigRequest(int arg_cnt, char **args);

int mqttConfigResp(char *as);

void cmdFunctionSave(int arg_cnt, char **args);

void cmdFunctionSetMac(int arg_cnt, char **args);

void cmdFunctionGet(int arg_cnt, char **args);

void printBool(bool arg);

void saveFlash(short n, char *str);

int loadFlash(short n, char *str, short l=MAXFLASHSTR);

void saveFlash(short n, IPAddress& ip);

int ipLoadFromFlash(short n, IPAddress &ip);

lan_status loadConfigFromHttp(int arg_cnt = 0, char **args = NULL);

void preTransmission();

void postTransmission();

void setup_main();

void loop_main();

void owIdle(void);

void modbusIdle(void);

void inputLoop(void);

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

//void printFreeRam();
