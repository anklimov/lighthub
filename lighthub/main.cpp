/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.
 *
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Homepage: http://lazyhome.ru
GIT:      https://github.com/anklimov/lighthub
e-mail    anklimov@gmail.com



 *
 *
 * Done:
 * MQMT/openhab
 * 1-wire
 * DMX - out
 * DMX IN
 * 1809 strip out (discarded)
 * Modbus master Out
 * DHCP
 * JSON config
 * cli
 * PWM Out 7,8,9
 * 1-w relay out
 * Termostat out

Todo (backlog)
===
rotary encoder local ctrl ?
analog in local ctrl
Smooth regulation/fading
PID Termostat out ?
dmx relay out
Relay array channel
Relay DMX array channel
Config URL & MQTT password commandline configuration
1-wire Update refactoring (save memory)
Topic configuration
Timer
Modbus response check
control/debug (Commandline) over MQTT
more Modbus dimmers

todo DUE related:
PWM freq fix
Config webserver
SSL

todo ESP:
Config webserver
SSL

ESP32
PWM Out

*/
#include "Arduino.h"
#include "main.h"
#include "options.h"

#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
EthernetClient ethClient;
#endif

#if defined(__AVR__)
EthernetClient ethClient;
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <user_interface.h>
WiFiClient ethClient;
#endif

#ifdef ARDUINO_ARCH_ESP32
#include <WiFiClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <EEPROM.h>
WiFiClient ethClient;
#endif

lan_status lanStatus = INITIAL_STATE;

const char outprefix[] PROGMEM = OUTTOPIC;
const char inprefix[] PROGMEM = INTOPIC;
const char configserver[] PROGMEM = CONFIG_SERVER;


aJsonObject *root = NULL;
aJsonObject *items = NULL;
aJsonObject *inputs = NULL;

aJsonObject *mqttArr = NULL;
aJsonObject *modbusArr = NULL;
aJsonObject *owArr = NULL;
aJsonObject *dmxArr = NULL;

unsigned long nextPollingCheck = 0;
unsigned long nextInputCheck = 0;
unsigned long nextLanCheckTime = 0;
unsigned long nextThermostatCheck = 0;

aJsonObject *pollingItem = NULL;

bool owReady = false;
bool configOk = false;

#ifdef _modbus
ModbusMaster node;
#endif

byte mac[6];

PubSubClient mqttClient(ethClient);

bool wifiInitialized;

int mqttErrorRate;

void watchdogSetup(void) {
//Serial.begin(115200);
//Serial.println("Watchdog armed.");
}    //Do not remove - strong re-definition WDT Init for DUE


// MQTT Callback routine


void mqttCallback(char *topic, byte *payload, unsigned int length) {

    Serial.print(F("\n["));
    Serial.print(topic);
    Serial.print(F("] "));
    if (!payload) return;
      payload[length] = 0;

    int fr = freeRam();
    if (fr < 250) {
        Serial.println(F("OOM!"));
        return;
    }

    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();

    if(!strcmp(topic,CMDTOPIC)) {
      cmd_parse((char *)payload);
      return;
    }

    boolean retaining = (lanStatus == RETAINING_COLLECTING);
    //Check if topic = Command topic
    short intopic = 0;
    {
        char buf[MQTT_TOPIC_LENGTH + 1];
        strncpy_P(buf, inprefix, sizeof(buf));
        intopic = strncmp(topic, buf, strlen(inprefix));
    }
    // in Retaining status - trying to restore previous state from retained output topic. Retained input topics are not relevant.
    if (retaining && !intopic) {
        Serial.println(F("Skipping.."));
        return;
    }
    char subtopic[MQTT_SUBJECT_LENGTH] = "";
  //  int cmd = 0;
    //cmd = txt2cmd((char *) payload);
    char *t;
    if (t = strrchr(topic, '/'))
        strncpy(subtopic, t + 1, MQTT_SUBJECT_LENGTH - 1);
        Item item(subtopic);
        if (item.isValid()) {
            if (item.itemType == CH_GROUP && retaining)
                return; //Do not restore group channels - they consist not relevant data
        item.Ctrl((char *)payload, !retaining);
        } //valid item
}

void printIPAddress(IPAddress ipAddress) {
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        Serial.print(ipAddress[thisByte], DEC);
        if (thisByte < 3)
            Serial.print(F("."));
    }
}

void printMACAddress() {
    Serial.print(F("Configured MAC:"));
    for (byte thisByte = 0; thisByte < 6; thisByte++) {
        Serial.print(mac[thisByte], HEX);
        Serial.print(F(":"));
    }
    Serial.println();
}

void restoreState() {
    // Once connected, publish an announcement... // Once connected, publish an announcement...
    //mqttClient.publish("/myhome/out/RestoreState", "ON");
};

