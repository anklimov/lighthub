#include "options.h"

#ifndef LIGHTHUB_MAIN_H
#define LIGHTHUB_MAIN_H


#if defined(__SAM3X8E__)
#define wdt_res() watchdogReset()
#define wdt_en()
#define wdt_dis()
#endif

#if defined(__AVR__)
#define wdt_en()   wdt_enable(WDTO_8S)
#define wdt_dis()  wdt_disable()
#define wdt_res()  wdt_reset()
#endif

#if defined(__ESP__)
#define wdt_res()
#define wdt_en()
#define wdt_dis()
#endif

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

#if defined(__ESP__)
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#endif

#ifdef _owire

#include "owTerm.h"

#endif

#if defined(_dmxin) || defined(_dmxout) || defined (_artnet)

#include "dmx.h"

#endif

#ifdef Wiz5500
#include <Ethernet2.h>
#else
#include <Ethernet.h>
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

#ifndef WITHOUT_DHT
#include "DHT.h"
#endif
//void watchdogSetup(void);

void mqttCallback(char *topic, byte *payload, unsigned int length);

#ifndef __ESP__

void printIPAddress();

#endif

void printMACAddress();

void restoreState();

int lanLoop();

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

int loadFlash(short n, IPAddress& ip);

int getConfig(int arg_cnt=0, char **args=NULL);

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

int getConfig(int arg_cnt, char **args);

void printFirmwareVersionAndBuildOptions();

#endif //LIGHTHUB_MAIN_H
