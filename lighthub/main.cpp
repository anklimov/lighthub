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
*/

#include "main.h"
#include "statusled.h"

#include "flashstream.h"
#include "config.h"

#if defined(__SAM3X8E__)
#include "TimerInterrupt_Generic.h"
#endif

#if defined(FS_STORAGE)
flashStream sysConfStream("config.bin"); 
flashStream JSONStream("config.json"); 
#else
flashStream sysConfStream(SYSCONF_OFFSET,EEPROM_offsetJSON); 
flashStream JSONStream(EEPROM_offsetJSON,MAX_JSON_CONF_SIZE); 
#endif

systemConfig sysConf(&sysConfStream);


extern long  timer0_overflow_count;	 
#ifdef WIFI_ENABLE
WiFiClient ethClient;

    #if not defined(WIFI_MANAGER_DISABLE)
        WiFiManager wifiManager;
    #endif

#else
#include <Dhcp.h>
EthernetClient ethClient;
#endif

#if defined(OTA)
#include <ArduinoOTA.h>
#endif

#ifdef MDNS_ENABLE
    #ifndef WIFI_ENABLE
    EthernetUDP mdnsUDP;
    #else
    WiFiUDP mdnsUDP;
    #endif
MDNS mdns(mdnsUDP);
#endif

/*
#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
#endif


#ifdef ARDUINO_ARCH_ESP32
NRFFlashStorage EEPROM;
#endif

#ifdef ARDUINO_ARCH_STM32
NRFFlashStorage EEPROM;
#endif

#ifdef NRF5
NRFFlashStorage EEPROM;
#endif
*/

#ifdef SYSLOG_ENABLE
#include <Syslog.h>

    #ifndef WIFI_ENABLE
    EthernetUDP udpSyslogClient;
    #else
    WiFiUDP udpSyslogClient;
    #endif
Syslog udpSyslog(udpSyslogClient, SYSLOG_PROTO_BSD);
//unsigned long timerSyslogPingTime;
static char syslogDeviceHostname[16];

Streamlog debugSerial(&debugSerialPort,LOG_DEBUG,&udpSyslog);
Streamlog errorSerial(&debugSerialPort,LOG_ERROR,&udpSyslog,ledRED);
Streamlog infoSerial (&debugSerialPort,LOG_INFO,&udpSyslog);
#else
Streamlog debugSerial(&debugSerialPort,LOG_DEBUG);
Streamlog errorSerial(&debugSerialPort,LOG_ERROR, ledRED);
Streamlog infoSerial (&debugSerialPort,LOG_INFO);
#endif

StatusLED statusLED(ledRED);


lan_status lanStatus = INITIAL_STATE;


const char configserver[] PROGMEM = CONFIG_SERVER;
const char verval_P[] PROGMEM = QUOTE(PIO_SRC_REV);

#if defined(__SAM3X8E__)
UID UniqueID;
#endif

char *deviceName = NULL;
aJsonObject *topics = NULL;
aJsonObject *root = NULL;
aJsonObject *items = NULL;
aJsonObject *inputs = NULL;

aJsonObject *mqttArr = NULL;
#ifdef _modbus
aJsonObject *modbusObj = NULL;
#endif
#ifdef _owire
aJsonObject *owArr = NULL;
#endif
#ifdef _dmxout
aJsonObject *dmxArr = NULL;
#endif

#ifdef SYSLOG_ENABLE
bool syslogInitialized = false;
#endif

uint32_t timerPollingCheck = 0;
uint32_t timerInputCheck = 0;
uint32_t timerLanCheckTime = 0;
uint32_t timerThermostatCheck = 0;
uint32_t timerSensorCheck =0;
uint32_t WiFiAwaitingTime =0;

aJsonObject *pollingItem = NULL;

bool owReady = false;
bool configOk = false; // At least once connected to MQTT
bool configLoaded = false;
bool initializedListeners = false;
int8_t ethernetIdleCount =0;
int8_t configLocked = 0;

#if defined (_modbus)
ModbusMaster node;
#endif

//byte mac[6];

PubSubClient mqttClient(ethClient);

bool wifiInitialized;

int mqttErrorRate;

#if defined(__SAM3X8E__)
void watchdogSetup(void) {}    //Do not remove - strong re-definition WDT Init for DUE
#endif

void cleanConf()
{
  if (!root) return;
debugSerial<<F("Unlocking config ...")<<endl;
uint32_t stamp=millis();
while (configLocked && !isTimeOver(stamp,millis(),10000))
{
          //wdt_res();
          cmdPoll();
          #ifdef _owire
          if (owReady && owArr) owLoop();
          #endif
          #ifdef _dmxin
          DMXCheck();
          #endif
          if (isNotRetainingStatus())  pollingLoop();
          thermoLoop();
          inputLoop(CHECK_INPUT);
          yield();
}

if (configLocked)
{
    errorSerial<<F("Not unlocked in 10s - continue ...")<<endl;
}

debugSerial<<F("Stopping channels ...")<<endl;
//Stoping the channels
aJsonObject * item = items->child;
while (items && item)
   {
    if (item->type == aJson_Array && aJson.getArraySize(item)>0)
    {
        Item it(item->name);
        if (it.isValid()) it.Stop();
        yield();

    }
   item = item->next;
  }
pollingItem = NULL;
debugSerial<<F("Stopped")<<endl;

#ifdef SYSLOG_ENABLE
syslogInitialized=false; //Garbage in memory
#endif
configLoaded=false;
debugSerial<<F("Deleting conf. RAM was:")<<freeRam();
    aJson.deleteItem(root);
    root   = NULL;
    inputs = NULL;
    items  = NULL;
    topics = NULL;
    mqttArr = NULL;
  #ifdef _dmxout
  dmxArr = NULL;
  #endif
  #ifdef _owire
  owArr = NULL;
  #endif
  #ifdef _modbus
  modbusObj = NULL;
  #endif
     debugSerial<<F(" is ")<<freeRam()<<endl;
     
     configOk=false;
}

bool isNotRetainingStatus() {
  return (lanStatus != RETAINING_COLLECTING);
}

int httpHandler(Client& client, String request, long contentLength, bool authorized)
{
    String response = "";

    debugSerial<<request<<endl;


    if (request == (F("GET /"))) 
        {
         client.println(F("HTTP/1.1 301 Moved permanently"));
         client.print(F("Location: http://lazyhome.ru/pwa?mac="));
         for (int i=0; i<6; i++) {client.print(sysConf.mac[i]>>4,HEX);client.print(sysConf.mac[i]&0xf,HEX);}
         client.print(F("&ip="));
         client.println(Ethernet.localIP());
         delay(100);
         return 1;
        }
    if (!authorized) return 401;

    if (request.startsWith(F("POST /item/")))
       {
        request.remove(0,11);      
        String body=client.readStringUntil('\n');
        Item   item((char*)request.c_str());
          if (!item.isValid() || !item.Ctrl((char*) body.c_str())) return 400;

        itemCmd ic;
        ic.loadItem(&item,true);
        char buf[32];
        response=ic.toString(buf, sizeof(buf));             
       }
    else if (request.startsWith(F("GET /item/")))
       {
        request.remove(0,10);
        Item   item((char*)request.c_str());
          if (!item.isValid()) return 400; 

         itemCmd ic;
         ic.loadItem(&item,true);
         char buf[32];
         response=ic.toString(buf, sizeof(buf));
        
       }  
    else if (request.startsWith(F("POST /command/"))) 
        {
        request.remove(0,14);      
        String body=client.readStringUntil('\n');  
        
        request+=" ";
        request+=body;

        debugSerial<<F("Cmd: ")<<request<<endl;
        if (request=="reboot") client.stop();
        const char* res=request.c_str();
        cmd_parse((char*) res);        
        } 
    else return -1;  //Unknown

        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Connection: close"));
        
          #ifdef CORS
          client.print(F("Access-Control-Allow-Origin: "));
          client.println(CORS);
          #endif
          
        if (response!="") {
              client.println("Content-Type: text/plain");
              client.println();
              client.println(response);
              }
  
        delay(100);
return 1;
}

