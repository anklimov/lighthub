




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
Static IP
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
#include "options.h"

#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
EthernetClient ethClient;
#endif

#if defined(__AVR__)
EthernetClient ethClient;
#endif

const char outprefix[] PROGMEM = OUTTOPIC;
const char inprefix[] PROGMEM = INTOPIC;

aJsonObject *root = NULL;
aJsonObject *items = NULL;
aJsonObject *inputs = NULL;

aJsonObject *mqttArr = NULL;
aJsonObject *modbusArr = NULL;
aJsonObject *owArr = NULL;
aJsonObject *dmxArr = NULL;

unsigned long nextPollingCheck = 0;
unsigned long nextInputCheck = 0;
unsigned long lanCheck = 0;
unsigned long nextThermostatCheck = 0;

aJsonObject *pollingItem = NULL;

bool owReady = false;
int lanStatus = 0;

#ifdef _modbus
ModbusMaster node;
#endif

byte mac[6];


PubSubClient mqttClient(ethClient);


void watchdogSetup(void) {}    //Do not remove - strong re-definition WDT Init for DUE

// MQTT Callback routine
#define MQTT_SUBJECT_LENGTH 20
#define MQTT_TOPIC_LENGTH 20

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    payload[length] = 0;
    Serial.print(F("\n["));
    Serial.print(topic);
    Serial.print(F("] "));

    int fr = freeRam();
    if (fr < 250) {
        Serial.println(F("OOM!"));
        return;
    }

    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();

    boolean retaining = (lanStatus == 4);  //Todo - named constant
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
    int cmd = 0;

    cmd = txt2cmd((char *) payload);
    char *t;
    if (t = strrchr(topic, '/'))
        strncpy(subtopic, t + 1, MQTT_SUBJECT_LENGTH - 1);



    /* No 1-w direct support anymore
    int subchan;
    char buf[17];
    //Check for  one-wire address
    if (sscanf(subtopic,"S%1d%16s",&subchan,&buf)==2)   // SnXXXXXXXX
    {    DeviceAddress addr;
         SetAddr(buf,addr);;
         PrintBytes(addr,8);
         Serial.print(F(":"));
         Serial.println(subchan);
         cntrl2413(addr,subchan,(cmd==CMD_ON)?1:0);
    }// End OneWire

     else
     */
    {

        Item item(subtopic);
        if (item.isValid()) {
            if (item.itemType == CH_GROUP && retaining)
                return; //Do not restore group channels - they consist not relevant data
            switch (cmd) {
                case 0: {
                    short i = 0;
                    int Par[3];

                    while (payload && i < 3)
                        Par[i++] = getInt((char **) &payload);

                    item.Ctrl(0, i, Par, !retaining);
                }
                    break;

                case -1: //Not known command
                case -2: //JSON input (not implemented yet
                    break;
                case -3: //RGB color in #RRGGBB notation
                {
                    CRGB rgb;
                    if (sscanf((const char*)payload, "#%2X%2X%2X", &rgb.r, &rgb.g, &rgb.b) == 3) {
                        int Par[3];
                        CHSV hsv = rgb2hsv_approximate(rgb);
                        Par[0] = map(hsv.h, 0, 255, 0, 365);
                        Par[1] = map(hsv.s, 0, 255, 0, 100);
                        Par[2] = map(hsv.v, 0, 255, 0, 100);
                        item.Ctrl(0, 3, Par, !retaining);
                    }
                    break;
                }
                case CMD_ON:

                    //       if (item.getEnableCMD(500) || lanStatus == 4)
                    item.Ctrl(cmd, 0, NULL,
                              !retaining); //Accept ON command not earlier then 500 ms after set settings (Homekit hack)
                    //       else Serial.println(F("on Skipped"));

                    break;
                default: //some known command
                    item.Ctrl(cmd, 0, NULL, !retaining);

            } //ctrl
        } //valid json
    } //no1wire
}

#ifndef __ESP__

void printIPAddress() {
    Serial.print(F("My IP address: "));
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        Serial.print(Ethernet.localIP()[thisByte], DEC);
        Serial.print(F("."));
    }
    Serial.println();
}

#endif

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