lan_status lanLoop() {

    #ifdef NOETHER
    lanStatus=DO_NOTHING;//-14;
#endif

    switch (lanStatus) {
        case INITIAL_STATE:
            onInitialStateInitLAN();
            break;

        case HAVE_IP_ADDRESS:
            if (!configOk)
                lanStatus = getConfig(0, NULL); //got config from server or load from NVRAM
            else lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
#ifdef _artnet
            if (artnet) artnet->begin();
#endif
            break;

        case IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER:
            wdt_res();
            ip_ready_config_loaded_connecting_to_broker();
            break;

        case RETAINING_COLLECTING:
            if (millis() > nextLanCheckTime) {
                char buf[MQTT_TOPIC_LENGTH];

                //Unsubscribe from status topics..
                strncpy_P(buf, outprefix, sizeof(buf));
                strncat(buf, "#", sizeof(buf));
                mqttClient.unsubscribe(buf);

                lanStatus = OPERATION;//3;
                Serial.println(F("Accepting commands..."));
                break;
            }

        case OPERATION:
            if (!mqttClient.connected()) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            break;

        case AWAITING_ADDRESS:
            if (millis() > nextLanCheckTime)
                lanStatus = INITIAL_STATE;//0;
            break;

        case RECONNECT:
            if (millis() > nextLanCheckTime)

                lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            break;

        case READ_RE_CONFIG:
            if (loadConfigFromEEPROM(0, NULL)) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            else {
                nextLanCheckTime = millis() + 5000;
                lanStatus = AWAITING_ADDRESS;//-10;
            }
            break;

        case DO_NOTHING:;
    }


    {
#if defined(__AVR__) || defined(__SAM3X8E__)
        wdt_dis();
        if (lanStatus > 0)
            switch (Ethernet.maintain()) {
                   case NO_LINK:
                    Serial.println(F("No link"));
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 30000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;
                case DHCP_CHECK_RENEW_FAIL:
                    Serial.println(F("Error: renewed fail"));
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 1000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;

                case DHCP_CHECK_RENEW_OK:
                    Serial.println(F("Renewed success. IP address:"));
                    printIPAddress(Ethernet.localIP());
                    break;

                case DHCP_CHECK_REBIND_FAIL:
                    Serial.println(F("Error: rebind fail"));
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 1000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;

                case DHCP_CHECK_REBIND_OK:
                    Serial.println(F("Rebind success. IP address:"));
                    printIPAddress(Ethernet.localIP());
                    break;

                default:
                    break;

            }

        wdt_en();
#endif
    }

    return lanStatus;

}

void ip_ready_config_loaded_connecting_to_broker() {
    short n = 0;
    int port = 1883;
    char empty = 0;
    char *user = &empty;
    char passwordBuf[16] = "";
    char *password = passwordBuf;


    if (!mqttClient.connected() && mqttArr && ((n = aJson.getArraySize(mqttArr)) > 1)) {
                    char *client_id = aJson.getArrayItem(mqttArr, 0)->valuestring;
                    char *servername = aJson.getArrayItem(mqttArr, 1)->valuestring;
                    if (n >= 3) port = aJson.getArrayItem(mqttArr, 2)->valueint;
                    if (n >= 4) user = aJson.getArrayItem(mqttArr, 3)->valuestring;
                    if (!loadFlash(OFFSET_MQTT_PWD, passwordBuf, sizeof(passwordBuf)) && (n >= 5)) {
                        password = aJson.getArrayItem(mqttArr, 4)->valuestring;
                        Serial.println(F("Using MQTT password from config"));
                    }

                    mqttClient.setServer(servername, port);
                    mqttClient.setCallback(mqttCallback);

                    Serial.print(F("Attempting MQTT connection to "));
                    Serial.print(servername);
                    Serial.print(F(":"));
                    Serial.print(port);
                    Serial.print(F(" user:"));
                    Serial.print(user);
                    Serial.print(F(" ..."));

                    wdt_dis();  //potential unsafe for ethernetIdle(), but needed to avoid cyclic reboot if mosquitto out of order
                    if (mqttClient.connect(client_id, user, password)) {
                        mqttErrorRate = 0;
                        Serial.print(F("connected as "));
                        Serial.println(client_id);
                        wdt_en();
                        configOk = true;
                        // ... Temporary subscribe to status topic
                        char buf[MQTT_TOPIC_LENGTH];

                        strncpy_P(buf, outprefix, sizeof(buf));
                        strncat(buf, "#", sizeof(buf));
                        mqttClient.subscribe(buf);

                        //Subscribing for command topics
                        strncpy_P(buf, inprefix, sizeof(buf));
                        strncat(buf, "#", sizeof(buf));
                        mqttClient.subscribe(buf);

                        //restoreState();
                        // if (_once) {DMXput(); _once=0;}
                        lanStatus = RETAINING_COLLECTING;//4;
                        nextLanCheckTime = millis() + 5000;
                        Serial.println(F("Awaiting for retained topics"));
                    } else {
                        Serial.print(F("failed, rc="));
                        Serial.print(mqttClient.state());
                        Serial.println(F(" try again in 5 seconds"));
                        nextLanCheckTime = millis() + 5000;
#ifdef RESTART_LAN_ON_MQTT_ERRORS
                        mqttErrorRate++;
                        if(mqttErrorRate>50){
                            Serial.print(F("Too many MQTT connection errors. Restart LAN"));
                            mqttErrorRate=0;
#ifdef RESET_PIN
                            resetHard();
#endif
                            lanStatus=INITIAL_STATE;
                            return;
                        }
#endif

                        lanStatus = RECONNECT;//12;
                    }
                }
}

void onInitialStateInitLAN() {
#if defined(ESP8266) and defined(WIFI_MANAGER_DISABLE)
    if(!wifiInitialized) {
                WiFi.mode(WIFI_STA);
                Serial.print(F("WIFI AP/Password:"));
                Serial.print(QUOTE(ESP_WIFI_AP));
                Serial.print(F("/"));
                Serial.println(QUOTE(ESP_WIFI_PWD));
                wifi_set_macaddr(STATION_IF,mac);
                WiFi.begin(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));
                wifiInitialized = true;
            }