int inTopic (char * topic,  topicType tt)
{
char buf[MQTT_TOPIC_LENGTH + 1];
int pfxlen;
int intopic;
  setTopic(buf,sizeof(buf)-1,tt);
  pfxlen = strlen(buf);
  intopic = strncmp(topic, buf, pfxlen);
  // debugSerial<<buf<<" "<<pfxlen<<" "<<intopic<<endl;
  if (!intopic) return pfxlen;
  return 0; 
}    

void mqttCallback(char *topic, byte *payload, unsigned int length) 
{
    if (!payload) return;
    payload[length] = 0;

    int fr = freeRam();

    debugSerial<<F("\n")<<fr<<F(":[")<<topic<<F("] ");

    if (fr < 250) {
        errorSerial<<F("OutOfMemory!")<<endl;
        return;// -2;
    }
    
    statusLED.flash(ledBLUE);
    debugSerial<<(char*)payload<<endl;
    short pfxlen  = 0;
    char * itemName = NULL;
    char * subItem = NULL;

// in Retaining status - trying to restore previous state from retained output topic. Retained input topics are not relevant.
if  (lanStatus == RETAINING_COLLECTING) pfxlen=inTopic(topic,T_OUT);
else
{
        pfxlen=inTopic(topic,T_BCST);
        if (!pfxlen) pfxlen = inTopic(topic,T_DEV);      
}   
    if (!pfxlen) {
        debugSerial<<F("Skipping..")<<endl;
        return;// -3;
    }

    itemName=topic+pfxlen;
    // debugSerial<<itemName<<endl;
    if(!strcmp(itemName,CMDTOPIC) && payload && (strlen((char*) payload)>1)) {
      //  mqttClient.publish(topic, "");
        cmd_parse((char *)payload);
        return;// -4;
    }

    if (subItem = strchr(itemName, '/'))
      {
        *subItem = 0;
        subItem++;
        if (*subItem=='$') return;// -5; //Skipping homie stuff
      }

    if (itemName[0]=='$') return;// -6; //Skipping homie stuff

    Item item(itemName);
    if (item.isValid()) {
        //return 
        item.Ctrl((char *)payload,subItem);        
    } //valid item
 return;// -7;   
}



void printMACAddress() {
    //macAddress * mac = sysConf.getMAC();
    infoSerial<<F("MAC:");
    for (byte i = 0; i < 6; i++)
{
  if (sysConf.mac[i]<16) infoSerial<<"0";
        (i < 5) ?infoSerial<<_HEX(sysConf.mac[i])<<F(":"):infoSerial<<_HEX(sysConf.mac[i])<<endl;
}
}

char* getStringFromConfig(aJsonObject * a, int i)
{
aJsonObject * element = NULL;
if (!a) return NULL;
if (a->type == aJson_Array)
  element = aJson.getArrayItem(a, i);
// TODO - human readable JSON objects as alias

  if (element && element->type == aJson_String) return element->valuestring;
  return NULL;
}

char* getStringFromConfig(aJsonObject * a, char * name)
{
aJsonObject * element = NULL;
if (!a) return NULL;
if (a->type == aJson_Object)
  element = aJson.getObjectItem(a, name);
if (element && element->type == aJson_String) return element->valuestring;
  return NULL;
}

#ifdef OTA
    void setupOTA(void)
    {
            ArduinoOTA.begin(Ethernet.localIP(), "Lighthub", "password", InternalStorage, sysConfStream, JSONStream);
            ArduinoOTA.setCustomHandler(httpHandler);
            infoSerial<<F("OTA initialized\n");

    }
#else 
void setupOTA(void) {};
#endif

void setupSyslog()
{
  #ifdef SYSLOG_ENABLE
   int syslogPort = 514;
   short n = 0;
   aJsonObject *udpSyslogArr = NULL;

   if (syslogInitialized) return;
   if (lanStatus<HAVE_IP_ADDRESS) return;
   if (!root) return;

    udpSyslogClient.begin(SYSLOG_LOCAL_SOCKET);

        udpSyslogArr = aJson.getObjectItem(root, "syslog");
      if (udpSyslogArr && (n = aJson.getArraySize(udpSyslogArr))) {
        char *syslogServer = getStringFromConfig(udpSyslogArr, 0);
        if (n>1) syslogPort = aJson.getArrayItem(udpSyslogArr, 1)->valueint;

        _inet_ntoa_r(Ethernet.localIP(),syslogDeviceHostname,sizeof(syslogDeviceHostname));
        infoSerial<<F("Syslog params:")<<syslogServer<<":"<<syslogPort<<":"<<syslogDeviceHostname<<endl;
        udpSyslog.server(syslogServer, syslogPort);
        udpSyslog.deviceHostname(syslogDeviceHostname);

        if (mqttArr) deviceName = getStringFromConfig(mqttArr, 0);
        if (deviceName) udpSyslog.appName(deviceName);
        udpSyslog.defaultPriority(LOG_KERN);
        syslogInitialized=true;

        infoSerial<<F("UDP Syslog initialized.\n");
      }
  #endif
}