int lanLoop() {

#ifdef NOETHER
    lanStatus=-14;
#endif

//Serial.println(lanStatus);
    switch (lanStatus) {
//Initial state
        case 0: //Ethernet.begin(mac,ip);

#ifdef __ESP__
            //WiFi.mode(WIFI_STA);
//wifiMulti.addAP("Smartbox", "");
if((wifiMulti.run() == WL_CONNECTED)) lanStatus=1;
#else
            Serial.println(F("Starting lan"));
            wdt_dis();
            if (Ethernet.begin(mac, 12000) == 0) {
                Serial.println(F("Failed to configure Ethernet using DHCP"));
                lanStatus = -10;
                lanCheck = millis() + 60000;
            } else {
                printIPAddress();
                lanStatus = 1;
            }
            wdt_en();
            wdt_res();
#endif
            break;
//Have IP address
        case 1:

            lanStatus = getConfig(0, NULL); //got config from server or load from NVRAM
#ifdef _artnet
            if (artnet) artnet->begin();
#endif

            break;
        case 2: // IP Ready, config loaded, Connecting broker  & subscribeArming Watchdog
            wdt_res();
            {
                short n = 0;
                int port = 1883;
                char empty = 0;
                char *user = &empty;
                char *password = &empty;

                if (!mqttClient.connected() && mqttArr && ((n = aJson.getArraySize(mqttArr)) > 1)) {
                    char *client_id = aJson.getArrayItem(mqttArr, 0)->valuestring;
                    char *servername = aJson.getArrayItem(mqttArr, 1)->valuestring;
                    if (n >= 3) port = aJson.getArrayItem(mqttArr, 2)->valueint;

                    if (n >= 4) user = aJson.getArrayItem(mqttArr, 3)->valuestring;
                    if (n >= 5) password = aJson.getArrayItem(mqttArr, 4)->valuestring;

                    mqttClient.setServer(servername, port);

                    Serial.print(F("Attempting MQTT connection to "));
                    Serial.print(servername);
                    Serial.print(F(":"));
                    Serial.print(port);
                    Serial.print(F(" user:"));
                    Serial.print(user);
                    Serial.print(F(" ..."));

                    if (mqttClient.connect(client_id, user, password)) {
                        Serial.print(F("connected as "));
                        Serial.println(client_id);


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
                        lanStatus = 4;
                        lanCheck = millis() + 5000;
                        Serial.println(F("Awaiting for retained topics"));
                    } else {
                        Serial.print(F("failed, rc="));
                        Serial.print(mqttClient.state());
                        Serial.println(F(" try again in 5 seconds"));
                        lanCheck = millis() + 5000;
                        lanStatus = -12;
                    }
                }
                break;
            }

        case 4: //retaining ... Collecting
            if (millis() > lanCheck) {
                char buf[MQTT_TOPIC_LENGTH];

                //Unsubscribe from status topics..
                strncpy_P(buf, outprefix, sizeof(buf));
                strncat(buf, "#", sizeof(buf));
                mqttClient.unsubscribe(buf);

                lanStatus = 3;
                Serial.println(F("Accepting commands..."));
                break;
            }

        case 3: //operation
            if (!mqttClient.connected()) lanStatus = 2;
            break;

//Awaiting address
        case -10:
            if (millis() > lanCheck)
                lanStatus = 0;
            break;
//Reconnect
        case -12:
            if (millis() > lanCheck)

                lanStatus = 2;
            break;
            // read or Re-read config
        case -11:
            if (loadConfigFromEEPROM(0, NULL)) lanStatus = 2;
            else {
                lanCheck = millis() + 5000;
                lanStatus = -10;
            }
            break;

        case -14:;
            // do notghing with net
    }


    {
#ifndef __ESP__
        wdt_dis();
        if (lanStatus > 0)
            switch (Ethernet.maintain()) {
                case DHCP_CHECK_RENEW_FAIL:
                    //renewed fail
                    Serial.println(F("Error: renewed fail"));
                    lanStatus = -10;
                    break;

                case DHCP_CHECK_RENEW_OK:
                    Serial.println(F("Renewed success"));
                    printIPAddress();
                    break;

                case DHCP_CHECK_REBIND_FAIL:
                    Serial.println(F("Error: rebind fail"));
                    lanStatus = -10;
                    break;

                case DHCP_CHECK_REBIND_OK:
                    Serial.println(F("Rebind success"));
                    printIPAddress();
                    break;

                default:
                    //nothing happened
                    break;

            }

        wdt_en();
#endif
    }

    return lanStatus;

}

#ifdef _owire

