#include "options.h"

#ifndef LIGHTHUB_MAIN_H
#define LIGHTHUB_MAIN_H


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

//#if defined(ESP8266)
//#define wdt_res()
//#define wdt_en()
//#define wdt_dis()
//#endif

#if defined(WATCH_DOG_TICKER_DISABLE) && defined(__AVR__)
#define wdt_en() wdt_disable()
#define wdt_dis() wdt_disable()
#define wdt_res() wdt_disable()
#endif

#include "Arduino.h"
#include "DallasTemperature.h"
#include <PubSubClient.h>
#include <SPI.h>
#include "utils.h"
#include <string.h>
#include <ModbusMaster.h>
#include "aJSON.h"
#include <Cmd.h>
#include "stdarg.h"
#include "item.h"
#include "inputs.h"
#include "FastLED.h"
#include "Dns.h"
//#include "hsv2rgb.h"

#if defined(__SAM3X8E__)

#include <DueFlashStorage.h>
#include <watchdog.h>
#include <ArduinoHttpClient.h>

#endif

#if defined(__AVR__)
#include "HTTPClient.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#endif

#if defined(ESP8266)
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#endif

#ifdef _owire

#include "owTerm.h"

#endif

#if defined(_dmxin) || defined(_dmxout) || defined (_artnet)

#include "dmx.h"

#endif

#if defined(__AVR__) || defined(__SAM3X8E__) || defined(ESP8266)
#ifdef Wiz5500
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif
#endif

#ifdef ARDUINO_ARCH_ESP32
#include <SPI.h>
#include <Ethernet3.h>
#endif


#ifdef _artnet
#include <Artnet.h>
#endif

#ifdef SD_CARD_INSERTED
#include "sd_card_w5100.h"
#endif

#ifdef _artnet
extern Artnet *artnet;
#endif

enum lan_status {
    INITIAL_STATE = 0,
    HAVE_IP_ADDRESS = 1,
    IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER = 2,
    OPERATION = 3,
    RETAINING_COLLECTING = 4,
    AWAITING_ADDRESS = -10,
    RECONNECT = 12,
    READ_RE_CONFIG = -11,
    DO_NOTHING = -14
};

//void watchdogSetup(void);

void mqttCallback(char *topic, byte *payload, unsigned int length);


void printIPAddress(IPAddress ipAddress);

void printMACAddress();

void restoreState();

lan_status lanLoop();

void Changed(int i, DeviceAddress addr, int val);

void modbusIdle(void);

void cmdFunctionHelp(int arg_cnt, char **args);

void cmdFunctionKill(int arg_cnt, char **args);

void applyConfig();

void cmdFunctionLoad(int arg_cnt, char **args);

int loadConfigFromEEPROM(int arg_cnt, char **args);

void cmdFunctionReq(int arg_cnt, char **args);

int mqttConfigRequest(int arg_cnt, char **args);

int mqttConfigResp(char *as);

void cmdFunctionSave(int arg_cnt, char **args);

void cmdFunctionSetMac(int arg_cnt, char **args);

void cmdFunctionGet(int arg_cnt, char **args);

void printBool(bool arg);

void saveFlash(short n, char *str);

int loadFlash(short n, char *str, short l=32);

void saveFlash(short n, IPAddress& ip);

int ipLoadFromFlash(short n, IPAddress &ip);

lan_status getConfig(int arg_cnt=0, char **args=NULL);

void preTransmission();

void postTransmission();

void setup_main();

void loop_main();

void owIdle(void);

void modbusIdle(void);

void inputLoop(void);

void pollingLoop(void);

void thermoLoop(void);

short thermoSetCurTemp(char *name, short t);

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

#endif //LIGHTHUB_MAIN_H