lan_status lanLoop() {

    #ifdef NOETHER
    lanStatus=DO_NOTHING;//-14;
    #endif
//Serial.println(lanStatus);
    switch (lanStatus) {

        case INITIAL_STATE:
          //  statusLED.set(ledRED|((configLoaded)?ledBLINK:0));
              statusLED.set(ledRED|((configLoaded)?ledBLINK:0));

            #if  defined(WIFI_ENABLE)
                onInitialStateInitLAN(); // Moves state to AWAITING_ADDRESS or HAVE_IP_ADDRESS
            #else
                if (Ethernet.linkStatus() != LinkOFF)   onInitialStateInitLAN(); // Moves state to AWAITING_ADDRESS or HAVE_IP_ADDRESS
            #endif

            break;

        case AWAITING_ADDRESS:

        #if defined(WIFI_ENABLE)
            if (WiFi.status() == WL_CONNECTED)
            {
               infoSerial<<F("WiFi connected. IP address: ")<<WiFi.localIP()<<endl;
               wifiInitialized = true;
               lanStatus = HAVE_IP_ADDRESS;
            }
            else
     //       if (millis()>WiFiAwaitingTime)
              if (isTimeOver(WiFiAwaitingTime,millis(),WIFI_TIMEOUT))
            {
                errorSerial<<F("\nProblem with WiFi!");
                return lanStatus = DO_REINIT;
            }
        #else
            lanStatus = HAVE_IP_ADDRESS;
        #endif
        break;

        case HAVE_IP_ADDRESS:
        if (!initializedListeners)
        {
                setupSyslog();
                setupOTA();
                #ifdef _artnet
                            if (artnet) artnet->begin();
                #endif
                initializedListeners = true;
        }
        lanStatus = LIBS_INITIALIZED;
        break;

        case LIBS_INITIALIZED:
            statusLED.set(ledRED|ledGREEN|((configLoaded)?ledBLINK:0));
            if (configLocked) return LIBS_INITIALIZED;      

            if (!configOk)
                lanStatus = loadConfigFromHttp(0, NULL);
            else lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;

            break;

        case IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER:
            wdt_res();
            statusLED.set(ledRED|ledGREEN|((configLoaded)?ledBLINK:0));
            ip_ready_config_loaded_connecting_to_broker();
            break;

        case RETAINING_COLLECTING:
            //if (millis() > timerLanCheckTime) 
            if (isTimeOver(timerLanCheckTime,millis(),TIMEOUT_RETAIN))
            {
                char buf[MQTT_TOPIC_LENGTH+1];

                //Unsubscribe from status topics..
                //strncpy_P(buf, outprefix, sizeof(buf));
                setTopic(buf,sizeof(buf),T_OUT);
                strncat(buf, "+/+/#", sizeof(buf)); // Subscribing only on separated command/parameters topics
                mqttClient.unsubscribe(buf);

                lanStatus = OPERATION;//3;
                infoSerial<<F("Accepting commands...\n");
            }
            break;
        case OPERATION:
            statusLED.set(ledGREEN|((configLoaded)?ledBLINK:0));
            if (!mqttClient.connected()) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            break;

        case DO_REINIT: // Pause and re-init LAN
             //if (mqttClient.connected()) mqttClient.disconnect();  // Hmm hungs then cable disconnected
             timerLanCheckTime = millis();// + 5000;
             lanStatus = REINIT;
             statusLED.set(ledRED|((configLoaded)?ledBLINK:0));
             break;

        case REINIT: // Pause and re-init LAN

            //if (millis() > timerLanCheckTime)
            if (isTimeOver(timerLanCheckTime,millis(),TIMEOUT_REINIT))
                {
                lanStatus = INITIAL_STATE;
                }
            break;

        case DO_RECONNECT: // Pause and re-connect MQTT
            if (mqttClient.connected()) mqttClient.disconnect();
            timerLanCheckTime = millis();// + 5000;
            lanStatus = RECONNECT;
            break;

        case RECONNECT:
            //if (millis() > timerLanCheckTime)
            if (isTimeOver(timerLanCheckTime,millis(),TIMEOUT_RECONNECT))

                lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            break;

        case READ_RE_CONFIG: // Restore config from FLASH, re-init LAN
            if (loadConfigFromEEPROM()) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
            else {
                //timerLanCheckTime = millis();// + 5000;
                lanStatus = DO_REINIT;//-10;
            }
            break;

        case DO_NOTHING:;
    }


    {

#if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__) || defined (NRF5)
        wdt_dis();
        if (lanStatus >= HAVE_IP_ADDRESS)
        {
        int etherStatus = Ethernet.maintain();

        #ifndef Wiz5500
        #define NO_LINK 5
        if (Ethernet.linkStatus() == LinkOFF) etherStatus = NO_LINK;
        #endif

            switch (etherStatus) {
                   case NO_LINK:
                    errorSerial<<F("\nNo link")<<endl;
                    lanStatus = DO_REINIT;
                    break;
                case DHCP_CHECK_RENEW_FAIL:
                    errorSerial<<F("Error: renewed fail")<<endl;
                    lanStatus = DO_REINIT;
                    break;

                case DHCP_CHECK_RENEW_OK:
                    infoSerial<<F("Renewed success. IP address:");
                    printIPAddress(Ethernet.localIP());
                    infoSerial<<endl;
                    break;

                case DHCP_CHECK_REBIND_FAIL:
                    errorSerial<<F("Error: rebind fail")<<endl;
                    if (mqttClient.connected()) mqttClient.disconnect();
                    //timerLanCheckTime = millis();// + 1000;
                    lanStatus = DO_REINIT;
                    break;

                case DHCP_CHECK_REBIND_OK:
                    infoSerial<<F("Rebind success. IP address:");
                    printIPAddress(Ethernet.localIP());
                    infoSerial<<endl;
                    break;

                default:
                    break;

            }
            
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
  configLocked++;
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
              yield();
              item = item->next;
          }  //if
          //strncpy_P(topic, outprefix, sizeof(topic));
          setTopic(topic,sizeof(topic),T_DEV);
          strncat_P(topic, nodes_P, sizeof(topic));
    ///      mqttClient.publish(topic,buf,true);
  }
configLocked--;
#endif
}


void ip_ready_config_loaded_connecting_to_broker() {

    int port = 1883;
    char empty = 0;
    short n = 0;
    char *user = &empty;
    char passwordBuf[16] = "";
    char *password = passwordBuf;

    if (mqttClient.connected())
        {
          lanStatus = RETAINING_COLLECTING;
          return;
        }

    if (!mqttArr || ((n = aJson.getArraySize(mqttArr)) < 2)) //At least device name and broker IP must be configured
        {
          lanStatus = READ_RE_CONFIG;
          return;
        }


    deviceName = getStringFromConfig(mqttArr, 0);
    infoSerial<<F("Device Name:")<<deviceName<<endl;
    
  //  #ifdef OTA
  //  ArduinoOTA.setDeviceName(deviceName);
  //  #endif

    #ifdef MDNS_ENABLE
    mdns.setName(deviceName);
    #endif

//debugSerial<<F("N:")<<n<<endl;

    char *servername = getStringFromConfig(mqttArr, 1);
    if (n >= 3) port = aJson.getArrayItem(mqttArr, 2)->valueint;
    if (n >= 4) user = getStringFromConfig(mqttArr, 3);
    //if (!loadFlash(OFFSET_MQTT_PWD, passwordBuf, sizeof(passwordBuf)) && (n >= 5))
    if (!sysConf.getMQTTpwd(passwordBuf, sizeof(passwordBuf)) && (n >= 5))
        {
            password = getStringFromConfig(mqttArr, 4);
            infoSerial<<F("Using MQTT password from config")<<endl;
        }

    mqttClient.setServer(servername, port);
    mqttClient.setCallback(mqttCallback);

    char willMessage[16];
    char willTopic[32];

    strncpy_P(willMessage,disconnected_P,sizeof(willMessage));

      //  strncpy_P(willTopic, outprefix, sizeof(willTopic));
    setTopic(willTopic,sizeof(willTopic),T_DEV);

    strncat_P(willTopic, state_P, sizeof(willTopic));


    infoSerial<<F("\nAttempting MQTT connection to ")<<servername<<F(":")<<port<<F(" user:")<<user<<F(" ...");
        if (!strlen(user))
          {
            user = NULL;
            password= NULL;
          }
        //    wdt_dis();  //potential unsafe for ethernetIdle(), but needed to avoid cyclic reboot if mosquitto out of order
    if (mqttClient.connect(deviceName, user, password,willTopic,MQTTQOS1,true,willMessage))
          {
            mqttErrorRate = 0;
            infoSerial<<F("connected as ")<<deviceName <<endl;
            configOk = true;
            // ... Temporary subscribe to status topic
            char buf[MQTT_TOPIC_LENGTH+1];

            setTopic(buf,sizeof(buf),T_OUT);
            strncat(buf, "+/+/#", sizeof(buf));      // Only on separated cmd/val topics
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

            onMQTTConnect();
            // if (_once) {DMXput(); _once=0;}
            lanStatus = RETAINING_COLLECTING;//4;
            timerLanCheckTime = millis();// + 5000;
            infoSerial<<F("Awaiting for retained topics");
        } else
           {
            errorSerial<<F("failed, rc=")<<mqttClient.state()<<F(" try again in 5 seconds")<<endl;
            timerLanCheckTime = millis();// + 5000;
#ifdef RESTART_LAN_ON_MQTT_ERRORS
            mqttErrorRate++;
                        if(mqttErrorRate>50){
                            errorSerial<<F("Too many MQTT connection errors. Restart LAN"));
                            mqttErrorRate=0;
#ifdef RESET_PIN
                            resetHard();
#endif
                            lanStatus=DO_REINIT;// GO INITIAL_STATE;
                            return;
                        }
#endif

            lanStatus = DO_RECONNECT;//12;
        }

}