void Changed(int i, DeviceAddress addr, int val) {
    char addrstr[32] = "NIL";
    char addrbuf[17];
    char valstr[16] = "NIL";
    char *owEmit = NULL;
    char *owItem = NULL;

    //PrintBytes(addr,8);
    // Serial.print("Emit: ");
    SetBytes(addr, 8, addrbuf);
    addrbuf[17] = 0;

    //Serial.println(addrbuf);

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
    } else Serial.println(F("Not find"));


    /* No sw support anymore
   switch (addr[0]){
    case 0x29: // DS2408
      snprintf(addrstr,sizeof(addrstr),"%sS0%s",outprefix,addrbuf);
     // Serial.println(addrstr);
      client.publish(addrstr, (val & SW_STAT0)?"ON":"OFF");
      snprintf(addrstr,sizeof(addrstr),"%sS1%s",outprefix,addrbuf);
    //  Serial.println(addrstr);
      client.publish(addrstr, (val & SW_STAT1)?"ON":"OFF");
      snprintf(addrstr,sizeof(addrstr),"%sS2%s",outprefix,addrbuf);
     // Serial.println(addrstr);
      client.publish(addrstr, (val & SW_AUX0)?"OFF":"ON");
      snprintf(addrstr,sizeof(addrstr),"%sS3%s",outprefix,addrbuf);
     // Serial.println(addrstr);
      client.publish(addrstr, (val & SW_AUX1)?"OFF":"ON");
      break;

    case 0x28: // Thermomerer

     snprintf(addrstr,sizeof(addrstr),"%s%s",outprefix,addrbuf);
     sprintf(valstr,"%d",val);
     //Serial.println(val);
     //Serial.println(valstr);
     client.publish(addrstr, valstr);

     if (owItem)
        {
        thermoSetCurTemp(owItem,val);
        }
     break;

    case 0x01:
    case 0x81:
     snprintf(addrstr,sizeof(addrstr),"%sDS%s",outprefix,addrbuf);
     if (val) sprintf(valstr,"%s","ON"); else sprintf(valstr,"%s","OFF");
     client.publish(addrstr, valstr);
   }
    */

    if ((val == -127) || (val == 85) || (val == 0)) { //ToDo: 1-w short circuit mapped to "0" celsium
//        Serial.print("Temp err ");Serial.println(t);
        return;
    }

    strcpy_P(addrstr, outprefix);
    strncat(addrstr, addrbuf, sizeof(addrstr));
    //snprintf(addrstr,sizeof(addrstr),"%s%s",F(outprefix),addrbuf);
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
                             "'set de:ad:be:ef:fe:00' set and store MAC-address in EEPROM\n"
                             "'save' - save config in NVRAM\n"
                             "'get' - get config from pre-configured URL\n"
                             "'load' - load config from NVRAM\n"
                             "'kill' - test watchdog"));
}

void cmdFunctionKill(int arg_cnt, char **args) {
    for (short i = 17; i > 0; i--) {
        delay(1000);
        Serial.println(i);
    };
}

