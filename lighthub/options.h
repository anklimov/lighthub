// Configuration of drivers enabled

#ifndef FASTLED
#define ADAFRUIT_LED
#endif
// ADAFRUIT library allow to dynamically configure SPI LED Strip Parameters

// If not defined ADAFRUIT_LED - FastLED library will be used instead
// And strip type, pin, order must defined on compilation time
#ifndef CONTROLLER
#define CONTROLLER TM1809
#endif

#ifndef DATA_PIN
#define DATA_PIN 4
#endif

#ifndef ORDER
#define ORDER BRG
#endif

#define TXEnablePin 13
#define ESP_EEPROM_SIZE 2048

#ifndef AVR_DMXOUT_PIN
#define AVR_DMXOUT_PIN 18
#endif

#define T_ATTEMPTS 200
#define IET_TEMP     0
#define IET_ATTEMPTS 1

#define THERMO_GIST_CELSIUS 1.
#define THERMO_OVERHEAT_CELSIUS 38.
#define FM_OVERHEAT_CELSIUS 40.

#define MIN_VOLUME 25
#define INIT_VOLUME 50

#define MAXFLASHSTR 32
#define PWDFLASHSTR 16

#define OFFSET_MAC 0
#define OFFSET_IP OFFSET_MAC+6
#define OFFSET_DNS OFFSET_IP+4
#define OFFSET_GW OFFSET_DNS+4
#define OFFSET_MASK OFFSET_GW+4
#define OFFSET_CONFIGSERVER OFFSET_MASK+4
#define OFFSET_MQTT_PWD OFFSET_CONFIGSERVER+MAXFLASHSTR
#define EEPROM_offset_NotAlligned OFFSET_MQTT_PWD+PWDFLASHSTR
#define EEPROM_offset EEPROM_offset_NotAlligned + (4 -(EEPROM_offset_NotAlligned & 3))

#ifndef INTERVAL_CHECK_INPUT
#define INTERVAL_CHECK_INPUT  15
#endif

#ifndef INTERVAL_CHECK_SENSOR
#define INTERVAL_CHECK_SENSOR  5000
#endif

#define INTERVAL_CHECK_MODBUS 2000
#define INTERVAL_POLLING      100
#define THERMOSTAT_CHECK_PERIOD 30000

#ifndef OW_UPDATE_INTERVAL
#define OW_UPDATE_INTERVAL 5000
#endif

#ifndef MODBUS_SERIAL_BAUD
#define MODBUS_SERIAL_BAUD 9600
#endif

#ifndef MODBUS_DIMMER_PARAM
#define MODBUS_DIMMER_PARAM SERIAL_8N1
#endif

#define dimPar MODBUS_DIMMER_PARAM
#define fmPar  SERIAL_8N1

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

#ifndef MODBUS_DISABLE
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
#define AC_Serial Serial2
#else
#define modbusSerial Serial2
#define AC_Serial Serial3
#endif
#define dmxin DMXSerial
#define dmxout DmxSimple
#endif

#if defined(__SAM3X8E__)
#define modbusSerial Serial2
#define AC_Serial Serial3
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
#undef _modbus

#ifndef DMX_DISABLE
#define _espdmx
#endif

#define modbusSerial Serial1
#define AC_Serial Serial1
#endif

#if defined(ARDUINO_ARCH_ESP32)
#undef _dmxin
#undef _modbus

#ifndef DMX_DISABLE
#define _espdmx
#endif
//#undef _dmxout
#undef modbusSerial
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

//#ifdef M5STACK
//#define debugSerial M5.Lcd
//#endif

#ifndef debugSerial
#define debugSerial Serial
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
#define PINS_COUNT NUM_DIGITAL_PINS
#define isAnalogPin(p)  ((p >= 14) && (p<=21))
#endif
