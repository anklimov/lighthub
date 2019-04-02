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

#include "main.h"


#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
EthernetClient ethClient;
#endif

#if defined(ARDUINO_ARCH_AVR)
EthernetClient ethClient;
#endif

#ifdef ARDUINO_ARCH_ESP8266
WiFiClient ethClient;

#if not defined(WIFI_MANAGER_DISABLE)
    WiFiManager wifiManager;
#endif

#endif

#ifdef ARDUINO_ARCH_ESP32
WiFiClient ethClient;
NRFFlashStorage EEPROM;

#if not defined(WIFI_MANAGER_DISABLE)
    WiFiManager wifiManager;
#endif

#endif

#ifdef ARDUINO_ARCH_STM32
EthernetClient ethClient;
NRFFlashStorage EEPROM;
#endif

#ifdef NRF5
NRFFlashStorage EEPROM;
EthernetClient ethClient;
#endif

#ifdef SYSLOG_ENABLE
#include <Syslog.h>
EthernetUDP udpSyslogClient;
Syslog udpSyslog(udpSyslogClient, SYSLOG_PROTO_IETF);
unsigned long nextSyslogPingTime;
#endif

lan_status lanStatus = INITIAL_STATE;


const char configserver[] PROGMEM = CONFIG_SERVER;
const char verval_P[] PROGMEM = QUOTE(PIO_SRC_REV);


unsigned int UniqueID[5] = {0,0,0,0,0};
char *deviceName = NULL;
aJsonObject *topics = NULL;
aJsonObject *root = NULL;
aJsonObject *items = NULL;
aJsonObject *inputs = NULL;

aJsonObject *mqttArr = NULL;
#ifndef MODBUS_DISABLE
aJsonObject *modbusArr = NULL;
#endif
#ifdef _owire
aJsonObject *owArr = NULL;
#endif
#ifdef _dmxout
aJsonObject *dmxArr = NULL;
#endif
#ifdef SYSLOG_ENABLE
aJsonObject *udpSyslogArr = NULL;
#endif

uint32_t nextPollingCheck = 0;
uint32_t nextInputCheck = 0;
uint32_t nextLanCheckTime = 0;
uint32_t nextThermostatCheck = 0;
uint32_t nextSensorCheck =0;

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

#if defined(__SAM3X8E__)
void watchdogSetup(void) {}    //Do not remove - strong re-definition WDT Init for DUE
#endif

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    debugSerial<<F("\n[")<<topic<<F("] ");
    if (!payload) return;

    payload[length] = 0;
    int fr = freeRam();
    if (fr < 250) {
        debugSerial<<F("OOM!");
        return;
    }
    for (int i = 0; i < length; i++)
        debugSerial<<((char) payload[i]);
    debugSerial<<endl;

    short intopic = 0;
    short pfxlen  = 0;
    char * itemName = NULL;
    char * subItem = NULL;

{
char buf[MQTT_TOPIC_LENGTH + 1];


if  (lanStatus == RETAINING_COLLECTING)
{
  setTopic(buf,sizeof(buf),T_OUT);
  pfxlen = strlen(buf);
  intopic = strncmp(topic, buf, pfxlen);
}

else
{
       setTopic(buf,sizeof(buf),T_BCST);
        pfxlen = strlen(buf);
        intopic = strncmp(topic, buf, pfxlen );

        if (intopic)
        {
        setTopic(buf,sizeof(buf),T_DEV);
        pfxlen = strlen(buf);
        intopic = strncmp(topic, buf, pfxlen);
        }
}
}
    // in Retaining status - trying to restore previous state from retained output topic. Retained input topics are not relevant.
    if (intopic) {
        debugSerial<<F("Skipping..");
        return;
    }

    itemName=topic+pfxlen;

    if(!strcmp(itemName,CMDTOPIC)) {
        cmd_parse((char *)payload);
        return;
    }

    if (subItem = strchr(itemName, '/'))
      {
        *subItem = 0;
        subItem++;
        if (*subItem=='$') return; //Skipping homie stuff

        //  debugSerial<<F("Subitem:")<<subItem<<endl;
      }
       // debugSerial<<F("Item:")<<itemName<<endl;

    if (itemName[0]=='$') return; //Skipping homie stuff

    Item item(itemName);
    if (item.isValid()) {
        if (item.itemType == CH_GROUP && (lanStatus == RETAINING_COLLECTING))
            return; //Do not restore group channels - they consist not relevant data
        item.Ctrl((char *)payload, !(lanStatus == RETAINING_COLLECTING),subItem);
    } //valid item
}



void printMACAddress() {
    debugSerial<<F("MAC:");
    for (byte i = 0; i < 6; i++)
#ifdef WITH_PRINTEX_LIB
        (i < 5) ?debugSerial<<hex <<(mac[i])<<F(":"):debugSerial<<hex<<(mac[i])<<endl;
#else
        (i < 5) ?debugSerial<<_HEX(mac[i])<<F(":"):debugSerial<<_HEX(mac[i])<<endl;
#endif
}