void applyConfig() {
#ifdef _dmxout
    int maxChannels;
    aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
    if (dmxoutArr && aJson.getArraySize(dmxoutArr) == 2) {
        DMXoutSetup(maxChannels = aJson.getArrayItem(dmxoutArr, 1)->valueint,
                    aJson.getArrayItem(dmxoutArr, 0)->valueint);
        Serial.print(F("DMX out started. Channels: "));
        Serial.println(maxChannels);
    }
#endif
#ifdef _dmxin
    int itemsCount;
    dmxArr = aJson.getObjectItem(root, "dmxin");
    if (dmxArr && (itemsCount = aJson.getArraySize(dmxArr))) {
        DMXinSetup(itemsCount * 4);
        Serial.print(F("DMX in started. Channels:"));
        Serial.println(itemsCount * 4);
    }
#endif
#ifdef _modbus
    modbusArr = aJson.getObjectItem(root, "modbus");
#endif

#ifdef _owire
    owArr = aJson.getObjectItem(root, "ow");
#endif

#ifdef _owire
    if (owArr && !owReady) {
        aJsonObject *item = owArr->child;
        owReady = owSetup(&Changed);
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
{
aJsonObject * item = items->child;
while (items && item)
  if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
    int cmd = CMD_OFF;
    int pin = aJson.getArrayItem(item, I_ARG)->valueint;
    if (aJson.getArraySize(item) > I_CMD) cmd = aJson.getArrayItem(item, I_CMD)->valueint;

      switch (aJson.getArrayItem(item, I_TYPE)->valueint) {
          case CH_RELAY:
          case CH_THERMO:
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
          item = item->next;
     }  //if

}
    pollingItem = items->child;
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
    getConfig(arg_cnt, args);
    restoreState();
}

void printBool(bool arg) { (arg) ? Serial.println(F("on")) : Serial.println(F("off")); }


void saveFlash(short n, char *str) {}

void loadFlash(short n, char *str) {}

#ifndef MY_CONFIG_SERVER
#define CONFIG_SERVER "lazyhome.ru"
#else
#define CONFIG_SERVER QUOTE(MY_CONFIG_SERVER)
#endif

int getConfig(int arg_cnt, char **args)
//(char *tokens)
{


    int responseStatusCode = 0;
    char ch;
    char URI[40];
    char configServer[32] = CONFIG_SERVER;
    if (arg_cnt > 0) {
        strncpy(configServer, args[1], sizeof(configServer) - 1);
        saveFlash(0, configServer);
    } else loadFlash(0, configServer);

    snprintf(URI, sizeof(URI), "/%02x-%02x-%02x-%02x-%02x-%02x.config.json", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
    Serial.print(F("Config URI: http://"));
    Serial.print(configServer);
    Serial.println(URI);

#if defined(__AVR__)
    FILE *result;
    //byte hserver[] = { 192,168,88,2 };
    wdt_dis();

    HTTPClient hclient(configServer, 80);
    // FILE is the return STREAM type of the HTTPClient
    result = hclient.getURI(URI);
    responseStatusCode = hclient.getLastReturnCode();
    wdt_en();

    if (result != NULL) {
        if (responseStatusCode == 200) {

            Serial.println(F("got Config"));
            aJsonFileStream as = aJsonFileStream(result);
            aJson.deleteItem(root);
            root = aJson.parse(&as);
            hclient.closeStream(result);  // this is very important -- be sure to close the STREAM

            if (!root) {
                Serial.println(F("Config parsing failed"));
                lanCheck = millis() + 15000;
                return -11;
            } else {
            //    char *outstr = aJson.print(root);
            //    Serial.println(outstr);
            //    free(outstr);

                applyConfig();


            }

        } else {
            Serial.print(F("ERROR: Server returned "));
            Serial.println(responseStatusCode);
            lanCheck = millis() + 5000;
            return -11;
        }

    } else {
        Serial.println(F("failed to connect"));
        Serial.println(F(" try again in 5 seconds"));
        lanCheck = millis() + 5000;
        return -11;
    }

#else
    //Non AVR code
    String response;

    HttpClient htclient = HttpClient(ethClient, configServer, 80);
    htclient.setHttpResponseTimeout(4000);
    wdt_res();
    //Serial.println("making GET request");
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
            // lanCheck=millis()+15000;
            return -11; //Load from NVRAM
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
        //lanCheck=millis()+15000;
        return -11; //Load from NVRAM
    }


#endif
    return 2;
}

void preTransmission() {
    digitalWrite(TXEnablePin, 1);
}

void postTransmission() {
    //modbusSerial.flush();
    digitalWrite(TXEnablePin, 0);
}
//#define PIO_SRC_REV commit 8034a6b765229d94a94d90fd08dd9588acf5f3da Author: livello <livello@bk.ru> Date:   Wed Mar 28 02:35:50 2018 +0300 refactoring

void setup_main() {
    setupCmdArduino();
    printFirmwareVersionAndBuildOptions();

#ifdef SD_CARD_INSERTED
    sd_card_w5100_setup();
#endif

#ifdef __ESP__
    espSetup();
#endif

    setupMacAddress();

    loadConfigFromEEPROM(0, NULL);

#ifdef _modbus
    pinMode(TXEnablePin, OUTPUT);
    modbusSerial.begin(MODBUS_SERIAL_BAUD);
    node.idle(&modbusIdle);
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
#endif

    delay(20);
    owReady = 0;

#ifdef _owire
    if (net) net->idle(&owIdle);
#endif

    mqttClient.setCallback(mqttCallback);

#ifdef _artnet
    ArtnetSetup();
#endif

    //TODO: checkForRemoteSketchUpdate();
}

void printFirmwareVersionAndBuildOptions() {
    Serial.print(F("\nLazyhome.ru LightHub controller "));
    Serial.println(F(QUOTE(PIO_SRC_REV)));
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

#ifdef SD_CARD_INSERTED
    Serial.println(F("(+)SDCARD"));
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
    cmdAdd("help", cmdFunctionHelp);
    cmdAdd("save", cmdFunctionSave);
    cmdAdd("load", cmdFunctionLoad);
    cmdAdd("get", cmdFunctionGet);
    cmdAdd("set", cmdFunctionSetMac);
    cmdAdd("kill", cmdFunctionKill);
    cmdAdd("req", cmdFunctionReq);
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
        if (lanStatus != 4) pollingLoop();
#ifdef _owire
        thermoLoop();
#endif
    }


    if (inputs) inputLoop();

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

void modbusIdle(void) {
    wdt_res();
    if (lanLoop() > 1) {
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();
#endif
    }

#ifdef _dmxin
    DMXCheck();
#endif

#if defined (_espdmx)
    dmxout.update();
#endif
}

void inputLoop(void) {
    if (millis() > nextInputCheck) {
        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.Poll();
            }
            input = input->next;
        }
        nextInputCheck = millis() + INTERVAL_CHECK_INPUT;
    }
}

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