#endif

#ifdef ARDUINO_ARCH_ESP32
    if(!wifiInitialized) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        Serial.print(F("WIFI AP/Password:"));
        Serial.print(QUOTE(ESP_WIFI_AP));
        Serial.print(F("/"));
        Serial.println(QUOTE(ESP_WIFI_PWD));
        WiFi.begin(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));

        int wifi_connection_wait = 10000;
        while (WiFi.status() != WL_CONNECTED && wifi_connection_wait > 0) {
            delay(500);
            wifi_connection_wait -= 500;
            Serial.print(".");
        }
        wifiInitialized = true;
    }
#endif

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP8266)
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("WiFi connected. IP address: "));
        Serial.println(WiFi.localIP());
        lanStatus = HAVE_IP_ADDRESS;//1;
    } else
    {
        Serial.println(F("Problem with WiFi connected"));
        nextLanCheckTime = millis() + DHCP_RETRY_INTERVAL/5;
    }
#endif

    #if defined(__AVR__) || defined(__SAM3X8E__)
    IPAddress ip, dns, gw, mask;
    int res = 1;
    Serial.println(F("Starting lan"));
    if (ipLoadFromFlash(OFFSET_IP, ip)) {
        Serial.print("Loaded from flash IP:");
        printIPAddress(ip);
        if (ipLoadFromFlash(OFFSET_DNS, dns)) {
            Serial.print(" DNS:");
            printIPAddress(dns);
            if (ipLoadFromFlash(OFFSET_GW, gw)) {
                Serial.print(" GW:");
                printIPAddress(gw);
                if (ipLoadFromFlash(OFFSET_MASK, mask)) {
                    Serial.print(" MASK:");
                    printIPAddress(mask);
                    Ethernet.begin(mac, ip, dns, gw, mask);
                } else Ethernet.begin(mac, ip, dns, gw);
            } else Ethernet.begin(mac, ip, dns);
        } else Ethernet.begin(mac, ip);
    }
        else {
        Serial.println("No IP data found in flash");
        wdt_dis();
        res = Ethernet.begin(mac, 12000);
        wdt_en();
        wdt_res();
    }

    if (res == 0) {
        Serial.println(F("Failed to configure Ethernet using DHCP. You can set ip manually!"));
        Serial.print(F("'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n"));
        lanStatus = AWAITING_ADDRESS;//-10;
        nextLanCheckTime = millis() + DHCP_RETRY_INTERVAL;
#ifdef RESET_PIN
        resetHard();
#endif
    } else {
        Serial.print(F("Got IP address:"));
        printIPAddress(Ethernet.localIP());
        lanStatus = HAVE_IP_ADDRESS;//1;
    }


#endif
}

void (*softRebootFunc)(void) = 0;

void resetHard() {
#ifdef RESET_PIN
    Serial.print(F("Reset Arduino with digital pin "));
    Serial.println(QUOTE(RESET_PIN));
    delay(500);
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN,LOW);
    delay(500);
    digitalWrite(RESET_PIN,HIGH);
    delay(500);
#endif    
}

#ifdef _owire

void Changed(int i, DeviceAddress addr, int val) {
    char addrstr[32] = "NIL";
    char addrbuf[17];
    char valstr[16] = "NIL";
    char *owEmit = NULL;
    char *owItem = NULL;

    SetBytes(addr, 8, addrbuf);
    addrbuf[17] = 0;

    aJsonObject *owObj = aJson.getObjectItem(owArr, addrbuf);
    if (owObj) {
        owEmit = aJson.getObjectItem(owObj, "emit")->valuestring;
        if (owEmit) {
            strncpy(addrbuf, owEmit, sizeof(addrbuf));
            Serial.print(owEmit);
            Serial.print(F("="));
            Serial.println(val);
        }
        owItem = aJson.getObjectItem(owObj, "item")->valuestring;
    } else Serial.println(F("1w-item not found in config"));

    if ((val == -127) || (val == 85) || (val == 0)) { //ToDo: 1-w short circuit mapped to "0" celsium
        return;
    }

    strcpy_P(addrstr, outprefix);
    strncat(addrstr, addrbuf, sizeof(addrstr));
    sprintf(valstr, "%d", val);
    mqttClient.publish(addrstr, valstr);

    if (owItem) {
        thermoSetCurTemp(owItem, val);  ///TODO: Refactore using Items interface
    }
}

#endif //_owire

void cmdFunctionHelp(int arg_cnt, char **args)
//(char* tokens)
{
    printFirmwareVersionAndBuildOptions();
    Serial.println(F("Use the commands: 'help' - this text\n"
                             "'mac de:ad:be:ef:fe:00' set and store MAC-address in EEPROM\n"
                             "'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n"
                             "'save' - save config in NVRAM\n"
                             "'get' [config addr]' - get config from pre-configured URL and store addr\n"
                             "'load' - load config from NVRAM\n"
                             "'pwd' - define MQTT password\n"
                             "'kill' - test watchdog\n"
                             "'clear' - clear EEPROM\n"
                             "'reboot' - reboot controller"));
}

void cmdFunctionKill(int arg_cnt, char **args) {
    for (short i = 1; i < 20; i++) {
        delay(1000);
        Serial.println(i);
    };
}

void cmdFunctionReboot(int arg_cnt, char **args) {
    Serial.println(F("Soft rebooting..."));
    softRebootFunc();
}

