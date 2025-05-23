#pragma once
#include <Arduino.h>

#ifndef NOT_FILTER_PID_OUT
#define NOT_FILTER_PID_OUT 1
#endif

#define DHCP_ATTEMPTS_FALLBACK 3
#define TENS_FRACT_LEN 2
#define TENS_BASE 100

#define DEFAULT_FILESIZE_LIMIT 65535
#ifndef MAX_JSON_CONF_SIZE

#if defined(__SAM3X8E__)
#define MAX_JSON_CONF_SIZE 65535
#elif defined(ARDUINO_ARCH_AVR)
#define MAX_JSON_CONF_SIZE 4096
#elif defined(ARDUINO_ARCH_ESP32)
#define MAX_JSON_CONF_SIZE 65535
#else
#define MAX_JSON_CONF_SIZE 32000
#endif

#endif

#ifdef MDNS_ENABLE
    #ifndef OTA_PORT
    #define OTA_PORT  65280
    #endif
#endif    

// Configuration of drivers enabled
#define SYSLOG_LOCAL_SOCKET 514

#ifndef MODBUS_UART_RX_PIN
#define MODBUS_UART_RX_PIN -1
#endif

#ifndef MODBUS_UART_TX_PIN
#define MODBUS_UART_TX_PIN -1
#endif

#ifndef FASTLED
#define ADAFRUIT_LED
#endif
// ADAFRUIT library allow to dynamically configure SPI LED Strip Parameters

// If not defined ADAFRUIT_LED - FastLED library will be used instead
// And strip type, pin, order must defined on compilation time
#ifndef CONTROLLER
#define CONTROLLER TM1809
#endif

#ifndef SCALE_VOLUME_100
#define SCALE_VOLUME_100 false
#endif

#ifndef DATA_PIN
#define DATA_PIN 4
#endif

#ifndef ORDER
#define ORDER BRG
#endif

#ifndef MODBUS_TX_PIN
#define TXEnablePin 13
#else
#define TXEnablePin MODBUS_TX_PIN
#endif

//#define ESP_EEPROM_SIZE 2048

#ifndef AVR_DMXOUT_PIN
#define AVR_DMXOUT_PIN 18
#endif

#define WIFI_TIMEOUT 60000UL
#define TIMEOUT_RECONNECT 10000UL
#define TIMEOUT_REINIT 5000UL
#define TIMEOUT_RELOAD 30000UL
#define TIMEOUT_RETAIN 8000UL
#define TIMEOUT_REINIT_NOT_CONFIGURED 120000UL
#define INTERVAL_1W 5000UL
#define PERIOD_THERMOSTAT_FAILED (600 * 1000UL) //16000 sec (4h) max

//#define T_ATTEMPTS 200
//#define IET_TEMP     0
//#define IET_ATTEMPTS 1
#ifndef THERMO_GIST_CELSIUS
#define THERMO_GIST_CELSIUS 1.
#endif

#ifndef THERMO_OVERHEAT_CELSIUS
#define THERMO_OVERHEAT_CELSIUS 38.
#endif

#ifndef FM_OVERHEAT_CELSIUS
#define FM_OVERHEAT_CELSIUS 40.
#endif

#ifndef MIN_VOLUME
#define MIN_VOLUME 20
#endif

#ifndef INIT_VOLUME
#define INIT_VOLUME 30
#endif

/*
#define MAXFLASHSTR 32
#define PWDFLASHSTR 16
#define EEPROM_SIGNATURE "LHCF"
#define EEPROM_SIGNATURE_LENGTH 4

#define OFFSET_MAC 0
#define OFFSET_IP OFFSET_MAC+6
#define OFFSET_DNS OFFSET_IP+4
#define OFFSET_GW OFFSET_DNS+4
#define OFFSET_MASK OFFSET_GW+4
#define OFFSET_CONFIGSERVER OFFSET_MASK+4
#define OFFSET_MQTT_PWD OFFSET_CONFIGSERVER+MAXFLASHSTR
#define OFFSET_SIGNATURE OFFSET_MQTT_PWD+PWDFLASHSTR
#define EEPROM_offset_NotAlligned OFFSET_SIGNATURE+EEPROM_SIGNATURE_LENGTH
#define EEPROM_offsetJSON EEPROM_offset_NotAlligned + (4 -(EEPROM_offset_NotAlligned & 3))
//#define EEPROM_offsetJSON IFLASH_PAGE_SIZE
#define EEPROM_FIX_PART_LEN EEPROM_offsetJSON-OFFSET_MAC
*/

#ifndef INTERVAL_CHECK_INPUT
#define INTERVAL_CHECK_INPUT  11
#endif

#define INTERVAL_CHECK_ULTRASONIC 100

#ifndef TIMER_CHECK_INPUT
#define TIMER_CHECK_INPUT  15
#endif
#ifndef INTERVAL_CHECK_SENSOR
#define INTERVAL_CHECK_SENSOR  5000
#endif

#define INTERVAL_SLOW_POLLING 1000
//#define INTERVAL_POLLING      100
#ifndef THERMOSTAT_CHECK_PERIOD
#define THERMOSTAT_CHECK_PERIOD 30000
#endif

#ifndef OW_UPDATE_INTERVAL
#define OW_UPDATE_INTERVAL 5000
#endif

#ifndef MODBUS_SERIAL_BAUD
#define MODBUS_SERIAL_BAUD 9600
#endif

#ifndef MODBUS_SERIAL_PARAM
#define MODBUS_SERIAL_PARAM SERIAL_8N1
#endif