void onInitialStateInitLAN() {
#if defined(WIFI_ENABLE)

#if defined(WIFI_MANAGER_DISABLE)
    if(WiFi.status() != WL_CONNECTED) {
                WiFi.mode(WIFI_STA); // ESP 32 - WiFi.disconnect(); instead
                infoSerial<<F("WIFI AP/Password:")<<QUOTE(ESP_WIFI_AP)<<F("/")<<QUOTE(ESP_WIFI_PWD)<<endl;

                #ifndef ARDUINO_ARCH_ESP32
                wifi_set_macaddr(STATION_IF,sysConf.mac); //ESP32 to check
                #endif

                WiFi.begin(QUOTE(ESP_WIFI_AP), QUOTE(ESP_WIFI_PWD));

          //      int wifi_connection_wait = 10000;


          //      while (WiFi.status() != WL_CONNECTED && wifi_connection_wait > 0) {
          //          delay(500);
          //          wifi_connection_wait -= 500;
          //          debugSerial<<".";
          //          yield();
          //      }
          //      wifiInitialized = true; //???
            }
#endif
lanStatus = AWAITING_ADDRESS;
WiFiAwaitingTime = millis();// + 60000L;
return;
/*
if (WiFi.status() == WL_CONNECTED) {
        infoSerial<<F("WiFi connected. IP address: ")<<WiFi.localIP()<<endl;
        wifiInitialized = true;
        lanStatus = HAVE_IP_ADDRESS;//1;
//setupOTA();

    } else
    {
        errorSerial<<F("\nProblem with WiFi!");
        lanStatus = DO_REINIT;
        //timerLanCheckTime = millis() + DHCP_RETRY_INTERVAL;
    }
*/
#else // Ethernet connection

    //macAddress * mac = sysConf.getMAC();
    IPAddress ip, dns, gw, mask;
    int res = 1;
    infoSerial<<F("Starting lan")<<endl;
    if (sysConf.getIP(ip)) {
        infoSerial<<F("Loaded from flash IP:");
        printIPAddress(ip);
        if (sysConf.getDNS(dns)) {
            infoSerial<<F(" DNS:");
            printIPAddress(dns);
            if (sysConf.getGW(gw)) {
                infoSerial<<F(" GW:");
                printIPAddress(gw);
                if (sysConf.getMask(mask)) {
                    infoSerial<<F(" MASK:");
                    printIPAddress(mask);
                    Ethernet.begin(sysConf.mac, ip, dns, gw, mask);
                } else Ethernet.begin(sysConf.mac, ip, dns, gw);
            } else Ethernet.begin(sysConf.mac, ip, dns);
        } else Ethernet.begin(sysConf.mac, ip);
    infoSerial<<endl;
    lanStatus = HAVE_IP_ADDRESS;
    }
    else {
        infoSerial<<F("\nuses DHCP\n");
        wdt_dis();

        #if defined(ARDUINO_ARCH_STM32)
                res = Ethernet.begin(sysConf.mac);
        #else
                res = Ethernet.begin(sysConf.mac, 12000);
        #endif
        wdt_en();
        wdt_res();


    if (res == 0) {
        errorSerial<<F("Failed to configure Ethernet using DHCP. You can set ip manually!")<<F("'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n");
        lanStatus = DO_REINIT;//-10;
        //timerLanCheckTime = millis();// + DHCP_RETRY_INTERVAL;
#ifdef RESET_PIN
        resetHard();
#endif
    } else {
        infoSerial<<F("Got IP address:");
        printIPAddress(Ethernet.localIP());
        lanStatus = HAVE_IP_ADDRESS;
        
        #ifdef MDNS_ENABLE
        #ifndef OTA_PORT
        #define OTA_PORT  65280
        #endif

        mdns.begin(Ethernet.localIP(), "lighthub");
        mdns.addServiceRecord(("LightHub controller._http"),
                        OTA_PORT,
                        MDNSServiceTCP);

         mdns.addServiceRecord("Lighthub TXT"
                         "._http",
                         OTA_PORT,
                         MDNSServiceTCP,
                         "\x7path=/2");                

        #endif

    }
  }
    #endif
}


void resetHard() {
#ifdef RESET_PIN
    infoSerial<<F("Reset Arduino with digital pin ");
    infoSerial<<QUOTE(RESET_PIN);
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
    //char valstr[16] = "NIL";
    //char *owEmitString = NULL;
    char *owItem = NULL;

    SetBytes(addr, 8, addrstr);
    addrstr[17] = 0;
    if (!root) return;
    //printFloatValueToStr(currentTemp,valstr);
    debugSerial<<endl<<F("T:")<<currentTemp<<F("<")<<addrstr<<F(">")<<endl;
    aJsonObject *owObj = aJson.getObjectItem(owArr, addrstr);     
    if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
        executeCommand(owObj,-1,itemCmd(currentTemp));

    /*
    if (owObj) {
        owEmitString = getStringFromConfig(owObj, "emit");
        debugSerial<<owEmitString<<F(">")<<endl;
          if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
          {
           if (owEmitString)      // publish temperature to MQTT if configured
            {

#ifdef WITH_DOMOTICZ
            aJsonObject *idx = aJson.getObjectItem(owObj, "idx");
        if (idx && && idx->type ==aJson_String && idx->valuestring) {//DOMOTICZ json format support
            debugSerial << endl << idx->valuestring << F(" Domoticz valstr:");
            char valstr[50];
            sprintf(valstr, "{\"idx\":%s,\"svalue\":\"%.1f\"}", idx->valuestring, currentTemp);
            debugSerial << valstr;
            if (mqttClient.connected() && !ethernetIdleCount)
                  mqttClient.publish(owEmitString, valstr);
            return;
        }
#endif

            //strcpy_P(addrstr, outprefix);
            setTopic(addrstr,sizeof(addrstr),T_OUT);
            strncat(addrstr, owEmitString, sizeof(addrstr));
            if (mqttClient.connected() && !ethernetIdleCount)
                  mqttClient.publish(addrstr, valstr);
        }
        // And translate temp to internal items
        owItem = getStringFromConfig(owObj, "item");
        if (owItem)
            thermoSetCurTemp(owItem, currentTemp);  ///TODO: Refactore using Items interface
        } // if valid temperature
  } // if Address in config
  else debugSerial<<addrstr<<F(">")<<endl; // No item found
*/
}

#endif //_owire

void cmdFunctionHelp(int arg_cnt, char **args)
{
    printFirmwareVersionAndBuildOptions();
    printCurentLanConfig();
//    printFreeRam();
    infoSerial<<F("\nUse these commands: 'help' - this text\n"
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
    infoSerial << F("Current LAN config(ip,dns,gw,subnet):");
    printIPAddress(Ethernet.localIP());
//    printIPAddress(Ethernet.dnsServerIP());
    printIPAddress(Ethernet.gatewayIP());
    printIPAddress(Ethernet.subnetMask());
}

void cmdFunctionKill(int arg_cnt, char **args) {
    for (byte i = 1; i < 20; i++) {
        delay(1000);
        infoSerial<<i;
    };
}

void cmdFunctionReboot(int arg_cnt, char **args) {
    infoSerial<<F("Soft rebooting...");
    softRebootFunc();
}

void applyConfig() {
    if (!root) return;
configLocked++;

items = aJson.getObjectItem(root, "items");
topics = aJson.getObjectItem(root, "topics");
inputs = aJson.getObjectItem(root, "in");
mqttArr = aJson.getObjectItem(root, "mqtt");


setupSyslog();


#ifdef _dmxin
    int itemsCount;
    dmxArr = aJson.getObjectItem(root, "dmxin");
    if (dmxArr && (itemsCount = aJson.getArraySize(dmxArr))) {
        DMXinSetup(itemsCount * 4);
        infoSerial<<F("DMX in started. Channels:")<<itemsCount * 4<<endl;
    }
#endif
#ifdef _dmxout
    int maxChannels;
    short numParams;
    aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
    if (dmxoutArr &&  (numParams=aJson.getArraySize(dmxoutArr)) >=1 ) {
        DMXoutSetup(maxChannels = aJson.getArrayItem(dmxoutArr, numParams-1)->valueint);
        infoSerial<<F("DMX out started. Channels: ")<<maxChannels<<endl;
        debugSerial<<F("Free:")<<freeRam()<<endl;
    }
#endif
#ifdef _modbus
    modbusObj = aJson.getObjectItem(root, "modbus");
#endif

#ifdef _owire
    owArr = aJson.getObjectItem(root, "ow");
    if (owArr && !owReady) {
        aJsonObject *item = owArr->child;
        owReady = owSetup(&Changed);
        if (owReady) infoSerial<<F("One wire Ready\n");
        t_count = 0;

        while (item && owReady) {
            if ((item->type == aJson_Object)) {
                DeviceAddress addr;
                //infoSerial<<F("Add:")),infoSerial<<item->name);
                SetAddr(item->name, addr);
                owAdd(addr);
            }
            yield();
            item = item->next;
        }
    }
#endif


// Digital output related Items initialization
    pollingItem=NULL;
    if (items) {
        aJsonObject * item = items->child;
        while (items && item)
            if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
                Item it(item);
                if (it.isValid() && !it.Setup()) {
                  //Legacy Setup
                    short inverse = 0;
                    int pin=it.getArg();
                    if (pin<0) {pin=-pin; inverse = 1;}
                    int cmd = it.getCmd();
                    switch (it.itemType) {
                        case CH_THERMO:
                            if (cmd<1) it.setCmd(CMD_OFF);
                            pinMode(pin, OUTPUT);
                            digitalWrite(pin, false); //Initially, all thermostates are LOW (OFF for electho heaters, open for water NO)
                            debugSerial<<F("Thermo:")<<pin<<F("=LOW")<<F(";");
                            break;
                        case CH_RELAY:
                        {
                            int k;
                            pinMode(pin, OUTPUT);
                            if (inverse)
                            digitalWrite(pin, k = ((cmd == CMD_ON) ? LOW : HIGH));
                            else
                            digitalWrite(pin, k = ((cmd == CMD_ON) ? HIGH : LOW));
                            debugSerial<<F("Pin:")<<pin<<F("=")<<k<<F(";");
                        }
                            break;
                    } //switch
                } //isValid
                yield();
                item = item->next;
            }  //if
        pollingItem = items->child;
    }
    debugSerial<<endl;

   inputSetup();

    printConfigSummary();
configLoaded=true;
if (sysConf.getSaveSuccedConfig()) cmdFunctionSave(0,NULL);
configLocked--;
}