void applyConfig() {
  if (!root) return;

#ifdef _dmxin
    int itemsCount;
    dmxArr = aJson.getObjectItem(root, "dmxin");
    if (dmxArr && (itemsCount = aJson.getArraySize(dmxArr))) {
        DMXinSetup(itemsCount * 4);
        Serial.print(F("DMX in started. Channels:"));
        Serial.println(itemsCount * 4);
    }
#endif
#ifdef _dmxout
    int maxChannels;
    aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
    if (dmxoutArr && aJson.getArraySize(dmxoutArr) >=1 ) {
        DMXoutSetup(maxChannels = aJson.getArrayItem(dmxoutArr, 1)->valueint);
        //,aJson.getArrayItem(dmxoutArr, 0)->valueint);
        Serial.print(F("DMX out started. Channels: "));
        Serial.println(maxChannels);
    }
#endif
#ifdef _modbus
    modbusArr = aJson.getObjectItem(root, "modbus");
#endif

#ifdef _owire
    owArr = aJson.getObjectItem(root, "ow");
    if (owArr && !owReady) {
        aJsonObject *item = owArr->child;
        owReady = owSetup(&Changed);
        if (owReady) Serial.println(F("One wire Ready"));
        t_count = 0;
        while (item && owReady) {
            if ((item->type == aJson_Object)) {
                DeviceAddress addr;
                //Serial.print(F("Add:")),Serial.println(item->name);
                SetAddr(item->name, addr);
                owAdd(addr);
            }
            item = item->next;
        }
    }
#endif
    items = aJson.getObjectItem(root, "items");

// Digital output related Items initialization
pollingItem=NULL;
if (items) {
aJsonObject * item = items->child;
while (items && item)
  if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
    Item it(item);
    if (it.isValid()) {
    int pin=it.getArg();
    int cmd = it.getCmd();
    switch (it.itemType) {
          case CH_THERMO:
          if (cmd<1) it.setCmd(CMD_OFF);
          case CH_RELAY:
            {
            int k;
            pinMode(pin, OUTPUT);
            digitalWrite(pin, k = ((cmd == CMD_ON) ? HIGH : LOW));
            Serial.print(F("Pin:"));
            Serial.print(pin);
            Serial.print(F("="));
            Serial.println(k);
            }
            break;
          } //switch
       } //isValid
          item = item->next;
     }  //if
    pollingItem = items->child;
}
    inputs = aJson.getObjectItem(root, "in");
    mqttArr = aJson.getObjectItem(root, "mqtt");
    printConfigSummary();
}

void printConfigSummary() {
    Serial.println(F("Configured:"));
    Serial.print(F("items "));
    printBool(items);
    Serial.print(F("inputs "));
    printBool(inputs);
    Serial.print(F("modbus "));
    printBool(modbusArr);
    Serial.print(F("mqtt "));
    printBool(mqttArr);
    Serial.print(F("1-wire "));
    printBool(owArr);
}

void cmdFunctionLoad(int arg_cnt, char **args) {
    loadConfigFromEEPROM(arg_cnt, args);
    restoreState();
}

int loadConfigFromEEPROM(int arg_cnt, char **args)
//(char* tokens)
{
    char ch;
    Serial.println(F("loading Config"));

    ch = EEPROM.read(EEPROM_offset);
    if (ch == '{') {
        aJsonEEPROMStream as = aJsonEEPROMStream(EEPROM_offset);
        aJson.deleteItem(root);
        root = aJson.parse(&as);
        Serial.println();
        if (!root) {
            Serial.println(F("load failed"));
            return 0;
        }
        Serial.println(F("Loaded"));
        applyConfig();
        ethClient.stop(); //Refresh MQTT connect to get retained info
        return 1;
    } else {
        Serial.println(F("No stored config"));
        return 0;

    }
}

void cmdFunctionReq(int arg_cnt, char **args) {
    mqttConfigRequest(arg_cnt, args);
    restoreState();
}


int mqttConfigRequest(int arg_cnt, char **args)
//(char* tokens)
{
    char buf[25] = "/";
    Serial.println(F("request MQTT Config"));
    SetBytes((uint8_t *) mac, 6, buf + 1);
    buf[13] = 0;
    strncat(buf, "/resp/#", 25);
    Serial.println(buf);
    mqttClient.subscribe(buf);
    buf[13] = 0;
    strncat(buf, "/req/conf", 25);
    Serial.println(buf);
    mqttClient.publish(buf, "1");

}


int mqttConfigResp(char *as) {
    Serial.println(F("got MQTT Config"));

    //aJsonEEPROMStream as=aJsonEEPROMStream(EEPROM_offset);

    //aJson.deleteItem(root);
    root = aJson.parse(as);
    Serial.println();
    if (!root) {
        Serial.println(F("load failed"));
        return 0;
    }
    Serial.println(F("Loaded"));
    applyConfig();
    return 1;
}

void cmdFunctionSave(int arg_cnt, char **args)
//(char* tokens)
{
    aJsonEEPROMStream jsonEEPROMStream = aJsonEEPROMStream(EEPROM_offset);
    Serial.println(F("Saving config to EEPROM.."));
    aJson.print(root, &jsonEEPROMStream);
    jsonEEPROMStream.putEOF();
    Serial.println(F("Saved to EEPROM"));
}

