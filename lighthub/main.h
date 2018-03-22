//
// Created by livello on 13.03.18.
//

#ifndef LIGHTHUB_MAIN_H
#define LIGHTHUB_MAIN_H

#define TXEnablePin 13

#ifndef  SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

#define CUSTOM_FIRMWARE_MAC C4:3E:1f:03:1B:1E
#ifndef CUSTOM_FIRMWARE_MAC
#define FIRMWARE_MAC {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0}
#endif

#include "Arduino.h"
#include "DallasTemperature.h"

void watchdogSetup(void);
void callback(char* topic, byte* payload, unsigned int length);
#ifndef __ESP__
void printIPAddress();
#endif
void printMACAddress();
void restoreState();
int  lanLoop();
void Changed (int i, DeviceAddress addr, int val);
void modbusIdle(void);
void _handleHelp(int arg_cnt, char **args);
void _kill(int arg_cnt, char **args);
void applyConfig();
void _loadConfig (int arg_cnt, char **args);
int loadConfigFromEEPROM(int arg_cnt, char **args);
void _mqttConfigRequest(int arg_cnt, char **args);
int mqttConfigRequest(int arg_cnt, char **args);
int mqttConfigResp (char * as);
void _saveConfigToEEPROM(int arg_cnt, char **args);
void _setMacAddress(int arg_cnt, char **args);
void _getConfig(int arg_cnt, char **args);
void printBool (bool arg);
void saveFlash(short n, char* str);
void loadFlash(short n, char* str);
int getConfig (int arg_cnt, char **args);
void preTransmission();
void postTransmission();
void setup_main();
void loop_main();
void owIdle(void);
void modbusIdle(void);
void inputLoop(void);
void modbusLoop(void);
void thermoLoop(void);
short thermoSetCurTemp(char * name, short t);
#endif //LIGHTHUB_MAIN_H