void printConfigSummary() {
    infoSerial<<F("\nConfigured:")<<F("\nitems ");
    printBool(items);
    infoSerial<<F("\ninputs ");
    printBool(inputs);
#ifndef MODBUS_DISABLE
    infoSerial<<F("\nmodbus v1 (+)");
//    printBool(modbusObj);
#endif
#ifndef MBUS_DISABLE
    infoSerial<<F("\nmodbus v2");
    printBool(modbusObj);
#endif
    infoSerial<<F("\nmqtt ");
    printBool(mqttArr);
#ifdef _owire
    infoSerial<<F("\n1-wire ");
    printBool(owArr);
#endif
/*
#ifdef SYSLOG_ENABLE
    infoSerial<<F("\nudp syslog ");
    printBool(udpSyslogArr);
#endif
*/
    infoSerial << endl;
    infoSerial<<F("RAM=")<<freeRam()<<endl;
}

void cmdFunctionLoad(int arg_cnt, char **args) {
    loadConfigFromEEPROM();
}


int loadConfigFromEEPROM()
{
    char ch;
    infoSerial<<F("Loading Config from EEPROM")<<endl;
    JSONStream.seek();    
    if (JSONStream.peek() == '{') {
        aJsonStream as = aJsonStream(&JSONStream);
        cleanConf();
        root = aJson.parse(&as);
        if (!root) {
            errorSerial<<F("load failed")<<endl;
            return 0;
        }
        infoSerial<<F("Loaded")<<endl;
        applyConfig();
        ethClient.stop(); //Refresh MQTT connect to get retained info
        return 1;
    } else {
        JSONStream.write(255); //truncate garbage
        infoSerial<<F("No stored config")<<endl;
        return 0;
    }
    return 0;
}


#if defined(__SAM3X8E__) 

//#define saveBufLen 16000
void cmdFunctionSave(int arg_cnt, char **args)
{
  char* outBuf = (char*) malloc(MAX_JSON_CONF_SIZE); /* XXX: Dynamic size. */
  if (outBuf == NULL)
    {
      return;
    }
  infoSerial<<F("Saving config to EEPROM..")<<endl;
  aJsonStringStream stringStream(NULL, outBuf, MAX_JSON_CONF_SIZE);
  aJson.print(root, &stringStream);
  int len = strlen(outBuf);
  outBuf[len++]= EOF;
  
  //EEPROM.write(EEPROM_offsetJSON,(byte*) outBuf,len);
  JSONStream.seek();
  size_t res = JSONStream.write((byte*) outBuf,len);
  free (outBuf);
  infoSerial<<res<< F("bytes from ")<<len<<F(" are saved to EEPROM")<<endl;
}

#else
void cmdFunctionSave(int arg_cnt, char **args)
{
    //aJsonEEPROMStream jsonEEPROMStream = aJsonEEPROMStream(EEPROM_offsetJSON);
    //flashStream fs = flashStream(EEPROM_offsetJSON);
    JSONStream.seek();
    aJsonStream jsonEEPROMStream = aJsonStream(&JSONStream);
    
    infoSerial<<F("Saving config to EEPROM..");

    aJson.print(root, &jsonEEPROMStream);
    JSONStream.putEOF();


    infoSerial<<F("Saved to EEPROM")<<endl;
}

#endif

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

  //  switch (arg_cnt) {
    //    case 5:
            if (arg_cnt>4 && _inet_aton(args[4], ip)) sysConf.setMask(ip);
            else sysConf.setMask(ip0);
    //    case 4:
            if (arg_cnt>3 && _inet_aton(args[3], ip)) sysConf.setGW(ip);
            else sysConf.setGW(ip0);
    //    case 3:
            if (arg_cnt>2 && _inet_aton(args[2], ip)) sysConf.setDNS(ip);
            else sysConf.setDNS(ip0);
    //    case 2:
            if (arg_cnt>1 && _inet_aton(args[1], ip)) sysConf.setIP(ip);
            else sysConf.setIP(ip0);
    //        break;

    //    case 1: //dynamic IP
       if (arg_cnt==1)
       {
        sysConf.setIP(ip0);
        infoSerial<<F("Set dynamic IP\n");
      }
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
    //}
    infoSerial<<F("Saved\n");
}

void cmdFunctionClearEEPROM(int arg_cnt, char **args){
   sysConf.clear();
   infoSerial<<F("EEPROM cleared\n");
}

void cmdFunctionPwd(int arg_cnt, char **args)
{ //char empty[]="";
    if (arg_cnt)
         sysConf.setMQTTpwd(args[1]);
    else sysConf.setMQTTpwd();
    infoSerial<<F("Password updated\n");
}

void cmdFunctionOTAPwd(int arg_cnt, char **args)
{ //char empty[]="";
    if (arg_cnt)
         sysConf.setOTApwd(args[1]);
    else sysConf.setOTApwd();
    infoSerial<<F("Password updated\n");
}

void cmdFunctionSetMac(int arg_cnt, char **args) {
    char dummy;
    uint8_t mac[6];
    if (sscanf(args[1], "%x:%x:%x:%x:%x:%x%c", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &dummy) < 6) {
        errorSerial<<F("could not parse: ")<<args[1];
        return;
    }
    sysConf.setMAC(mac);
    printMACAddress();
    
    //for (short i = 0; i < 6; i++) { EEPROM.write(i, mac[i]); }
    infoSerial<<F("Updated\n");
}

void cmdFunctionGet(int arg_cnt, char **args) {
    lanStatus= loadConfigFromHttp(arg_cnt, args);
    ethClient.stop(); //Refresh MQTT connect to get retained info
}

void printBool(bool arg) { (arg) ? infoSerial<<F("+") : infoSerial<<F("-"); }