//TODO: refactoring

void thermoLoop(void) {
    if (millis() < nextThermostatCheck)
        return;

    bool thermostatCheckPrinted = false;
    aJsonObject *item = items->child;

    while (item) {
        if ((item->type == aJson_Array) && (aJson.getArrayItem(item, 0)->valueint == CH_THERMO) &&
            (aJson.getArraySize(item) > 4)) {
            int itemPin = aJson.getArrayItem(item, I_ARG)->valueint;
            int itemTempSetting = aJson.getArrayItem(item, I_VAL)->valueint;
            int itemCommand = aJson.getArrayItem(item, I_CMD)->valueint;
            aJsonObject *itemExtensionArray = aJson.getArrayItem(item, I_EXT);

            if (itemExtensionArray && (aJson.getArraySize(itemExtensionArray) > 1)) {
                int curtemp = aJson.getArrayItem(itemExtensionArray, IET_TEMP)->valueint;
                if (!aJson.getArrayItem(itemExtensionArray, IET_ATTEMPTS)->valueint) {
                    Serial.print(item->name);
                    Serial.println(F(" Expired"));

                } else {
                    if (!(--aJson.getArrayItem(itemExtensionArray, IET_ATTEMPTS)->valueint))
                        mqttClient.publish("/alarm/snsr", item->name);

                }
                if (curtemp > THERMO_OVERHEAT_CELSIUS) mqttClient.publish("/alarm/ovrht", item->name);

                thermostatCheckPrinted = true;
                Serial.print(item->name);
                Serial.print(F(" Set:"));
                Serial.print(itemTempSetting);
                Serial.print(F(" Curtemp:"));
                Serial.print(curtemp);
                Serial.print(F(" cmd:"));
                Serial.print(itemCommand), pinMode(itemPin, OUTPUT);
                if (itemCommand == CMD_OFF || itemCommand == CMD_HALT ||
                    aJson.getArrayItem(itemExtensionArray, IET_ATTEMPTS)->valueint == 0) {
                    digitalWrite(itemPin, LOW);
                    Serial.println(F(" OFF"));
                } else {
                    if (curtemp + THERMO_GIST_CELSIUS < itemTempSetting) {
                        digitalWrite(itemPin, HIGH);
                        Serial.println(F(" ON"));
                    } //too cold
                    else if (itemTempSetting <= curtemp) {
                        digitalWrite(itemPin, LOW);
                        Serial.println(F(" OFF"));
                    } //Reached settings
                    else Serial.println(F(" --")); // Nothing to do
                }
            }
        }
        item = item->next;
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
        aJsonObject *item = aJson.getObjectItem(items, name);
        if (item && (item->type == aJson_Array) && (aJson.getArrayItem(item, I_TYPE)->valueint == CH_THERMO) &&
            (aJson.getArraySize(item) >= 4)) {
            aJsonObject *extArray = NULL;

            if (aJson.getArraySize(item) == 4) //No thermo extension yet
            {
                extArray = aJson.createArray(); //Create Ext Array

                aJsonObject *ocurt = aJson.createItem(t);  //Create int
                aJsonObject *oattempts = aJson.createItem(T_ATTEMPTS); //Create int
                aJson.addItemToArray(extArray, ocurt);
                aJson.addItemToArray(extArray, oattempts);
                aJson.addItemToArray(item, extArray); //Adding to item
            } //if
            else if (extArray = aJson.getArrayItem(item, I_EXT)) {
                aJsonObject *att = aJson.getArrayItem(extArray, IET_ATTEMPTS);
                aJson.getArrayItem(extArray, IET_TEMP)->valueint = t;
                if (att->valueint == 0) mqttClient.publish("/alarmoff/snsr", item->name);
                att->valueint = (int) T_ATTEMPTS;
            } //if


        } //if
    } // if items

} //proc