lan_status lanLoop() {

    #ifdef NOETHER
    lanStatus=DO_NOTHING;//-14;
    #endif

    switch (lanStatus) {
        case INITIAL_STATE:
            if (millis() > nextLanCheckTime)
                onInitialStateInitLAN();
            break;

        case HAVE_IP_ADDRESS:
            if (!configOk)
                lanStatus = loadConfigFromHttp(0, NULL);
            else lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
            break;

        case IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER:
            wdt_res();
            ip_ready_config_loaded_connecting_to_broker();
            break;

        case RETAINING_COLLECTING:
            if (millis() > nextLanCheckTime) {
                char buf[MQTT_TOPIC_LENGTH];

                //Unsubscribe from status topics..
                //strncpy_P(buf, outprefix, sizeof(buf));
                setTopic(buf,sizeof(buf),T_OUT);
                strncat(buf, "#", sizeof(buf));
                mqttClient.unsubscribe(buf);

                lanStatus = OPERATION;//3;
                debugSerial<<F("Accepting commands...\n");
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
            if (loadConfigFromEEPROM()) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            else {
                nextLanCheckTime = millis() + 5000;
                lanStatus = AWAITING_ADDRESS;//-10;
            }
            break;

        case DO_NOTHING:;
    }


    {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
if (WiFi.status() != WL_CONNECTED)
         {
          wifiInitialized=false;
          lanStatus = INITIAL_STATE;
        }
#endif

#if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__)
        wdt_dis();
        if (lanStatus > 0)
            switch (Ethernet.maintain()) {
                   case NO_LINK:
                    debugSerial<<F("No link")<<endl;
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 30000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;
                case DHCP_CHECK_RENEW_FAIL:
                    debugSerial<<F("Error: renewed fail");
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 1000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;

                case DHCP_CHECK_RENEW_OK:
                    debugSerial<<F("Renewed success. IP address:");
                    printIPAddress(Ethernet.localIP());
                    break;

                case DHCP_CHECK_REBIND_FAIL:
                    debugSerial<<F("Error: rebind fail");
                    if (mqttClient.connected()) mqttClient.disconnect();
                    nextLanCheckTime = millis() + 1000;
                    lanStatus = AWAITING_ADDRESS;//-10;
                    break;

                case DHCP_CHECK_REBIND_OK:
                    debugSerial<<F("Rebind success. IP address:");
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


void onMQTTConnect(){
  char topic[64] = "";
  char buf[128] = "";

  // High level homie topics publishing
  //strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, state_P, sizeof(topic));
  strncpy_P(buf, ready_P, sizeof(buf));
  mqttClient.publish(topic,buf,true);

  //strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, name_P, sizeof(topic));
  strncpy_P(buf, nameval_P, sizeof(buf));
  strncat_P(buf,(verval_P),sizeof(buf));
  mqttClient.publish(topic,buf,true);

  //strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, stats_P, sizeof(topic));
  strncpy_P(buf, statsval_P, sizeof(buf));
  mqttClient.publish(topic,buf,true);

  #ifndef NO_HOMIE

//  strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, homie_P, sizeof(topic));
  strncpy_P(buf, homiever_P, sizeof(buf));
  mqttClient.publish(topic,buf,true);

  if (items) {
    char datatype[32]="\0";
    char format [64]="\0";
      aJsonObject * item = items->child;
      while (items && item)
          if (item->type == aJson_Array && aJson.getArraySize(item)>0) {
///              strncat(buf,item->name,sizeof(buf));
///              strncat(buf,",",sizeof(buf));

                  switch (  aJson.getArrayItem(item, I_TYPE)->valueint) {
                      case CH_THERMO:
                      strncpy_P(datatype,float_P,sizeof(datatype));
                      format[0]=0;
                      break;

                      case CH_RELAY:
                      case CH_GROUP:
                      strncpy_P(datatype,enum_P,sizeof(datatype));
                      strncpy_P(format,enumformat_P,sizeof(format));
                      break;

                      case  CH_RGBW:
                      case  CH_RGB:
                      strncpy_P(datatype,color_P,sizeof(datatype));
                      strncpy_P(format,hsv_P,sizeof(format));
                      break;
                      case  CH_DIMMER:
                      case  CH_MODBUS:
                      case  CH_PWM:
                      case  CH_VCTEMP:
                      case  CH_VC:
                      strncpy_P(datatype,int_P,sizeof(datatype));
                      strncpy_P(format,intformat_P,sizeof(format));
                      break;
                  } //switch

                  //strncpy_P(topic, outprefix, sizeof(topic));
                  setTopic(topic,sizeof(topic),T_DEV);

                  strncat(topic,item->name,sizeof(topic));
                  strncat(topic,"/",sizeof(topic));
                  strncat_P(topic,datatype_P,sizeof(topic));
                  mqttClient.publish(topic,datatype,true);

                  if (format[0])
                  {
                  //strncpy_P(topic, outprefix, sizeof(topic));
                  setTopic(topic,sizeof(topic),T_DEV);

                  strncat(topic,item->name,sizeof(topic));
                  strncat(topic,"/",sizeof(topic));
                  strncat_P(topic,format_P,sizeof(topic));
                  mqttClient.publish(topic,format,true);
                  }
              item = item->next;
          }  //if
          //strncpy_P(topic, outprefix, sizeof(topic));
          setTopic(topic,sizeof(topic),T_DEV);
          strncat_P(topic, nodes_P, sizeof(topic));
    ///      mqttClient.publish(topic,buf,true);
  }
#endif
}

void ip_ready_config_loaded_connecting_to_broker() {
    short n = 0;
    int port = 1883;
    char empty = 0;
    char *user = &empty;
    char passwordBuf[16] = "";
    char *password = passwordBuf;
    int syslogPort = 514;
    char syslogDeviceHostname[16];
    if (mqttArr && (aJson.getArraySize(mqttArr)))
      {
                deviceName = aJson.getArrayItem(mqttArr, 0)->valuestring;
                debugSerial<<F("Device Name:")<<deviceName<<endl;
              }
#ifdef SYSLOG_ENABLE
    //debugSerial<<"debugSerial:";
    delay(100);
    if (udpSyslogArr && (n = aJson.getArraySize(udpSyslogArr))) {
      char *syslogServer = aJson.getArrayItem(udpSyslogArr, 0)->valuestring;
      if (n>1) syslogPort = aJson.getArrayItem(udpSyslogArr, 1)->valueint;

       inet_ntoa_r(Ethernet.localIP(),syslogDeviceHostname,sizeof(syslogDeviceHostname));
/*
      char *syslogDeviceHostname = aJson.getArrayItem(udpSyslogArr, 2)->valuestring;
      char *syslogAppname = aJson.getArrayItem(udpSyslogArr, 3)->valuestring;
*/
      debugSerial<<F("Syslog params:")<<syslogServer<<":"<<syslogPort<<":"<<syslogDeviceHostname<<":"<<deviceName<<endl;
      udpSyslog.server(syslogServer, syslogPort);
      udpSyslog.deviceHostname(syslogDeviceHostname);
      if (deviceName) udpSyslog.appName(deviceName);
      udpSyslog.defaultPriority(LOG_KERN);
      udpSyslog.log(LOG_INFO, F("UDP Syslog initialized!"));
        debugSerial<<F("UDP Syslog initialized!\n");
    }
#endif

    if (!mqttClient.connected() && mqttArr && ((n = aJson.getArraySize(mqttArr)) > 1)) {
    //    char *client_id = aJson.getArrayItem(mqttArr, 0)->valuestring;
        char *servername = aJson.getArrayItem(mqttArr, 1)->valuestring;
        if (n >= 3) port = aJson.getArrayItem(mqttArr, 2)->valueint;
        if (n >= 4) user = aJson.getArrayItem(mqttArr, 3)->valuestring;
        if (!loadFlash(OFFSET_MQTT_PWD, passwordBuf, sizeof(passwordBuf)) && (n >= 5)) {
            password = aJson.getArrayItem(mqttArr, 4)->valuestring;
            debugSerial<<F("Using MQTT password from config");
        }

        mqttClient.setServer(servername, port);
        mqttClient.setCallback(mqttCallback);

        char willMessage[16];
        char willTopic[32];

        strncpy_P(willMessage,disconnected_P,sizeof(willMessage));

      //  strncpy_P(willTopic, outprefix, sizeof(willTopic));
        setTopic(willTopic,sizeof(willTopic),T_DEV);

        strncat_P(willTopic, state_P, sizeof(willTopic));


        debugSerial<<F("\nAttempting MQTT connection to ")<<servername<<F(":")<<port<<F(" user:")<<user<<F(" ...");
        wdt_dis();  //potential unsafe for ethernetIdle(), but needed to avoid cyclic reboot if mosquitto out of order
        if (mqttClient.connect(deviceName, user, password,willTopic,MQTTQOS1,true,willMessage)) {
            mqttErrorRate = 0;
            debugSerial<<F("connected as ")<<deviceName <<endl;
            wdt_en();
            configOk = true;
            // ... Temporary subscribe to status topic
            char buf[MQTT_TOPIC_LENGTH];

  //          strncpy_P(buf, outprefix, sizeof(buf));
            setTopic(buf,sizeof(buf),T_OUT);
            strncat(buf, "#", sizeof(buf));
            mqttClient.subscribe(buf);

            //Subscribing for command topics
            //strncpy_P(buf, inprefix, sizeof(buf));
            setTopic(buf,sizeof(buf),T_BCST);
            strncat(buf, "#", sizeof(buf));
            Serial.println(buf);
            mqttClient.subscribe(buf);

            setTopic(buf,sizeof(buf),T_DEV);
            strncat(buf, "#", sizeof(buf));
            Serial.println(buf);
            mqttClient.subscribe(buf);

            //restoreState();
            onMQTTConnect();
            // if (_once) {DMXput(); _once=0;}
            lanStatus = RETAINING_COLLECTING;//4;
            nextLanCheckTime = millis() + 5000;
            debugSerial<<F("Awaiting for retained topics");
        } else {
            debugSerial<<F("failed, rc=")<<mqttClient.state()<<F(" try again in 5 seconds");
            nextLanCheckTime = millis() + 5000;
#ifdef RESTART_LAN_ON_MQTT_ERRORS
            mqttErrorRate++;
                        if(mqttErrorRate>50){
                            debugSerial<<F("Too many MQTT connection errors. Restart LAN"));
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
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#if defined(WIFI_MANAGER_DISABLE)
    if(WiFi.status() != WL_CONNECTED) {
                WiFi.mode(WIFI_STA); // ESP 32 - WiFi.disconnect(); instead
                debugSerial<<F("WIFI AP/Password:")<<QUOTE(ESP_WIFI_AP)<<F("/")<<QUOTE(ESP_WIFI_PWD)<<endl;

                wifi_set_macaddr(STATION_IF,mac); //ESP32 to check

                WiFi.begin(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));
                int wifi_connection_wait = 10000;

                while (WiFi.status() != WL_CONNECTED && wifi_connection_wait > 0) {
                    delay(500);
                    wifi_connection_wait -= 500;
                    debugSerial<<".";
                }
                wifiInitialized = true;
            }
#else
// Wifi Manager
if (WiFi.status() != WL_CONNECTED)
{
//WiFi.disconnect();

#if defined(ESP_WIFI_AP) and defined(ESP_WIFI_PWD)
    wifiInitialized = wifiManager.autoConnect(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));
#else
    wifiInitialized = wifiManager.autoConnect();
#endif
}
#endif
#endif

/*
#if defined(ARDUINO_ARCH_ESP32) and defined(WIFI_MANAGER_DISABLE)
    if(!wifiInitialized) {
    //    WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        debugSerial<<F("WIFI AP/Password:")<<QUOTE(ESP_WIFI_AP)<<F("/")<<QUOTE(ESP_WIFI_PWD)<<endl;
        WiFi.begin(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));

        int wifi_connection_wait = 10000;
        while (WiFi.status() != WL_CONNECTED && wifi_connection_wait > 0) {
            delay(500);
            wifi_connection_wait -= 500;
            debugSerial<<".";
        }
        wifiInitialized = true;
    }
#endif
*/

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if (WiFi.status() == WL_CONNECTED) {
        debugSerial<<F("WiFi connected. IP address: ")<<WiFi.localIP()<<endl;
        lanStatus = HAVE_IP_ADDRESS;//1;
    } else
    {
        debugSerial<<F("Problem with WiFi!");
        nextLanCheckTime = millis() + DHCP_RETRY_INTERVAL;
    }
#endif

#if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__)||defined(ARDUINO_ARCH_STM32)
#ifdef W5500_CS_PIN
    Ethernet.w5500_cspin = W5500_CS_PIN;
    debugSerial<<F("Use W5500 pin: ");
    debugSerial<<(Ethernet.w5500_cspin);
#endif
    IPAddress ip, dns, gw, mask;
    int res = 1;
    debugSerial<<F("Starting lan");
    if (ipLoadFromFlash(OFFSET_IP, ip)) {
        debugSerial<<"Loaded from flash IP:";
        printIPAddress(ip);
        if (ipLoadFromFlash(OFFSET_DNS, dns)) {
            debugSerial<<" DNS:";
            printIPAddress(dns);
            if (ipLoadFromFlash(OFFSET_GW, gw)) {
                debugSerial<<" GW:";
                printIPAddress(gw);
                if (ipLoadFromFlash(OFFSET_MASK, mask)) {
                    debugSerial<<" MASK:";
                    printIPAddress(mask);
                    Ethernet.begin(mac, ip, dns, gw, mask);
                } else Ethernet.begin(mac, ip, dns, gw);
            } else Ethernet.begin(mac, ip, dns);
        } else Ethernet.begin(mac, ip);
    debugSerial<<endl;
    lanStatus = HAVE_IP_ADDRESS;
    }
    else {
        debugSerial<<"\nNo IP data found in flash\n";
        wdt_dis();
#if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__)
        res = Ethernet.begin(mac, 12000);
#endif
#if defined(ARDUINO_ARCH_STM32)
        res = Ethernet.begin(mac);
#endif
        wdt_en();
        wdt_res();


    if (res == 0) {
        debugSerial<<F("Failed to configure Ethernet using DHCP. You can set ip manually!")<<F("'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n");
        lanStatus = AWAITING_ADDRESS;//-10;
        nextLanCheckTime = millis() + DHCP_RETRY_INTERVAL;
#ifdef RESET_PIN
        resetHard();
#endif
    } else {
        debugSerial<<F("Got IP address:");
        printIPAddress(Ethernet.localIP());
        lanStatus = HAVE_IP_ADDRESS;//1;
        #ifdef _artnet
                    if (artnet) artnet->begin();
        #endif
    }
  }
    #endif
}


void resetHard() {
#ifdef RESET_PIN
    debugSerial<<F("Reset Arduino with digital pin ");
    debugSerial<<QUOTE(RESET_PIN);
    delay(500);
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN,LOW);
    delay(500);
    digitalWrite(RESET_PIN,HIGH);
    delay(500);
#endif
}

#ifdef _owire

void Changed(int i, DeviceAddress addr, float currentTemp) {
    char addrstr[32] = "NIL";
    //char addrbuf[17];
    char valstr[16] = "NIL";
    char *owEmitString = NULL;
    char *owItem = NULL;

    SetBytes(addr, 8, addrstr);
    addrstr[17] = 0;

    printFloatValueToStr(currentTemp,valstr);
    debugSerial<<endl<<F("T:")<<valstr<<F("<");
    aJsonObject *owObj = aJson.getObjectItem(owArr, addrstr);
    if (owObj) {
        owEmitString = aJson.getObjectItem(owObj, "emit")->valuestring;
        debugSerial<<owEmitString<<F(">")<<endl;
          if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
          {
           if (owEmitString)      // publish temperature to MQTT if configured
            {

#ifdef WITH_DOMOTICZ
            aJsonObject *idx = aJson.getObjectItem(owObj, "idx");
        if (idx && idx->valuestring) {//DOMOTICZ json format support
            debugSerial << endl << idx->valuestring << F(" Domoticz valstr:");
            char valstr[50];
            sprintf(valstr, "{\"idx\":%s,\"svalue\":\"%.1f\"}", idx->valuestring, currentTemp);
            debugSerial << valstr;
            mqttClient.publish(owEmitString, valstr);
            return;
        }
#endif

            //strcpy_P(addrstr, outprefix);
            setTopic(addrstr,sizeof(addrstr),T_OUT);
            strncat(addrstr, owEmitString, sizeof(addrstr));
            mqttClient.publish(addrstr, valstr);
        }
        // And translate temp to internal items
        owItem = aJson.getObjectItem(owObj, "item")->valuestring;
        if (owItem)
            thermoSetCurTemp(owItem, currentTemp);  ///TODO: Refactore using Items interface
        } // if valid temperature
  } // if Address in config
  else debugSerial<<addrstr<<F(">")<<endl; // No item found

}

#endif //_owire

void cmdFunctionHelp(int arg_cnt, char **args)
{
    printFirmwareVersionAndBuildOptions();
    #ifdef SYSLOG_ENABLE
//    udpSyslog.logf(LOG_INFO, "free RAM: %d",freeRam());
    #endif
    printCurentLanConfig();
//    printFreeRam();
    debugSerial<<F("\nUse these commands: 'help' - this text\n"
                          "'mac de:ad:be:ef:fe:00' set and store MAC-address in EEPROM\n"
                          "'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n"
                          "'save' - save config in NVRAM\n"
                          "'get' [config addr]' - get config from pre-configured URL and store addr\n"
                          "'load' - load config from NVRAM\n"
                          "'pwd' - define MQTT password\n"
                          "'kill' - test watchdog\n"
                          "'clear' - clear EEPROM\n"
                          "'reboot' - reboot controller");
}
void printCurentLanConfig() {
    debugSerial << F("Current LAN config(ip,dns,gw,subnet):");
    printIPAddress(Ethernet.localIP());
//    printIPAddress(Ethernet.dnsServerIP());
    printIPAddress(Ethernet.gatewayIP());
    printIPAddress(Ethernet.subnetMask());
}

void cmdFunctionKill(int arg_cnt, char **args) {
    for (byte i = 1; i < 20; i++) {
        delay(1000);
        debugSerial<<i;
    };
}

void cmdFunctionReboot(int arg_cnt, char **args) {
    debugSerial<<F("Soft rebooting...");
    softRebootFunc();
}

void applyConfig() {
    if (!root) return;
#ifdef _dmxin
    int itemsCount;
    dmxArr = aJson.getObjectItem(root, "dmxin");
    if (dmxArr && (itemsCount = aJson.getArraySize(dmxArr))) {
        DMXinSetup(itemsCount * 4);
        debugSerial<<F("DMX in started. Channels:")<<itemsCount * 4<<endl;
    }
#endif
#ifdef _dmxout
    int maxChannels;
    aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
    if (dmxoutArr && aJson.getArraySize(dmxoutArr) >=1 ) {
        DMXoutSetup(maxChannels = aJson.getArrayItem(dmxoutArr, 1)->valueint);
        debugSerial<<F("DMX out started. Channels: ")<<maxChannels<<endl;
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
        if (owReady) debugSerial<<F("One wire Ready\n");
        t_count = 0;
        while (item && owReady) {
            if ((item->type == aJson_Object)) {
                DeviceAddress addr;
                //debugSerial<<F("Add:")),debugSerial<<item->name);
                SetAddr(item->name, addr);
                owAdd(addr);
            }
            item = item->next;
        }
    }
#endif
    items = aJson.getObjectItem(root, "items");
    topics = aJson.getObjectItem(root, "topics");

// Digital output related Items initialization
    pollingItem=NULL;
    if (items) {
        aJsonObject * item = items->child;
        while (items && item)
            if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
                Item it(item);
                if (it.isValid()) {
                    int pin=it.getArg();
                    if (pin<0) pin=-pin;
                    int cmd = it.getCmd();
                    switch (it.itemType) {
                        case CH_THERMO:
                            if (cmd<1) it.setCmd(CMD_OFF);
                        case CH_RELAY:
                        {
                            int k;
                            pinMode(pin, OUTPUT);
                            digitalWrite(pin, k = ((cmd == CMD_ON) ? HIGH : LOW));
                            debugSerial<<F("Pin:")<<pin<<F("=")<<k<<F(",");
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

   inputSetup();
#ifdef SYSLOG_ENABLE
    udpSyslogArr = aJson.getObjectItem(root, "syslog");
#endif
    printConfigSummary();
}

void printConfigSummary() {
    debugSerial<<F("\nConfigured:")<<F("\nitems ");
    printBool(items);
    debugSerial<<F("\ninputs ");
    printBool(inputs);
#ifndef MODBUS_DISABLE
    debugSerial<<F("\nmodbus ");
    printBool(modbusArr);
#endif
    debugSerial<<F("\nmqtt ");
    printBool(mqttArr);
#ifdef _owire
    debugSerial<<F("\n1-wire ");
    printBool(owArr);
#endif
#ifdef SYSLOG_ENABLE
    debugSerial<<F("\nudp syslog ");
    printBool(udpSyslogArr);
#endif
    debugSerial << endl;
}

void cmdFunctionLoad(int arg_cnt, char **args) {
    loadConfigFromEEPROM();
  //  restoreState();
}

int loadConfigFromEEPROM()
{
    char ch;
    debugSerial<<F("loading Config");

    ch = EEPROM.read(EEPROM_offset);
    if (ch == '{') {
        aJsonEEPROMStream as = aJsonEEPROMStream(EEPROM_offset);
        aJson.deleteItem(root);
        root = aJson.parse(&as);
        if (!root) {
            debugSerial<<F("\nload failed\n");
            return 0;
        }
        debugSerial<<F("\nLoaded\n");
        applyConfig();
        ethClient.stop(); //Refresh MQTT connect to get retained info
        return 1;
    } else {
        debugSerial<<F("\nNo stored config\n");
        return 0;
    }
    return 0;
}

void cmdFunctionReq(int arg_cnt, char **args) {
    mqttConfigRequest(arg_cnt, args);
  //  restoreState();
}


int mqttConfigRequest(int arg_cnt, char **args)
{
    char buf[25] = "/";
    debugSerial<<F("\nrequest MQTT Config");
    SetBytes((uint8_t *) mac, 6, buf + 1);
    buf[13] = 0;
    strncat(buf, "/resp/#", 25);
    debugSerial<<buf;
    mqttClient.subscribe(buf);
    buf[13] = 0;
    strncat(buf, "/req/conf", 25);
    debugSerial<<buf;
    mqttClient.publish(buf, "1");
    return 1;
}


int mqttConfigResp(char *as) {
    debugSerial<<F("got MQTT Config");
    root = aJson.parse(as);

    if (!root) {
        debugSerial<<F("\nload failed\n");
        return 0;
    }
    debugSerial<<F("\nLoaded");
    applyConfig();
    return 1;
}

void cmdFunctionSave(int arg_cnt, char **args)
{
    aJsonEEPROMStream jsonEEPROMStream = aJsonEEPROMStream(EEPROM_offset);
    debugSerial<<F("Saving config to EEPROM..");
    aJson.print(root, &jsonEEPROMStream);
    jsonEEPROMStream.putEOF();
    debugSerial<<F("Saved to EEPROM");
}

void cmdFunctionIp(int arg_cnt, char **args)
{
    IPAddress ip0(0, 0, 0, 0);
    IPAddress ip;
/*
    #if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__) || defined(NRF5)
    DNSClient dns;
    #define inet_aton(cp, addr)   dns.inet_aton(cp, addr)
    #else
    #define inet_aton(cp, addr)   inet_aton(cp, addr)
    #endif
*/
    switch (arg_cnt) {
        case 5:
            if (inet_aton(args[4], ip)) saveFlash(OFFSET_MASK, ip);
            else saveFlash(OFFSET_MASK, ip0);
        case 4:
            if (inet_aton(args[3], ip)) saveFlash(OFFSET_GW, ip);
            else saveFlash(OFFSET_GW, ip0);
        case 3:
            if (inet_aton(args[2], ip)) saveFlash(OFFSET_DNS, ip);
            else saveFlash(OFFSET_DNS, ip0);
        case 2:
            if (inet_aton(args[1], ip)) saveFlash(OFFSET_IP, ip);
            else saveFlash(OFFSET_IP, ip0);
            break;

        case 1: //dynamic IP
        saveFlash(OFFSET_IP,ip0);
        debugSerial<<F("Set dynamic IP\n");
/*
            IPAddress current_ip = Ethernet.localIP();
            IPAddress current_mask = Ethernet.subnetMask();
            IPAddress current_gw = Ethernet.gatewayIP();
            IPAddress current_dns = Ethernet.dnsServerIP();
            saveFlash(OFFSET_IP, current_ip);
            saveFlash(OFFSET_MASK, current_mask);
            saveFlash(OFFSET_GW, current_gw);
            saveFlash(OFFSET_DNS, current_dns);
            debugSerial<<F("Saved current config(ip,dns,gw,subnet):");
            printIPAddress(current_ip);
            printIPAddress(current_dns);
            printIPAddress(current_gw);
            printIPAddress(current_mask); */
    }
    debugSerial<<F("Saved\n");
}

void cmdFunctionClearEEPROM(int arg_cnt, char **args){
    for (int i = 0; i < 512; i++)
        EEPROM.write(i, 0);
    debugSerial<<F("EEPROM cleared\n");

}

void cmdFunctionPwd(int arg_cnt, char **args)
{ char empty[]="";
    if (arg_cnt)
        saveFlash(OFFSET_MQTT_PWD,args[1]);
    else saveFlash(OFFSET_MQTT_PWD,empty);
    debugSerial<<F("Password updated\n");
}

void cmdFunctionSetMac(int arg_cnt, char **args) {
    if (sscanf(args[1], "%x:%x:%x:%x:%x:%x%с", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) < 6) {
        debugSerial<<F("could not parse: ")<<args[1];
        return;
    }
    printMACAddress();
    for (short i = 0; i < 6; i++) { EEPROM.write(i, mac[i]); }
    debugSerial<<F("Updated\n");
}

void cmdFunctionGet(int arg_cnt, char **args) {
    lanStatus= loadConfigFromHttp(arg_cnt, args);
    ethClient.stop(); //Refresh MQTT connect to get retained info
    //restoreState();
}

void printBool(bool arg) { (arg) ? debugSerial<<F("+") : debugSerial<<F("-"); }

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
    for (int i = 0; i < 4; i++)
        ip[i] = EEPROM.read(n++);
    return (ip[0] && (ip[0] != 0xff));
}
lan_status loadConfigFromHttp(int arg_cnt, char **args)
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
#ifndef DEVICE_NAME
    snprintf(URI, sizeof(URI), "/%02x-%02x-%02x-%02x-%02x-%02x.config.json", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
#else
#ifndef FLASH_64KB
    snprintf(URI, sizeof(URI), "/%s_config.json",QUOTE(DEVICE_NAME));
#else
    strncpy(URI, "/", sizeof(URI));
    strncat(URI, QUOTE(DEVICE_NAME), sizeof(URI));
    strncat(URI, "_config.json", sizeof(URI));
#endif
#endif
    debugSerial<<F("Config URI: http://")<<configServer<<URI<<endl;

#if defined(ARDUINO_ARCH_AVR)
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

            debugSerial<<F("got Config\n");
            char c;
            aJsonFileStream as = aJsonFileStream(configStream);
            noInterrupts();
            aJson.deleteItem(root);
            root = aJson.parse(&as);
            interrupts();
        //    debugSerial<<F("Parsed."));
            hclient.closeStream(configStream);  // this is very important -- be sure to close the STREAM

            if (!root) {
                debugSerial<<F("Config parsing failed\n");
                nextLanCheckTime = millis() + 15000;
                return READ_RE_CONFIG;//-11;
            } else {
            //    char *outstr = aJson.print(root);
            //    debugSerial<<outstr);
            //    free(outstr);
            debugSerial<<F("Applying.\n");
                applyConfig();
            debugSerial<<F("Done.\n");

            }

        } else {
            debugSerial<<F("ERROR: Server returned ");
            debugSerial<<responseStatusCode;
            nextLanCheckTime = millis() + 5000;
            return READ_RE_CONFIG;//-11;
        }

    } else {
        debugSerial<<F("failed to connect");
        debugSerial<<F(" try again in 5 seconds\n");
        nextLanCheckTime = millis() + 5000;
        return READ_RE_CONFIG;//-11;
    }
#endif
#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32) || defined(ARDUINO_ARCH_ESP32)
    #if defined(ARDUINO_ARCH_ESP32)
    WiFiClient configEthClient;
    #else
    EthernetClient configEthClient;
    #endif
      String response;
    HttpClient htclient = HttpClient(configEthClient, configServer, 80);
    //htclient.stop(); //_socket =MAX
    htclient.setHttpResponseTimeout(4000);
    wdt_res();
    //debugSerial<<"making GET request");get
    htclient.beginRequest();
    responseStatusCode = htclient.get(URI);
    htclient.endRequest();

    if (responseStatusCode == HTTP_SUCCESS)
    {
    // read the status code and body of the response
    responseStatusCode = htclient.responseStatusCode();
    response = htclient.responseBody();
    htclient.stop();
    wdt_res();
    debugSerial<<F("HTTP Status code: ");
    debugSerial<<responseStatusCode;
    //debugSerial<<"GET Response: ");

    if (responseStatusCode == 200) {
        aJson.deleteItem(root);
        root = aJson.parse((char *) response.c_str());

        if (!root) {
            debugSerial<<F("Config parsing failed\n");
            return READ_RE_CONFIG;//-11; //Load from NVRAM
        } else {
            debugSerial<<response;
            applyConfig();
            debugSerial<<F("Done.\n");
        }
      } else  {
          debugSerial<<F("Config retrieving failed\n");
          return READ_RE_CONFIG;//-11; //Load from NVRAM
      }
    } else {
        debugSerial<<F("Connect failed\n");
        return READ_RE_CONFIG;//-11; //Load from NVRAM
    }
#endif
#if defined(ARDUINO_ARCH_ESP8266)
    HTTPClient httpClient;
    String fullURI = "http://";
    fullURI+=configServer;
    fullURI+=URI;
    httpClient.begin(fullURI);
    int httpResponseCode = httpClient.GET();
    if (httpResponseCode > 0) {
        debugSerial.printf("[HTTP] GET... code: %d\n", httpResponseCode);
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = httpClient.getString();
            debugSerial<<response;
            aJson.deleteItem(root);
            root = aJson.parse((char *) response.c_str());
            if (!root) {
                debugSerial<<F("Config parsing failed\n");
                return READ_RE_CONFIG;
            } else {
                debugSerial<<F("Config OK, Applying\n");
                applyConfig();
                debugSerial<<F("Done.\n");
            }
        } else {
            debugSerial<<F("Config retrieving failed\n");
            return READ_RE_CONFIG;//-11; //Load from NVRAM
        }
    } else {
        debugSerial.printf("[HTTP] GET... failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
        httpClient.end();
        return READ_RE_CONFIG;
    }
    httpClient.end();
#endif

    return IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
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
  //Serial.println("Hello");
  //delay(1000);
    setupCmdArduino();
    printFirmwareVersionAndBuildOptions();

    scan_i2c_bus();

#ifdef SD_CARD_INSERTED
    sd_card_w5100_setup();
#endif
    setupMacAddress();

#if defined(ARDUINO_ARCH_ESP8266)
      EEPROM.begin(ESP_EEPROM_SIZE);
#endif
    loadConfigFromEEPROM();

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
    if (oneWire) oneWire->idle(&owIdle);
#endif

    mqttClient.setCallback(mqttCallback);

#ifdef _artnet
    ArtnetSetup();
#endif

#if (defined(ARDUINO_ARCH_ESP8266) or defined(ARDUINO_ARCH_ESP32)) and not defined(WIFI_MANAGER_DISABLE)
//    WiFiManager wifiManager;
    wifiManager.setTimeout(180);

#if defined(ESP_WIFI_AP) and defined(ESP_WIFI_PWD)
    wifiInitialized = wifiManager.autoConnect(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));
#else
    wifiInitialized = wifiManager.autoConnect();
#endif

#endif

    delay(LAN_INIT_DELAY);//for LAN-shield initializing
    //TODO: checkForRemoteSketchUpdate();
}

void printFirmwareVersionAndBuildOptions() {
    debugSerial<<F("\nLazyhome.ru LightHub controller ")<<F(QUOTE(PIO_SRC_REV))<<F(" C++ version:")<<F(QUOTE(__cplusplus))<<endl;
#ifdef CONTROLLINO
    debugSerial<<F("\n(+)CONTROLLINO");
#endif
#ifdef WATCH_DOG_TICKER_DISABLE
    debugSerial<<F("\n(-)WATCHDOG");
#else
    debugSerial<<F("\n(+)WATCHDOG");
#endif
    debugSerial<<F("\nConfig server:")<<F(CONFIG_SERVER)<<F("\nFirmware MAC Address ")<<F(QUOTE(CUSTOM_FIRMWARE_MAC));
#ifdef DISABLE_FREERAM_PRINT
    debugSerial<<F("\n(-)FreeRam printing");
#else
    debugSerial<<F("\n(+)FreeRam printing");
#endif

#ifdef USE_1W_PIN
    debugSerial<<F("\n(-)DS2482-100 USE_1W_PIN=")<<QUOTE(USE_1W_PIN);
#else
    debugSerial<<F("\n(+)DS2482-100");
#endif

#ifdef Wiz5500
    debugSerial<<F("\n(+)WizNet5500");
#endif

#ifdef DMX_DISABLE
    debugSerial<<F("\n(-)DMX");
#else
    debugSerial<<F("\n(+)DMX");
#endif

#ifdef MODBUS_DISABLE
    debugSerial<<F("\n(-)MODBUS");
#else
    debugSerial<<F("\n(+)MODBUS");
#endif

#ifdef OWIRE_DISABLE
    debugSerial<<F("\n(-)OWIRE");
#else
    debugSerial<<F("\n(+)OWIRE");
#endif
#ifndef DHT_DISABLE
    debugSerial<<F("\n(+)DHT");
#else
    debugSerial<<F("\n(-)DHT");
#endif

#ifndef COUNTER_DISABLE
    debugSerial<<F("\n(+)COUNTER");
#else
    debugSerial<<F("\n(-)COUNTER");
#endif

#ifdef SD_CARD_INSERTED
    debugSerial<<F("\n(+)SDCARD");
#endif

#ifdef RESET_PIN
    debugSerial<<F("\n(+)HARDRESET on pin=")<<QUOTE(RESET_PIN);
#else
    debugSerial<<F("\n(-)HARDRESET, using soft");
#endif

#ifdef RESTART_LAN_ON_MQTT_ERRORS
    debugSerial<<F("\n(+)RESTART_LAN_ON_MQTT_ERRORS");
#else
    debugSerial<<F("\n(-)RESTART_LAN_ON_MQTT_ERRORS");
#endif
debugSerial<<endl;


//    WDT_Disable( WDT ) ;
#if defined(__SAM3X8E__)

    Serial.println(F("Reading 128 bits unique identifier") ) ;
    ReadUniqueID( UniqueID ) ;

    Serial.print ("ID: ") ;
    for (byte b = 0 ; b < 4 ; b++)
      Serial.print ((unsigned int) UniqueID [b], HEX) ;
    Serial.println () ;
#endif

}



void publishStat(){
  long fr = freeRam();
  char topic[64];
  char intbuf[16];
  uint32_t ut = millis()/1000UL;
  if (!mqttClient.connected()) return;

//    debugSerial<<F("\nfree RAM: ")<<fr;
    setTopic(topic,sizeof(topic),T_DEV);
    strncat_P(topic, stats_P, sizeof(topic));
    strncat(topic, "/", sizeof(topic));
    strncat_P(topic, freeheap_P, sizeof(topic));
    printUlongValueToStr(intbuf, fr);
    mqttClient.publish(topic,intbuf,true);

    setTopic(topic,sizeof(topic),T_DEV);
    strncat_P(topic, stats_P, sizeof(topic));
    strncat(topic, "/", sizeof(topic));
    strncat_P(topic, uptime_P, sizeof(topic));
    printUlongValueToStr(intbuf, ut);
    mqttClient.publish(topic,intbuf,true);
}

void setupMacAddress() {
//Check MAC, stored in NVRAM
bool isMacValid = false;
for (short i = 0; i < 6; i++) {
        mac[i] = EEPROM.read(i);
        if (mac[i] != 0 && mac[i] != 0xff) isMacValid = true;
    }
if (!isMacValid) {
    debugSerial<<F("No MAC configured: set firmware's MAC\n");

    #if defined (CUSTOM_FIRMWARE_MAC) //Forced MAC from compiler's directive
    const char *macStr = QUOTE(CUSTOM_FIRMWARE_MAC);//colon(:) separated from build options
    parseBytes(macStr, ':', mac, 6, 16);

    #elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    //Using original MPU MAC
    WiFi.begin();
    WiFi.macAddress(mac);

    #elif defined(__SAM3X8E__)
    //Lets make MAC from MPU serial#
    mac[0]=0xDE;
    //firmwareMacAddress[1]=0xAD;
    for (byte b = 0 ; b < 5 ; b++)
              mac[b+1]=UniqueID [b] ;

    #elif defined DEFAULT_FIRMWARE_MAC
    uint8_t defaultMac[6] = DEFAULT_FIRMWARE_MAC;//comma(,) separated hex-array, hard-coded
    memcpy(mac,defaultMac,6);
    #endif
    }

    printMACAddress();
}

void setupCmdArduino() {
    cmdInit(uint32_t(SERIAL_BAUD));
    debugSerial<<(F(">>>"));
    cmdAdd("help", cmdFunctionHelp);
    cmdAdd("save", cmdFunctionSave);
    cmdAdd("load", cmdFunctionLoad);
    cmdAdd("get", cmdFunctionGet);
#ifndef FLASH_64KB
    cmdAdd("mac", cmdFunctionSetMac);
#endif
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
    if (lanLoop() > HAVE_IP_ADDRESS) {
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();  ///hung
#endif
    }

#ifdef _owire
    if (owReady && owArr) owLoop();
#endif

#ifdef _dmxin
    //    unsigned long lastpacket = DMXSerial.noDataSince();
    DMXCheck();
#endif

    if (items) {
        #ifndef MODBUS_DISABLE
        if (lanStatus != RETAINING_COLLECTING) pollingLoop();
        #endif
//#ifdef _owire
        thermoLoop();
//#endif
    }


    inputLoop();

#if defined (_espdmx)
    dmxout.update();
#endif

#ifdef SYSLOG_ENABLE
//        debugSerial<<F("#"));
//        udpSyslog.log(LOG_INFO, "Ping syslog:");
#endif

}

void owIdle(void) {
#ifdef _artnet
    if (artnet && (lanStatus>=HAVE_IP_ADDRESS)) artnet->read();
#endif

    wdt_res();
    return;

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
                in.poll(CHECK_INPUT);
            }
            input = input->next;
        }
        nextInputCheck = millis() + INTERVAL_CHECK_INPUT;
    }

    if (millis() > nextSensorCheck) {
        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.poll(CHECK_SENSOR);
            }
            input = input->next;
        }
        nextSensorCheck = millis() + INTERVAL_CHECK_SENSOR;
    }

}