lan_status loadConfigFromHttp(int arg_cnt, char **args)
{
    //macAddress * mac = sysConf.getMAC();
    int responseStatusCode = 0;
    char URI[64];
    char configServer[32]="";
    if (arg_cnt > 1) {
        strncpy(configServer, args[1], sizeof(configServer) - 1);
        sysConf.setServer(configServer);
        //saveFlash(OFFSET_CONFIGSERVER, configServer);
        infoSerial<<configServer<<F(" Saved")<<endl;
    } else if (!sysConf.getServer(configServer,sizeof(configServer)))
        {
        strncpy_P(configServer,configserver,sizeof(configServer));
        infoSerial<<F(" Default config server used: ")<<configServer<<endl;
      }
#ifndef DEVICE_NAME
    snprintf(URI, sizeof(URI), "/cnf/%02x-%02x-%02x-%02x-%02x-%02x.config.json", sysConf.mac[0], sysConf.mac[1], sysConf.mac[2], sysConf.mac[3], sysConf.mac[4],
             sysConf.mac[5]);
#else
#ifndef FLASH_64KB
    snprintf(URI, sizeof(URI), "/cnf/%s_config.json",QUOTE(DEVICE_NAME));
#else
    strncpy(URI, "/cnf/", sizeof(URI));
    strncat(URI, QUOTE(DEVICE_NAME), sizeof(URI));
    strncat(URI, "_config.json", sizeof(URI));
#endif
#endif
    infoSerial<<F("Config URI: http://")<<configServer<<URI<<endl;

#if defined(ARDUINO_ARCH_AVR)
    FILE *configStream;
    //wdt_dis();
    //if (freeRam()<512) cleanConf();

    HTTPClient hclient(configServer, 80);
    debugSerial<<F("free ")<<freeRam()<<endl;delay(100);
    // FILE is the return STREAM type of the HTTPClient
    configStream = hclient.getURI(URI);
    debugSerial<<F("hclient")<<endl;delay(100);
    responseStatusCode = hclient.getLastReturnCode();
    debugSerial<<F("retcode ")<<responseStatusCode<<endl;delay(100);
    //wdt_en();
    
    if (configStream != NULL) {
        if (responseStatusCode == 200) {

            infoSerial<<F("got Config\n"); delay(500);
            char c;
            aJsonFileStream as = aJsonFileStream(configStream);
            noInterrupts();
            cleanConf();
            root = aJson.parse(&as);
            interrupts();
            hclient.closeStream(configStream);  // this is very important -- be sure to close the STREAM

            if (!root) {
                errorSerial<<F("Config parsing failed\n");
//                timerLanCheckTime = millis();// + 15000;
                return READ_RE_CONFIG;//-11;
            } else {
            infoSerial<<F("Applying.\n");
                applyConfig();
            infoSerial<<F("Done.\n");

            }

        } else {
            errorSerial<<F("ERROR: Server returned ");
            errorSerial<<responseStatusCode<<endl;
//            timerLanCheckTime = millis();// + 5000;
            return READ_RE_CONFIG;//-11;
        }

    } else {
        debugSerial<<F("failed to connect\n");
//        debugSerial<<F(" try again in 5 seconds\n");
//        timerLanCheckTime = millis();// + 5000;
        return READ_RE_CONFIG;//-11;
    }
#endif
#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32) || defined (NRF5) //|| defined(ARDUINO_ARCH_ESP32) //|| defined(ARDUINO_ARCH_ESP8266)
    #if defined(WIFI_ENABLE)
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
    infoSerial<<F("HTTP Status code: ")<<responseStatusCode<<endl;
    
//delay(1000);
    if (responseStatusCode == 200) {
        debugSerial<<F("GET Response: ")<<response<<endl;
        debugSerial<<F("Free:")<<freeRam()<<endl;
        cleanConf();
        debugSerial<<F("Configuration cleaned")<<endl;
        debugSerial<<F("Free:")<<freeRam()<<endl;
        root = aJson.parse((char *) response.c_str());

        if (!root) {
            errorSerial<<F("Config parsing failed\n");
            return READ_RE_CONFIG;//-11; //Load from NVRAM
        } else {
            debugSerial<<F("Parsed. Free:")<<freeRam()<<endl;
            //debugSerial<<response;
            applyConfig();
            infoSerial<<F("Done.\n");
        }
      } else  {
          errorSerial<<F("Config retrieving failed\n");
          return READ_RE_CONFIG;//-11; //Load from NVRAM
      }
    } else {
        errorSerial<<F("Connect failed\n");
        return READ_RE_CONFIG;//-11; //Load from NVRAM
    }
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) //|| defined (NRF5)
    HTTPClient httpClient;
   // WiFiClient wifiClient;
    #if defined(WIFI_ENABLE)
        WiFiClient configEthClient;
    #else
        EthernetClient configEthClient;
    #endif
    String fullURI = "http://";
    fullURI+=configServer;
    fullURI+=URI;
    
    #if defined(ARDUINO_ARCH_ESP8266) 
    httpClient.begin(configEthClient,fullURI);
    #else
    httpClient.begin(fullURI);
    #endif

    int httpResponseCode = httpClient.GET();
    if (httpResponseCode > 0) {
        infoSerial.printf("[HTTP] GET... code: %d\n", httpResponseCode);
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = httpClient.getString();
            debugSerial<<response;
            cleanConf();
            root = aJson.parse((char *) response.c_str());
            if (!root) {
                errorSerial<<F("Config parsing failed\n");
                return READ_RE_CONFIG;
            } else {
                infoSerial<<F("Config OK, Applying\n");
                applyConfig();
                infoSerial<<F("Done.\n");
            }
        } else {
            errorSerial<<F("Config retrieving failed\n");
            return READ_RE_CONFIG;//-11; //Load from NVRAM
        }
    } else {
        errorSerial.printf("[HTTP] GET... failed, error: %s\n", httpClient.errorToString(httpResponseCode).c_str());
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



volatile unsigned long timerCount=0; 
volatile int16_t timerNumber=-1;

void TimerHandler(void)
{   interrupts();
    timerCount=micros();
     if (configLoaded) inputLoop(CHECK_INTERRUPT);
    timerCount=micros()-timerCount;

}

#if defined(__SAM3X8E__) && defined (TIMER_INT)
int16_t attachTimer(double microseconds, timerCallback callback, const char* TimerName)
{  
    if (timerNumber!=-1) return timerNumber;

    DueTimerInterrupt dueTimerInterrupt = DueTimer.getAvailable();
    dueTimerInterrupt.attachInterruptInterval(microseconds, callback);
    timerNumber = dueTimerInterrupt.getTimerNumber();
    debugSerial<<TimerName<<F(" attached to Timer(")<<timerNumber<<F(")")<<endl;
  return timerNumber;
}
#endif

void setup_main() {

  #if defined(__SAM3X8E__)
  memset(&UniqueID,0,sizeof(UniqueID));
  #endif

  #if defined(M5STACK)
   // Initialize the M5Stack object
   M5.begin();
  #endif

    setupCmdArduino();
    printFirmwareVersionAndBuildOptions();

    //Checkin EEPROM integrity (signature)

    if (!sysConf.isValidSysConf()) sysConf.clear();                   
  //  scan_i2c_bus();

#ifdef SD_CARD_INSERTED
    sd_card_w5100_setup();
#endif
    setupMacAddress();
/*
#if defined(ARDUINO_ARCH_ESP8266) 
      EEPROM.begin(ESP_EEPROM_SIZE);
#endif
*/

#if defined(FS_PREPARE)

//Initialize File System
  if(SPIFFS.begin(true))
  {
    debugSerial<<("SPIFFS Initialize....ok")<<endl;
  }
  else
  {
    debugSerial<<("SPIFFS Initialization...failed")<<endl;
  }
#endif

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
        setupOwIdle(&owIdle);
    #endif

    mqttClient.setCallback(mqttCallback);

#ifdef _artnet
    ArtnetSetup();
#endif

#if defined(WIFI_ENABLE) and not defined(WIFI_MANAGER_DISABLE)
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

    #if defined(W5500_CS_PIN) && ! defined(WIFI_ENABLE)
        Ethernet.init(W5500_CS_PIN);
        infoSerial<<F("Use W5500 pin: ");
        infoSerial<<QUOTE(W5500_CS_PIN)<<endl;
    #endif

        loadConfigFromEEPROM();       
}

void printFirmwareVersionAndBuildOptions() {
    infoSerial<<F("\nLazyhome.ru LightHub controller ")<<F(QUOTE(PIO_SRC_REV))<<F(" C++ version:")<<F(QUOTE(__cplusplus))<<endl;
#ifdef CONTROLLINO
    infoSerial<<F("\n(+)CONTROLLINO");
#endif
#ifndef WATCH_DOG_TICKER_DISABLE
    infoSerial<<F("\n(+)WATCHDOG");
#else
    infoSerial<<F("\n(-)WATCHDOG");
#endif
    infoSerial<<F("\nConfig server:")<<F(CONFIG_SERVER)<<F("\nFirmware MAC Address ")<<F(QUOTE(CUSTOM_FIRMWARE_MAC));
#ifdef DISABLE_FREERAM_PRINT
    infoSerial<<F("\n(-)FreeRam printing");
#else
    infoSerial<<F("\n(+)FreeRam printing");
#endif

#ifdef USE_1W_PIN
    infoSerial<<F("\n(-)DS2482-100 USE_1W_PIN=")<<QUOTE(USE_1W_PIN);
#else
    infoSerial<<F("\n(+)DS2482-100");
#endif

#ifdef WIFI_ENABLE
    infoSerial<<F("\n(+)WiFi");
#elif Wiz5500
    infoSerial<<F("\n(+)WizNet5500");
#elif Wiz5100
    infoSerial<<F("\n(+)Wiznet5100");
#else
    infoSerial<<F("\n(+)Wiznet5x00");
#endif

#ifndef DMX_DISABLE
    infoSerial<<F("\n(+)DMX");
    #ifdef FASTLED
        infoSerial<<F("\n(+)FASTLED");
    #else
        infoSerial<<F("\n(+)ADAFRUIT LED");
    #endif
#else
    infoSerial<<F("\n(-)DMX");
#endif

#ifdef _modbus
    infoSerial<<F("\n(+)MODBUS");
#else
    infoSerial<<F("\n(-)MODBUS");
#endif

#ifndef OWIRE_DISABLE
    infoSerial<<F("\n(+)OWIRE");
#else
    infoSerial<<F("\n(-)OWIRE");
#endif
#ifndef DHT_DISABLE
    infoSerial<<F("\n(+)DHT");
#else
    infoSerial<<F("\n(-)DHT");
#endif

#ifndef COUNTER_DISABLE
    infoSerial<<F("\n(+)COUNTER");
#else
    infoSerial<<F("\n(-)COUNTER");
#endif

#ifdef SD_CARD_INSERTED
    infoSerial<<F("\n(+)SDCARD");
#endif

#ifdef RESET_PIN
    infoSerial<<F("\n(+)HARDRESET on pin=")<<QUOTE(RESET_PIN);
#else
    infoSerial<<F("\n(-)HARDRESET, using soft");
#endif

#ifdef RESTART_LAN_ON_MQTT_ERRORS
    infoSerial<<F("\n(+)RESTART_LAN_ON_MQTT_ERRORS");
#else
    infoSerial<<F("\n(-)RESTART_LAN_ON_MQTT_ERRORS");
#endif


#ifndef CSSHDC_DISABLE
    infoSerial<<F("\n(+)CCS811 & HDC1080");
#else
    infoSerial<<F("\n(-)CCS811 & HDC1080");
#endif
#ifndef AC_DISABLE
    infoSerial<<F("\n(+)AC HAIER");
#else
    infoSerial<<F("\n(-)AC HAIER");
#endif
#ifndef MOTOR_DISABLE
    infoSerial<<F("\n(+)MOTOR CTR");
#else
    infoSerial<<F("\n(-)MOTOR CTR");
#endif
#ifndef SPILED_DISABLE
    infoSerial<<F("\n(+)SPI LED");
#else
    infoSerial<<F("\n(-)SPI LED");
#endif

#ifdef OTA
    infoSerial<<F("\n(+)OTA");
#else
    infoSerial<<F("\n(-)OTA");
#endif

#ifdef ARTNET_ENABLE
infoSerial<<F("\n(+)ARTNET");
#else
infoSerial<<F("\n(-)ARTNET");
#endif

#ifdef MCP23017
infoSerial<<F("\n(+)MCP23017");
#else
infoSerial<<F("\n(-)MCP23017");
#endif

#ifndef PID_DISABLE
infoSerial<<F("\n(+)PID");
#else
infoSerial<<F("\n(-)PID");
#endif

#ifdef SYSLOG_ENABLE
infoSerial<<F("\n(+)SYSLOG");
#else
infoSerial<<F("\n(-)SYSLOG");
#endif

#ifdef UARTBRIDGE_ENABLE
infoSerial<<F("\n(+)UARTBRIDGE");
#else
infoSerial<<F("\n(-)UARTBRIDGE");
#endif

#ifdef MDNS_ENABLE
infoSerial<<F("\n(+)MDNS");
#else
infoSerial<<F("\n(-)MDNS");
#endif
infoSerial<<endl;

//    WDT_Disable( WDT ) ;
#if defined(__SAM3X8E__)

    debugSerial<<F("Reading 128 bits unique identifier")<<endl ;
    ReadUniqueID( UniqueID.UID_Long ) ;

    infoSerial<< F ("ID: ");
    for (byte b = 0 ; b < 4 ; b++)
    infoSerial<< _HEX((unsigned int) UniqueID.UID_Long [b]);
    infoSerial<< endl;
#endif

}



void publishStat(){
  long fr = freeRam();
  char topic[64];
  char intbuf[16];
  uint32_t ut = millis()/1000UL;
  if (!mqttClient.connected()  || ethernetIdleCount) return;
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
if (!sysConf.getMAC()) {
    infoSerial<<F("No MAC configured: set firmware's MAC\n");

    #if defined (CUSTOM_FIRMWARE_MAC) //Forced MAC from compiler's directive
    const char *macStr = QUOTE(CUSTOM_FIRMWARE_MAC);//colon(:) separated from build options
    parseBytes(macStr, ':', sysConf.mac, 6, 16);

    sysConf.mac[0]&=0xFE;
    sysConf.mac[0]|=2;

    #elif defined(WIFI_ENABLE)
    //Using original MPU MAC
    WiFi.begin();
    WiFi.macAddress(sysConf.mac);

    #elif defined(__SAM3X8E__)
    //Lets make MAC from MPU serial#
    sysConf.mac[0]=0xDE;

    for (byte b = 0 ; b < 5 ; b++)
              sysConf.mac[b+1]=UniqueID.UID_Byte [b*3] + UniqueID.UID_Byte [b*3+1] + UniqueID.UID_Byte [b*3+2];

    #elif defined DEFAULT_FIRMWARE_MAC
    uint8_t defaultMac[6] = DEFAULT_FIRMWARE_MAC;//comma(,) separated hex-array, hard-coded
    memcpy(sysConf.mac,defaultMac,6);
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
    //cmdAdd("req", cmdFunctionReq);
    cmdAdd("ip", cmdFunctionIp);
    cmdAdd("pwd", cmdFunctionPwd);
    cmdAdd("otapwd", cmdFunctionOTAPwd);
    cmdAdd("clear",cmdFunctionClearEEPROM);
    cmdAdd("reboot",cmdFunctionReboot);
}

void loop_main() { 
  statusLED.poll();

  #if defined(M5STACK)
   // Initialize the M5Stack object
   yield();
   M5.update();
  #endif

    wdt_res();
    yield();
    cmdPoll();
    if (lanLoop() > HAVE_IP_ADDRESS) { 
        mqttClient.loop();

        #if defined(OTA)
        yield();
        ArduinoOTA.poll();
        #endif

#ifdef _artnet
        yield();
        if (artnet) artnet->read();  ///hung
#endif
    }

#ifdef _owire
    yield();
    if (owReady && owArr) owLoop();
#endif

#ifdef _dmxin
    yield();
    DMXCheck();
#endif

    if (items) {
        if (isNotRetainingStatus()) pollingLoop();
        yield();
        thermoLoop();
    }

    yield();
    inputLoop(CHECK_INPUT);

#if defined (_espdmx)
    yield();
    dmxout.update();
#endif

#ifdef MDNS_ENABLE
        mdns.run();
#endif
}

void owIdle(void) {
#ifdef _artnet
    if (artnet && (lanStatus>=HAVE_IP_ADDRESS)) artnet->read();
#endif

    wdt_res();
    return;

#ifdef _dmxin
    yield();
    DMXCheck();
#endif

#if defined (_espdmx)
    yield();
    dmxout.update();
#endif
}
void ethernetIdle(void){
ethernetIdleCount++;
    wdt_res();
    yield();
    inputLoop(CHECK_INPUT);
ethernetIdleCount--;
};

void modbusIdle(void) {
    statusLED.poll();
    yield();
    cmdPoll();
    wdt_res();
    if (lanLoop() > HAVE_IP_ADDRESS) {
        yield();
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();
#endif
        yield();
        inputLoop(CHECK_INPUT);
    }
#ifdef MDNS_ENABLE
        mdns.run();
#endif

#ifdef _dmxin
    DMXCheck();
#endif

#if defined (_espdmx)
    dmxout.update();
#endif
}

volatile int8_t inputLoopBusy = 0;
void inputLoop(short cause) {
    if (!inputs || inputLoopBusy) return;
inputLoopBusy++;

configLocked++;
    //if (millis() > timerInputCheck) 
    if (isTimeOver(timerInputCheck,millis(),INTERVAL_CHECK_INPUT))
    {
        aJsonObject *input = inputs->child;

        while (input) {
            if (input->type == aJson_Object)  {
                // Check for nested inputs
                aJsonObject * inputArray = aJson.getObjectItem(input, "act");
                if  (inputArray && (inputArray->type == aJson_Array))
                    {
                      aJsonObject *inputObj = inputArray->child;

                      while(inputObj)
                        {
                          Input in(inputObj,input);
                          in.Poll(cause);

                          //yield();
                          inputObj = inputObj->next;

                        }
                    }
                else
                {
                 Input in(input);
                 in.Poll(cause);
                }
            }
            //yield();
            input = input->next;
        }
        if (cause != CHECK_INTERRUPT) timerInputCheck = millis();// + INTERVAL_CHECK_INPUT;
        inCache.invalidateInputCache();
    }

    //if (millis() > timerSensorCheck) 
    if (cause != CHECK_INTERRUPT && isTimeOver(timerSensorCheck,millis(),INTERVAL_CHECK_SENSOR))
    {
        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.Poll(CHECK_SENSOR);
            }
            yield();
            input = input->next;
        }
        timerSensorCheck = millis();// + INTERVAL_CHECK_SENSOR;
    }
configLocked--;
inputLoopBusy--;
}