void cmdFunctionIp(int arg_cnt, char **args)
//(char* tokens)
{
    IPAddress ip0(0, 0, 0, 0);
    IPAddress ip;
    DNSClient dns;
    switch (arg_cnt) {
        case 5:
            if (dns.inet_aton(args[4], ip)) saveFlash(OFFSET_MASK, ip);
            else saveFlash(OFFSET_MASK, ip0);
        case 4:
            if (dns.inet_aton(args[3], ip)) saveFlash(OFFSET_GW, ip);
            else saveFlash(OFFSET_GW, ip0);
        case 3:
            if (dns.inet_aton(args[2], ip)) saveFlash(OFFSET_DNS, ip);
            else saveFlash(OFFSET_DNS, ip0);
        case 2:
            if (dns.inet_aton(args[1], ip)) saveFlash(OFFSET_IP, ip);
            else saveFlash(OFFSET_IP, ip0);
            break;
        case 1:
            IPAddress current_ip = Ethernet.localIP();
            IPAddress current_mask = Ethernet.subnetMask();
            IPAddress current_gw = Ethernet.gatewayIP();
            IPAddress current_dns = Ethernet.dnsServerIP();
            saveFlash(OFFSET_IP, current_ip);
            saveFlash(OFFSET_MASK, current_mask);
            saveFlash(OFFSET_GW, current_gw);
            saveFlash(OFFSET_DNS, current_dns);
            Serial.print(F("Saved current config(ip,dns,gw,subnet):"));
            printIPAddress(current_ip);
            Serial.print(F(" ,"));
            printIPAddress(current_dns);
            Serial.print(F(" ,"));
            printIPAddress(current_gw);
            Serial.print(F(" ,"));
            printIPAddress(current_mask);
            Serial.println(F(";"));

    }
    Serial.println(F("Saved"));
}

void cmdFunctionClearEEPROM(int arg_cnt, char **args){
    for (int i = 0; i < 512; i++)
        EEPROM.write(i, 0);
    Serial.println(F("EEPROM cleared"));

}

void cmdFunctionPwd(int arg_cnt, char **args)
//(char* tokens)
{ char empty[]="";
  if (arg_cnt)
      saveFlash(OFFSET_MQTT_PWD,args[1]);
  else saveFlash(OFFSET_MQTT_PWD,empty);
  Serial.println(F("Password updated"));
    }

void cmdFunctionSetMac(int arg_cnt, char **args) {

    //Serial.print("Got:");
    //Serial.println(args[1]);
    if (sscanf(args[1], "%x:%x:%x:%x:%x:%x%с",
               &mac[0],
               &mac[1],
               &mac[2],
               &mac[3],
               &mac[4],
               &mac[5]) < 6) {
        Serial.print(F("could not parse: "));
        Serial.println(args[1]);
        return;
    }
    printMACAddress();
    for (short i = 0; i < 6; i++) { EEPROM.write(i, mac[i]); }
    Serial.println(F("Updated"));
}

void cmdFunctionGet(int arg_cnt, char **args) {
    lanStatus=getConfig(arg_cnt, args);
    ethClient.stop(); //Refresh MQTT connect to get retained info
    //restoreState();
}

void printBool(bool arg) { (arg) ? Serial.println(F("on")) : Serial.println(F("off")); }


void saveFlash(short n, char *str) {
  short i;
  short len=strlen(str);
  if (len>31) len=31;
  for(int i=0;i<len;i++) EEPROM.write(n+i,str[i]);
  EEPROM.write(n+len,0);
}

int loadFlash(short n, char *str, short l) {
short i;
uint8_t ch = EEPROM.read(n);
if (!ch || (ch == 0xff)) return 0;
  for (i=0;i<l-1 && (str[i] = EEPROM.read(n++));i++);
  str[i]=0;
return 1;
}

void saveFlash(short n, IPAddress& ip) {
  for(int i=0;i<4;i++) EEPROM.write(n++,ip[i]);
}

int ipLoadFromFlash(short n, IPAddress &ip) {
  for(int i=0;i<4;i++) ip[i]=EEPROM.read(n++);
  if (ip[0] && (ip[0] != 0xff)) return 1;
return 0;
}