void inputSetup(void) {
    if (!inputs) return;

        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.setup();
            }
            input = input->next;
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
    if (!items) return;
    bool thermostatCheckPrinted = false;
    for (aJsonObject *thermoItem = items->child; thermoItem; thermoItem = thermoItem->next) {
        if (isThermostatWithMinArraySize(thermoItem, 5)) {
            aJsonObject *thermoExtensionArray = aJson.getArrayItem(thermoItem, I_EXT);
            if (thermoExtensionArray && (aJson.getArraySize(thermoExtensionArray) > 1)) {
                int thermoPin = aJson.getArrayItem(thermoItem, I_ARG)->valueint;
                float thermoSetting = aJson.getArrayItem(thermoItem, I_VAL)->valueint; ///
                int thermoStateCommand = aJson.getArrayItem(thermoItem, I_CMD)->valueint;
                float curTemp = aJson.getArrayItem(thermoExtensionArray, IET_TEMP)->valuefloat;

                if (!aJson.getArrayItem(thermoExtensionArray, IET_ATTEMPTS)->valueint) {
                    debugSerial<<thermoItem->name<<F(" Expired\n");

                } else {
                    if (!(--aJson.getArrayItem(thermoExtensionArray, IET_ATTEMPTS)->valueint))
                        mqttClient.publish("/alarm/snsr", thermoItem->name);

                }
                if (curTemp > THERMO_OVERHEAT_CELSIUS) mqttClient.publish("/alarm/ovrht", thermoItem->name);


                debugSerial << endl << thermoItem->name << F(" Set:") << thermoSetting << F(" Cur:") << curTemp
                            << F(" cmd:") << thermoStateCommand;
                if (thermoPin<0) pinMode(-thermoPin, OUTPUT); else pinMode(thermoPin, OUTPUT);
                if (thermoDisabledOrDisconnected(thermoExtensionArray, thermoStateCommand)) {
                    if (thermoPin<0) digitalWrite(-thermoPin, LOW); digitalWrite(thermoPin, LOW);
                    // Caution - for water heaters (negative pin#) if some comes wrong (or no connection with termometers output is LOW - valve OPEN)
                    // OFF - also VALVE is OPEN (no teat control)
                    debugSerial<<F(" OFF");
                } else {
                    if (curTemp < thermoSetting - THERMO_GIST_CELSIUS) {
                        if (thermoPin<0) digitalWrite(-thermoPin, LOW); digitalWrite(thermoPin, HIGH);
                        debugSerial<<F(" ON");
                    } //too cold
                    else if (curTemp >= thermoSetting) {
                        if (thermoPin<0) digitalWrite(-thermoPin, HIGH); digitalWrite(thermoPin, LOW);
                        debugSerial<<F(" OFF");
                    } //Reached settings
                    else debugSerial<<F(" -target zone-"); // Nothing to do
                }
                thermostatCheckPrinted = true;
            }
        }
    }

    nextThermostatCheck = millis() + THERMOSTAT_CHECK_PERIOD;