void inputSetup(void) {
    if (!inputs) return;
configLocked++;
        aJsonObject *input = inputs->child;
        while (input) {
            if ((input->type == aJson_Object)) {
                Input in(input);
                in.setup();
            }
            yield();
            input = input->next;
        }
    #if defined(__SAM3X8E__) && defined (TIMER_INT)  
    // Interval in microsecs
    attachTimer(TIMER_CHECK_INPUT * 1000, TimerHandler, "ITimer");  
    #endif     
configLocked--;
}


void pollingLoop(void) {
 if (!items) return;   
// FAST POLLINT - as often AS possible every item
configLocked++;
    aJsonObject * item = items->child;
    while (items && item)
        if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
            Item it(item);
            if (it.isValid()) {
               it.Poll(POLLING_FAST);
            } //isValid
            yield();
            item = item->next;
        }  //if
configLocked--;
// SLOW POLLING
    boolean done = false;
    if (lanStatus == RETAINING_COLLECTING) return;
    //if (millis() > timerPollingCheck) 
    if (isTimeOver(timerPollingCheck,millis(),INTERVAL_SLOW_POLLING))    
        {
        while (pollingItem && !done) {
            if (pollingItem->type == aJson_Array) {
                Item it(pollingItem);
                uint32_t ret = it.Poll(POLLING_SLOW);
                if (ret)
                {
                  timerPollingCheck = millis();// +  ret;  //INTERVAL_CHECK_MODBUS;
                  done = true;
                }
            }//if
            if (!pollingItem) return; //Config was re-readed
            yield();
            pollingItem = pollingItem->next;
            if (!pollingItem) {
                pollingItem = items->child;
                return;
            } //start from 1-st element
        } //while
    }//if
}