lan_status getConfig(int arg_cnt, char **args)
{
    int responseStatusCode = 0;
    char ch;
    char URI[40];
    char configServer[32]="";
    if (arg_cnt > 1) {
        strncpy(configServer, args[1], sizeof(configServer) - 1);
        saveFlash(OFFSET_CONFIGSERVER, configServer);
    } else if (!loadFlash(OFFSET_CONFIGSERVER, configServer))
              strncpy_P(configServer,configserver,sizeof(configServer));

    snprintf(URI, sizeof(URI), "/%02x-%02x-%02x-%02x-%02x-%02x.config.json", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
    Serial.print(F("Config URI: http://"));
    Serial.print(configServer);
    Serial.println(URI);

#if defined(__AVR__)
    FILE *configStream;
    //byte hserver[] = { 192,168,88,2 };
    wdt_dis();
    HTTPClient hclient(configServer, 80);
    HTTPClient hclientPrint(configServer, 80);
    // FILE is the return STREAM type of the HTTPClient
    configStream = hclient.getURI(URI);
    responseStatusCode = hclient.getLastReturnCode();
    wdt_en();

    if (configStream != NULL) {
        if (responseStatusCode == 200) {

            Serial.println(F("got Config"));
            char c;
            aJsonFileStream as = aJsonFileStream(configStream);
            noInterrupts();
            aJson.deleteItem(root);
            root = aJson.parse(&as);
            interrupts();
        //    Serial.println(F("Parsed."));
            hclient.closeStream(configStream);  // this is very important -- be sure to close the STREAM

            if (!root) {
                Serial.println(F("Config parsing failed"));
                nextLanCheckTime = millis() + 15000;
                return READ_RE_CONFIG;//-11;
            } else {
            //    char *outstr = aJson.print(root);
            //    Serial.println(outstr);
            //    free(outstr);
            Serial.println(F("Applying."));
                applyConfig();


            }

        } else {
            Serial.print(F("ERROR: Server returned "));
            Serial.println(responseStatusCode);
            nextLanCheckTime = millis() + 5000;
            return READ_RE_CONFIG;//-11;
        }

    } else {
        Serial.println(F("failed to connect"));
        Serial.println(F(" try again in 5 seconds"));
        nextLanCheckTime = millis() + 5000;
        return READ_RE_CONFIG;//-11;
    }
#endif
#if defined(__SAM3X8E__)
    String response;
    EthernetClient configEthClient;
    HttpClient htclient = HttpClient(configEthClient, configServer, 80);
    //htclient.stop(); //_socket =MAX
    htclient.setHttpResponseTimeout(4000);
    wdt_res();
    //Serial.println("making GET request");get
    htclient.beginRequest();
    htclient.get(URI);
    htclient.endRequest();


    // read the status code and body of the response
    responseStatusCode = htclient.responseStatusCode();
    response = htclient.responseBody();
    htclient.stop();
    wdt_res();
    Serial.print(F("HTTP Status code: "));
    Serial.println(responseStatusCode);
    //Serial.print("GET Response: ");

    if (responseStatusCode == 200) {
        aJson.deleteItem(root);
        root = aJson.parse((char *) response.c_str());

        if (!root) {
            Serial.println(F("Config parsing failed"));
            // nextLanCheckTime=millis()+15000;
            return READ_RE_CONFIG;//-11; //Load from NVRAM
        } else {
            /*
            char * outstr=aJson.print(root);
            Serial.println(outstr);
            free (outstr);
             */
            Serial.println(response);
            applyConfig();


        }
    } else {
        Serial.println(F("Config retrieving failed"));
        //nextLanCheckTime=millis()+15000;
        return READ_RE_CONFIG;//-11; //Load from NVRAM
    }
#endif
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP8266)
    HTTPClient httpClient;
    String fullURI = "http://";
    fullURI+=configServer;
    fullURI+=URI;
    httpClient.begin(fullURI);
    int httpResponseCode = httpClient.GET();
    if (httpResponseCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpResponseCode);
        // file found at server
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = httpClient.getString();
            Serial.println(response);
            aJson.deleteItem(root);
            root = aJson.parse((char *) response.c_str());
            if (!root) {
                Serial.println(F("Config parsing failed"));
                return READ_RE_CONFIG;//-11; //Load from NVRAM
            } else {
                Serial.println(F("Config OK, Applying"));
                applyConfig();
            }
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
        httpClient.end();
        return READ_RE_CONFIG;//-11; //Load from NVRAM
    }
    httpClient.end();
#endif

    return IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
}

void preTransmission() {
#ifdef CONTROLLINO
// set DE and RE on HIGH
    PORTJ |= B01100000;
#else
    digitalWrite(TXEnablePin, 1);
#endif
}

void postTransmission() {
    #ifdef CONTROLLINO
    // set DE and RE on LOW
		PORTJ &= B10011111;
    #else
    digitalWrite(TXEnablePin, 0);
    #endif
}

void setup_main() {
    setupCmdArduino();
    printFirmwareVersionAndBuildOptions();

#ifdef SD_CARD_INSERTED
    sd_card_w5100_setup();
#endif
    setupMacAddress();
    loadConfigFromEEPROM(0, NULL);

#ifdef _modbus
#ifdef CONTROLLINO
    //set PORTJ pin 5,6 direction (RE,DE)
    DDRJ |= B01100000;
    //set RE,DE on LOW
    PORTJ &= B10011111;
#else
    pinMode(TXEnablePin, OUTPUT);
#endif
        modbusSerial.begin(MODBUS_SERIAL_BAUD);
        node.idle(&modbusIdle);
        node.preTransmission(preTransmission);
        node.postTransmission(postTransmission);
#endif

    delay(20);
    //owReady = 0;

#ifdef _owire
    if (net) net->idle(&owIdle);
#endif

    mqttClient.setCallback(mqttCallback);

#ifdef _artnet
    ArtnetSetup();
#endif

#if defined(ESP8266) and not defined(WIFI_MANAGER_DISABLE)
    WiFiManager wifiManager;
#if defined(ESP_WIFI_AP) and defined(ESP_WIFI_PWD)
    wifiManager.autoConnect(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));
#else
    wifiManager.autoConnect();
#endif
#endif

    delay(LAN_INIT_DELAY);//for LAN-shield initializing
    //TODO: checkForRemoteSketchUpdate();
}

#ifdef MMMMMM
errro!!!
#endif

