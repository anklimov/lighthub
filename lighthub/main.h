/**
 * @file main.h
 * @brief Main controller interface of LightHub.
 *
 * Declares the application entry points (setup_main/loop_main), the
 * LAN/MQTT state machine (lan_status, lanLoop) and its initialization
 * routines (onInitialStateInitLAN, onMQTTConnect,
 * ip_ready_config_loaded_connecting_to_broker), remote config handling
 * (loadConfigFromHttp, loadConfigFromEEPROM, applyConfig), the serial CLI
 * command handlers (cmdFunction*) and the periodic polling loops
 * (pollingLoop, thermoLoop, inputLoop) that drive items, thermostats
 * and inputs.
 */
#pragma once

#include "options.h"

#if defined(M5STACK)
#include <M5Stack.h>
#endif

#if defined(__SAM3X8E__)
#include <watchdog.h>
    #if not defined (NOIP)
    #include <ArduinoHttpClient.h>
    #endif
//#include "TimerInterrupt_Generic.h"
#endif

#if defined(ARDUINO_ARCH_AVR)
    #if not defined (NOIP)
    #include "HTTPClient.h"
    #endif

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

#if defined (ARDUINO_ARCH_ESP32)
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

#if defined (ARDUINO_ARCH_STM32) and not defined (NOIP)
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
#else
#if  not defined (NOIP)  //Wired connection
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
    #if not defined (NOIP)
    #include <PubSubClient.h>
    #endif
#include <SPI.h>
#include <string.h>
#include "aJSON.h"
#include <Cmd.h>
#include "stdarg.h"
#include "item.h"
#include "inputs.h"
#ifdef CANDRV
#include <candriver.h>
#endif

#ifdef _artnet
extern Artnet *artnet;
#endif

/**
 * @brief LAN/MQTT boot-up and runtime state machine states.
 *
 * Positive values are boot/operation states; negative and high values
 * are transient actions requested by loop_main (reinit, reconnect,
 * config re-read, get).
 */
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
    GET = -17,
    GET_IN_PROGRESS = 18,
    AWAITING_CONFIG = 19
};

/** @brief Current state of the LAN/MQTT state machine. */
extern lan_status lanStatus;

/** @brief Chip unique ID (20 bytes), viewable as 5 longs or 20 bytes. */
typedef union {
    uint32_t  UID_Long[5];
    uint8_t   UID_Byte[20];
} UID;

/**
 * @brief Checks whether the controller is not in the retain/collecting phase.
 * @return true if lanStatus is not RETAINING_COLLECTING.
 */
bool isNotRetainingStatus();

#if not defined (NOIP)
/**
 * @brief MQTT receive callback: dispatches incoming messages to item
 * command processing or config handling.
 * @param topic MQTT topic of the received message.
 * @param payload Message payload (not necessarily null-terminated).
 * @param length Payload length in bytes.
 */
void mqttCallback(char *topic, byte *payload, unsigned int length);
/**
 * @brief One iteration of the LAN/MQTT state machine; advances the
 * ethernet/WiFi/MQTT boot sequence and returns the new state.
 * @return Next lan_status for the main loop.
 */
lan_status lanLoop();
/**
 * @brief Downloads the device JSON config from the config server
 * (HTTP GET with If-None-Match ETag), parses and applies it.
 * @return HTTP status code (200 on success, 304 if unchanged, negative on error).
 */
int loadConfigFromHttp();
/**
 * @brief Initial LAN setup: initializes ethernet/WiFi and starts the
 * address acquisition sequence.
 */
void onInitialStateInitLAN();
/**
 * @brief Called on (re)connect to the MQTT broker: sets up topics,
 * subscribes and publishes retained states.
 */
void onMQTTConnect();
/**
 * @brief State handler for IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER:
 * connects to the configured MQTT broker(s).
 */
void ip_ready_config_loaded_connecting_to_broker();

#endif

/**
 * @brief Resolves the controller MAC address (from flash or chip UID)
 * and stores it in the system config.
 */
void setupMacAddress();
/**
 * @brief Prints the current MAC address to the info log.
 */
void printMACAddress();

#ifndef OWIRE_DISABLE
/**
 * @brief 1-Wire temperature change callback.
 * @param i Sensor index.
 * @param addr DeviceAddress of the sensor.
 * @param currentTemp Newly read temperature in Celsius.
 */
void Changed(int i, DeviceAddress addr, float currentTemp);
#endif

/**
 * @brief Idle hook invoked by the Modbus code between serial operations
 * to let the main system breathe.
 */
void modbusIdle(void);

/**
 * @brief CLI command "help": lists available CLI commands.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings.
 * @return Command result code.
 */
int cmdFunctionHelp(int arg_cnt, char **args);

/**
 * @brief CLI command "kill": aborts the current long operation.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings.
 * @return Command result code.
 */
int cmdFunctionKill(int arg_cnt, char **args);

/**
 * @brief Applies the loaded JSON config tree to the running system
 * (items, inputs, topics, drivers).
 * @return true if the config was applied successfully.
 */
bool  applyConfig();

/**
 * @brief CLI command "load": loads the config from the portal or flash.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings.
 * @return Command result code.
 */