////// Legacy Thermostat code below - to be moved in module /////
void thermoRelay(int pin, bool on)
{   
    int thermoPin = abs(pin);
    pinMode(thermoPin, OUTPUT);
    
    if (on)
            {
            digitalWrite(thermoPin, (pin<0)?LOW:HIGH); 
            debugSerial<<F(" ON")<<endl;
            }
     else   
     {
            digitalWrite(thermoPin, (pin<0)?HIGH:LOW); 
            debugSerial<<F(" OFF")<<endl;
     }       

}

void thermoLoop(void) {

 if (!isTimeOver(timerThermostatCheck,millis(),THERMOSTAT_CHECK_PERIOD))
        return;
    if (!items) return;
    bool thermostatCheckPrinted = false;
    configLocked++;
    for (aJsonObject *thermoItem = items->child; thermoItem; thermoItem = thermoItem->next) {
        if ((thermoItem->type == aJson_Array) && (aJson.getArrayItem(thermoItem, I_TYPE)->valueint == CH_THERMO)) 
        {        
                Item thermostat(thermoItem); //Init Item
                if (!thermostat.isValid()) continue;
                itemCmd thermostatCmd(&thermostat); // Got itemCmd

                thermostatStore tStore;
                tStore.asint=thermostat.getExt();

                int   thermoPin     = thermostat.getArg(0);
                float thermoSetting = thermostatCmd.getFloat();
                int   thermoStateCommand = thermostat.getCmd();
                float curTemp       = (float) tStore.tempX100/100.;
                bool  active        = thermostat.isActive();

                debugSerial << F(" Set:") << thermoSetting << F(" Cur:") << curTemp
                            << F(" cmd:") << thermoStateCommand;

                if (tStore.timestamp16) //Valid temperature
                   {        
                        if (isTimeOver(tStore.timestamp16,millisNZ(8) & 0xFFFF,PERIOD_THERMOSTAT_FAILED,0xFFFF))
                        {
                        errorSerial<<thermoItem->name<<F(" Alarm Expired\n");
                        mqttClient.publish("/alarm/snsr", thermoItem->name);
                        tStore.timestamp16=0; //Stop termostat
                        thermostat.setExt(tStore.asint);
                        thermoRelay(thermoPin,false);
                        }
                         else
                         { // Not expired yet
                            if (curTemp > THERMO_OVERHEAT_CELSIUS) mqttClient.publish("/alarm/ovrht", thermoItem->name); 

                            if (!active) thermoRelay(thermoPin,false);//OFF 
                               else if (curTemp < thermoSetting - THERMO_GIST_CELSIUS) thermoRelay(thermoPin,true);//ON
                                    else if (curTemp >= thermoSetting) thermoRelay(thermoPin,false);//OFF
                                         else debugSerial<<F(" -target zone-")<<endl; // Nothing to do

                         }
                   }
                 else debugSerial<<F(" Expired\n");  
                    thermostatCheckPrinted = true;  
        } //item is termostat
    } //for
  configLocked--;
    timerThermostatCheck = millis();// + THERMOSTAT_CHECK_PERIOD;
publishStat();
#ifndef DISABLE_FREERAM_PRINT
    (thermostatCheckPrinted) ? debugSerial<<F("\nRAM=")<<freeRam()<<F(" Locks:")<<configLocked: debugSerial<<F(" ")<<freeRam()<<F(" Locks:")<<configLocked;
     debugSerial<<F(" Timer:")<<timerCount<<endl;
#endif
 

}