/*
#ifndef MODBUS_TCP_BAUD
#define MODBUS_TCP_BAUD 9600
#endif

#ifndef MODBUS_TCP_PARAM
#define MODBUS_TCP_PARAM SERIAL_8N1
#endif
*/

#define MODBUS_FM_BAUD 9600
#define MODBUS_FM_PARAM  SERIAL_8N1

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

#ifndef CUSTOM_FIRMWARE_MAC
#define DEFAULT_FIRMWARE_MAC {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xFF}
#endif

#ifndef MY_CONFIG_SERVER
#define CONFIG_SERVER "lazyhome.ru"
#else
#define CONFIG_SERVER QUOTE(MY_CONFIG_SERVER)
#endif

#ifndef HOMETOPIC
#define HOMETOPIC  "myhome"
#endif

//Default output topic
#ifndef OUTTOPIC
#define OUTTOPIC "s_out"
#endif

//Topic to receive CLI commands
#ifndef CMDTOPIC
#define CMDTOPIC "$command"
#endif

//Default broadcast topic
#ifndef INTOPIC
#define INTOPIC  "in"
#endif

#define MQTT_SUBJECT_LENGTH 20
#define MQTT_TOPIC_LENGTH 64

#ifndef DMX_DISABLE
#define _dmxin
#define _dmxout
#endif

#ifndef OWIRE_DISABLE
#define _owire
#endif

#if !(defined  (MODBUS_DISABLE) && defined (MBUS_DISABLE))
#define _modbus
#endif

#ifdef ARTNET_ENABLE
#define _artnet
#endif

#ifndef LAN_INIT_DELAY
#define LAN_INIT_DELAY 500
#endif

#define DEFAULT_INC_STEP 5


#if defined(ARDUINO_ARCH_AVR)
//All options available
#ifdef CONTROLLINO
#define modbusSerial Serial3
    #ifndef AC_Serial
    #define AC_Serial Serial2
    #endif
#else
#define modbusSerial Serial2
    #ifndef AC_Serial
    #define AC_Serial Serial3
    #endif
#endif
#define dmxin DMXSerial
#define dmxout DmxSimple
#endif

#if defined(__SAM3X8E__)
#ifndef modbusSerial
#define modbusSerial Serial2
#endif

#ifndef AC_Serial
#define AC_Serial Serial3
#endif

#define dmxout DmxDue1
#define dmxin  DmxDue1
#endif

#if defined(NRF5)
//#define modbusSerial Serial1
#undef _dmxin
#undef _dmxout
#undef _modbus
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#undef _dmxin

#ifndef DMX_DISABLE
#define _espdmx
#endif

#ifndef modbusSerial
#define modbusSerial Serial1
#endif

#ifndef AC_Serial
#define AC_Serial Serial1
#endif

#endif

#if defined(ARDUINO_ARCH_ESP32)
#undef _dmxin
#ifndef DMX_DISABLE
#define _espdmx
#endif

#ifndef modbusSerial
#define modbusSerial Serial2
#endif

#ifndef AC_Serial
#define AC_Serial Serial2
#endif

#endif

#ifndef _dmxout
#undef _artnet
#endif

#ifdef WIFI_MANAGER_DISABLE
#ifndef ESP_WIFI_AP
#define ESP_WIFI_AP mywifiap
#endif

#ifndef ESP_WIFI_PWD
#define ESP_WIFI_PWD mywifipass
#endif
#endif

#define DHT_POLL_DELAY_DEFAULT 15000
#define UPTIME_POLL_DELAY_DEFAULT 30000

#ifdef ARDUINO_ARCH_STM32F1
#define strncpy_P strncpy
#endif

#ifndef DMX_SMOOTH_DELAY
#define DMX_SMOOTH_DELAY 10
#endif

//#ifdef M5STACK
//#define debugSerial M5.Lcd
//#endif
#ifdef NOSERIAL
    #undef debugSerialPort
#else
    #ifndef debugSerialPort
    #define debugSerialPort Serial
    #endif
#endif

#ifndef Wiz5500
#define W5100_ETHERNET_SHIELD
#else
#define W5500_ETHERNET_SHIELD
#endif


#if defined(ARDUINO_ARCH_AVR)
#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ((p >= 54) && (p<=69))
#endif

#if defined(__SAM3X8E__)
#define isAnalogPin(p)  (g_APinDescription[p].ulPinAttribute & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG
#endif

#if defined(ARDUINO_ARCH_STM32)
#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ((p >= 44) && (p<=57))
#endif

#if defined(ESP8266)
#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ( p ==17 )
#endif

#if defined(ARDUINO_ARCH_ESP32)
#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ((p ==4) || (p>=12)&& (p<=15) || (p>=25)&& (p<=27)||(p>=32)&& (p<=33) || (p>=37)&& (p<=38))
#endif

#if defined(NRF5)
//#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ((p >= 14) && (p<=21))
#endif

#ifdef AVR
#define minimalMemory 200
#else
#define minimalMemory 1200
#endif

#if not defined PROTECTED_PINS 
#define PROTECTED_PINS
#endif

#if not defined DEFAULT_OTA_USERNAME 
#define DEFAULT_OTA_USERNAME arduino
#endif

#if not defined DEFAULT_OTA_PASSWORD 
#define DEFAULT_OTA_PASSWORD password
#endif

const short protectedPins[]={PROTECTED_PINS};
#define protectedPinsNum (sizeof(protectedPins)/sizeof(short))

#ifdef CRYPT
#if not defined SHAREDSECRET
#define SHAREDSECRET "12345678"
#endif

#endif