publishStat();
#ifndef DISABLE_FREERAM_PRINT
    (thermostatCheckPrinted) ? debugSerial<<F("\nfree:")<<freeRam()<<" " : debugSerial<<F(" ")<<freeRam()<<F(" ");
#endif


}

short thermoSetCurTemp(char *name, float t) {
  if (!name || !strlen(name)) return 0;
    if (items) {
        aJsonObject *thermoItem = aJson.getObjectItem(items, name);
        if (!thermoItem) return 0;
        if (isThermostatWithMinArraySize(thermoItem, 4)) {
            aJsonObject *extArray = NULL;

            if (aJson.getArraySize(thermoItem) == 4) //No thermo extension yet
            {
                extArray = aJson.createArray(); //Create Ext Array

                aJsonObject *ocurt = aJson.createItem(t);  //Create float
                aJsonObject *oattempts = aJson.createItem(T_ATTEMPTS); //Create int
                aJson.addItemToArray(extArray, ocurt);
                aJson.addItemToArray(extArray, oattempts);
                aJson.addItemToArray(thermoItem, extArray); //Adding to thermoItem
            }
            else if (extArray = aJson.getArrayItem(thermoItem, I_EXT)) {
                aJsonObject *att = aJson.getArrayItem(extArray, IET_ATTEMPTS);
                aJson.getArrayItem(extArray, IET_TEMP)->valuefloat = t;
                if (att->valueint == 0) mqttClient.publish("/alarmoff/snsr", thermoItem->name);
                att->valueint = (int) T_ATTEMPTS;
            }
        }
    return 1;}
return 0;}