int cmdFunctionLoad(int arg_cnt, char **args);

/**
 * @brief Loads the last saved config from flash into the config tree.
 * @return 0 on success, nonzero on error.
 */
int loadConfigFromEEPROM();

/**
 * @brief CLI command "save": persists the current config tree to flash.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings.
 * @return Command result code.
 */
int cmdFunctionSave(int arg_cnt, char **args);

/**
 * @brief CLI command "mac": changes the controller MAC address.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings (new MAC).
 * @return Command result code.
 */
int cmdFunctionSetMac(int arg_cnt, char **args);

/**
 * @brief CLI command "get": re-downloads the config from the portal.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings.
 * @return Command result code.
 */
int cmdFunctionGet(int arg_cnt, char **args);

/**
 * @brief CLI command "log": changes serial/UDP log levels.
 * @param arg_cnt Number of arguments.
 * @param args Argument strings (levels).
 * @return Command result code.
 */
int cmdFunctionLoglevel(int arg_cnt, char **args);

/**
 * @brief Prints a bool as "+" or "-" to the info log.
 * @param arg Value to print.
 */
void printBool(bool arg);

int cmdFunctionSave(int arg_cnt, char **args);
/*
void saveFlash(short n, char *str);

int loadFlash(short n, char *str, short l=MAXFLASHSTR);

void saveFlash(short n, IPAddress& ip);

int ipLoadFromFlash(short n, IPAddress &ip);
*/



/**
 * @brief Marks that an Ethernet transmission is about to happen;
 * disables time-sensitive drivers as needed.
 */
void preTransmission();

/**
 * @brief Marks that an Ethernet transmission has finished;
 * re-enables drivers and handles lost-time compensation.
 */
void postTransmission();

/**
 * @brief Application setup: init serial, flash config, MAC, LAN,
 * HTTP/CLI listeners and all drivers (runs once at boot).
 */
void setup_main();

/**
 * @brief Application main loop: runs the LAN/MQTT state machine, CLI
 * poller, item polling, thermostat, input, DMX and CAN iterations.
 */
void loop_main();

/**
 * @brief Idle hook invoked by the 1-Wire/DS2482 code between operations.
 */
void owIdle(void);

void modbusIdle(void);

/**
 * @brief Polls digital/analog inputs and processes their events.
 * @param cause Why the loop is running (e.g. CHECK_INPUT).
 */
void inputLoop(short);

/**
 * @brief Polls periodic sensors (DHT, ultrasonic, ...) connected to inputs.
 */
void inputSensorsLoop();

/**
 * @brief Sets up input channels from the loaded config.
 */
void inputSetup(void);
/**
 * @brief Stops and releases all input channels.
 */
void inputStop(void);

/**
 * @brief Periodic item polling: refreshes states and publishes changes.
 */
void pollingLoop(void);

/**
 * @brief Thermostat control loop: checks temperature readings and
 * drives heater relays (on/off/error bypass) with overheat alarms.
 */
void thermoLoop(void);

/**
 * @brief Updates the current-temperature store of a thermostat by name.
 * @param name Name of the thermostat item.
 * @param t New temperature in Celsius.
 * @return 0 on success, nonzero if the item was not found.
 */
short thermoSetCurTemp(char *name, float t);

void modbusIdle(void);

/**
 * @brief Prints a summary of the current network/system config.
 */
void printConfigSummary();

/**
 * @brief Registers the serial CLI commands (help, save, load, get,
 * mac, ip, kill, reboot, log, ...) with the Cmd library.
 */
void setupCmdArduino();

/**
 * @brief Prints firmware version, revision and build options at boot.
 */
void printFirmwareVersionAndBuildOptions();

/**
 * @brief Checks whether a config array entry is a thermostat item.
 * @param item aJsonObject config array to check.
 * @return true if the entry is a CH_THERMO item.
 */
bool IsThermostat(const aJsonObject *item);

/**
 * @brief Checks whether a thermostat must be stopped (disabled or its
 * temperature sensor disconnected for too long).
 * @param thermoExtensionArray Thermostat extension array from config.
 * @param thermoLatestCommand Last received thermostat command.
 * @return true if the thermostat should be disabled.
 */
bool disabledDisconnected(const aJsonObject *thermoExtensionArray, int thermoLatestCommand);

/**
 * @brief Hard-resets the board (watchdog or MCU reset).
 */
void resetHard();

/**
 * @brief Frees the current config tree and its driver resources.
 * @param locksAlowed Number of config locks that may be active.
 * @return true if the config was successfully released.
 */
bool cleanConf(short locksAlowed=0);

/**
 * @brief Prints the current LAN configuration to the serial port.
 */
void printCurentLanConfig();

/**
 * @brief Attaches the Matura timer (interrupt-based timing helper).
 * @return Attached timer number, or negative on error.
 */
int16_t attachMaturaTimer();

/**
 * @brief Selects the first configured MQTT broker for connection.
 */
void setFirstBroker();
/**
 * @brief Moves to the next configured MQTT broker (failover).
 */
void setNextBroker();

//void printFreeRam();