void printFirmwareVersionAndBuildOptions() {
    Serial.print(F("\nLazyhome.ru LightHub controller "));
    Serial.println(F(QUOTE(PIO_SRC_REV)));
#ifdef CONTROLLINO
    Serial.println(F("(+)CONTROLLINO"));
#endif
#ifdef WATCH_DOG_TICKER_DISABLE
    Serial.println(F("(-)WATCHDOG"));
#else
    Serial.println(F("(+)WATCHDOG"));
#endif
    Serial.print(F("Config server:"));
    Serial.println(F(CONFIG_SERVER));
    Serial.print(F("Firmware MAC Address "));
    Serial.println(F(QUOTE(CUSTOM_FIRMWARE_MAC))); //Q Macros didn't working with 6 args
#ifdef DISABLE_FREERAM_PRINT
    Serial.println(F("(-)FreeRam printing"));
#else
    Serial.println(F("(+)FreeRam printing"));
#endif

#ifdef USE_1W_PIN
    Serial.print(F("(-)DS2482-100 USE_1W_PIN="));
    Serial.println(QUOTE(USE_1W_PIN));
#else
    Serial.println(F("(+)DS2482-100"));
#endif

#ifdef Wiz5500
    Serial.println(F("(+)WizNet5500"));
#endif

#ifdef DMX_DISABLE
    Serial.println(F("(-)DMX"));
#else
    Serial.println(F("(+)DMX"));
#endif

#ifdef MODBUS_DISABLE
    Serial.println(F("(-)MODBUS"));
#else
    Serial.println(F("(+)MODBUS"));
#endif

#ifdef OWIRE_DISABLE
    Serial.println(F("(-)OWIRE"));
#else
    Serial.println(F("(+)OWIRE"));
#endif
#ifndef DHT_DISABLE
    Serial.println(F("(+)DHT"));
#else
    Serial.println(F("(-)DHT"));
#endif

#ifdef Wiz5500
    Serial.println(F("(+)Wiz5500"));
#endif

#ifdef SD_CARD_INSERTED
    Serial.println(F("(+)SDCARD"));
#endif

#ifdef RESET_PIN
    Serial.print(F("(+)HARDRESET on pin="));
    Serial.println(F(QUOTE(RESET_PIN)));
#else
    Serial.println("(-)HARDRESET, using soft");
#endif

#ifdef RESTART_LAN_ON_MQTT_ERRORS
    Serial.println(F("(+)RESTART_LAN_ON_MQTT_ERRORS"));
#else
    Serial.println("(-)RESTART_LAN_ON_MQTT_ERRORS");
#endif
}

void setupMacAddress() {

#ifdef DEFAULT_FIRMWARE_MAC
    byte firmwareMacAddress[6] = DEFAULT_FIRMWARE_MAC;//comma(,) separated hex-array, hard-coded
#endif

#ifdef CUSTOM_FIRMWARE_MAC
    byte firmwareMacAddress[6];
    const char *macStr = QUOTE(CUSTOM_FIRMWARE_MAC);//colon(:) separated from build options
    parseBytes(macStr, ':', firmwareMacAddress, 6, 16);
#endif

    bool isMacValid = false;
    for (short i = 0; i < 6; i++) {
        mac[i] = EEPROM.read(i);
        if (mac[i] != 0 && mac[i] != 0xff) isMacValid = true;
    }
    if (!isMacValid) {
        Serial.println(F("Invalid MAC: set firmware's MAC"));
        memcpy(mac, firmwareMacAddress, 6);
    }
    printMACAddress();
}

void setupCmdArduino() {
    cmdInit(uint32_t(SERIAL_BAUD));
    Serial.println(F(">>>"));
    cmdAdd("help", cmdFunctionHelp);
    cmdAdd("save", cmdFunctionSave);
    cmdAdd("load", cmdFunctionLoad);
    cmdAdd("get", cmdFunctionGet);
    cmdAdd("mac", cmdFunctionSetMac);
    cmdAdd("kill", cmdFunctionKill);
    cmdAdd("req", cmdFunctionReq);
    cmdAdd("ip", cmdFunctionIp);
    cmdAdd("pwd", cmdFunctionPwd);
    cmdAdd("clear",cmdFunctionClearEEPROM);
    cmdAdd("reboot",cmdFunctionReboot);
}

void loop_main() {
    wdt_res();
    cmdPoll();
    if (lanLoop() > 1) {
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();
#endif
    }

#ifdef _owire
    if (owReady && owArr) owLoop();
#endif

#ifdef _dmxin
//    unsigned long lastpacket = DMXSerial.noDataSince();
    DMXCheck();
#endif
    // if (lastpacket && (lastpacket%10==0)) Serial.println(lastpacket);

    if (items) {
        #ifndef MODBUS_DISABLE
        if (lanStatus != RETAINING_COLLECTING) pollingLoop();
        #endif
#ifdef _owire
        thermoLoop();
#endif
    }


    inputLoop();

#if defined (_espdmx)
    dmxout.update();
#endif
}

void owIdle(void) {
#ifdef _artnet
    if (artnet) artnet->read();
#endif

    wdt_res();
    return; //TODO: unreached code
    Serial.print(F("o"));
    if (lanLoop() == 1) mqttClient.loop();
//if (owReady) owLoop();

#ifdef _dmxin
    DMXCheck();
#endif

#if defined (_espdmx)
    dmxout.update();
#endif
}
void ethernetIdle(void){
  wdt_res();
  inputLoop();
//  Serial.print(".");
  };

void modbusIdle(void) {
    wdt_res();
    if (lanLoop() > 1) {
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();
#endif
     inputLoop();
    }


#ifdef _dmxin
    DMXCheck();
#endif

#if defined (_espdmx)
    dmxout.update();
#endif
}

void inputLoop(void) {
    if (!inputs) return;
    if (millis() > nextInputCheck) {
        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.poll();
            }
            input = input->next;
        }
        nextInputCheck = millis() + INTERVAL_CHECK_INPUT;
    }
}

#ifndef MODBUS_DISABLE
void pollingLoop(void) {
    boolean done = false;
    if (millis() > nextPollingCheck) {
        while (pollingItem && !done) {
            if (pollingItem->type == aJson_Array) {
                Item it(pollingItem);
                nextPollingCheck = millis() + it.Poll();    //INTERVAL_CHECK_MODBUS;
                done = true;
            }//if
            pollingItem = pollingItem->next;
            if (!pollingItem) {
                pollingItem = items->child;
                return;
            } //start from 1-st element
        } //while
    }//if
}
#endif

bool isThermostatWithMinArraySize(aJsonObject *item, int minimalArraySize) {
    return (item->type == aJson_Array) && (aJson.getArrayItem(item, I_TYPE)->valueint == CH_THERMO) &&
           (aJson.getArraySize(item) >= minimalArraySize);
}

bool thermoDisabledOrDisconnected(aJsonObject *thermoExtensionArray, int thermoStateCommand) {
    return thermoStateCommand == CMD_OFF || thermoStateCommand == CMD_HALT ||
           aJson.getArrayItem(thermoExtensionArray, IET_ATTEMPTS)->valueint == 0;
}


//TODO: refactoring
void thermoLoop(void) {
    if (millis() < nextThermostatCheck)
        return;
    bool thermostatCheckPrinted = false;

    for (aJsonObject *thermoItem = items->child; thermoItem; thermoItem = thermoItem->next) {
        if (isThermostatWithMinArraySize(thermoItem, 5)) {
            aJsonObject *thermoExtensionArray = aJson.getArrayItem(thermoItem, I_EXT);
            if (thermoExtensionArray && (aJson.getArraySize(thermoExtensionArray) > 1)) {
                int thermoPin = aJson.getArrayItem(thermoItem, I_ARG)->valueint;
                int thermoSetting = aJson.getArrayItem(thermoItem, I_VAL)->valueint;
                int thermoStateCommand = aJson.getArrayItem(thermoItem, I_CMD)->valueint;
                int curTemp = aJson.getArrayItem(thermoExtensionArray, IET_TEMP)->valueint;

                if (!aJson.getArrayItem(thermoExtensionArray, IET_ATTEMPTS)->valueint) {
                    Serial.print(thermoItem->name);
                    Serial.println(F(" Expired"));

                } else {
                    if (!(--aJson.getArrayItem(thermoExtensionArray, IET_ATTEMPTS)->valueint))
                        mqttClient.publish("/alarm/snsr", thermoItem->name);

                }
                if (curTemp > THERMO_OVERHEAT_CELSIUS) mqttClient.publish("/alarm/ovrht", thermoItem->name);


                Serial.print(thermoItem->name);
                Serial.print(F(" Set:"));
                Serial.print(thermoSetting);
                Serial.print(F(" Cur:"));
                Serial.print(curTemp);
                Serial.print(F(" cmd:"));
                Serial.print(thermoStateCommand);
                pinMode(thermoPin, OUTPUT);
                if (thermoDisabledOrDisconnected(thermoExtensionArray, thermoStateCommand)) {
                    digitalWrite(thermoPin, LOW);
                    Serial.println(F(" OFF"));
                } else {
                    if (curTemp < thermoSetting - THERMO_GIST_CELSIUS) {
                        digitalWrite(thermoPin, HIGH);
                        Serial.println(F(" ON"));
                    } //too cold
                    else if (curTemp >= thermoSetting) {
                        digitalWrite(thermoPin, LOW);
                        Serial.println(F(" OFF"));
                    } //Reached settings
                    else Serial.println(F(" -target zone-")); // Nothing to do
                }
                thermostatCheckPrinted = true;
            }
        }
    }

    nextThermostatCheck = millis() + THERMOSTAT_CHECK_PERIOD;

#ifndef DISABLE_FREERAM_PRINT
    (thermostatCheckPrinted) ? Serial.print(F("\nfree:")) : Serial.print(F(" "));
    Serial.print(freeRam());
    Serial.print(" ");
#endif
}

short thermoSetCurTemp(char *name, short t) {
    if (items) {
        aJsonObject *thermoItem = aJson.getObjectItem(items, name);
        if (isThermostatWithMinArraySize(thermoItem, 4)) {
            aJsonObject *extArray = NULL;

            if (aJson.getArraySize(thermoItem) == 4) //No thermo extension yet
            {
                extArray = aJson.createArray(); //Create Ext Array

                aJsonObject *ocurt = aJson.createItem(t);  //Create int
                aJsonObject *oattempts = aJson.createItem(T_ATTEMPTS); //Create int
                aJson.addItemToArray(extArray, ocurt);
                aJson.addItemToArray(extArray, oattempts);
                aJson.addItemToArray(thermoItem, extArray); //Adding to thermoItem
            }
            else if (extArray = aJson.getArrayItem(thermoItem, I_EXT)) {
                aJsonObject *att = aJson.getArrayItem(extArray, IET_ATTEMPTS);
                aJson.getArrayItem(extArray, IET_TEMP)->valueint = t;
                if (att->valueint == 0) mqttClient.publish("/alarmoff/snsr", thermoItem->name);
                att->valueint = (int) T_ATTEMPTS;
            }

        }
    }

}

