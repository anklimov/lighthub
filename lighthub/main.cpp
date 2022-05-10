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


#ifdef SYSLOG_ENABLE
#include <Syslog.h>

    #ifndef WIFI_ENABLE
    EthernetUDP udpSyslogClient;
    #else
    WiFiUDP udpSyslogClient;
    #endif
Syslog udpSyslog(udpSyslogClient, SYSLOG_PROTO_BSD);
static char syslogDeviceHostname[16];

Streamlog debugSerial(&debugSerialPort,LOG_DEBUG,&udpSyslog);
Streamlog errorSerial(&debugSerialPort,LOG_ERROR,&udpSyslog,ledRED);
Streamlog infoSerial (&debugSerialPort,LOG_INFO,&udpSyslog);
#else
Streamlog debugSerial(&debugSerialPort,LOG_DEBUG);
Streamlog errorSerial(&debugSerialPort,LOG_ERROR, ledRED);
Streamlog infoSerial (&debugSerialPort,LOG_INFO);
#endif

flashStream sysConfStream;
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
    MDNS mdns(mdnsUDP);
    #endif
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

#ifdef WIFI_ENABLE
volatile uint32_t WiFiAwaitingTime =0;
#endif

volatile uint32_t timerPollingCheck = 0;
volatile uint32_t timerInputCheck = 0;
volatile uint32_t timerLanCheckTime = 0;
volatile uint32_t timerThermostatCheck = 0;
volatile uint32_t timerSensorCheck =0;
volatile unsigned long timerCount=0; 
volatile int16_t  timerNumber=-1;
volatile int8_t   timerHandlerBusy=0;

aJsonObject *pollingItem = NULL;

bool owReady = false;
bool configOk = false; // At least once connected to MQTT
bool configLoaded = false;
bool initializedListeners = false;
volatile int8_t ethernetIdleCount =0;
volatile int8_t configLocked = 0;

#if defined (_modbus)
ModbusMaster node;
#endif

PubSubClient mqttClient(ethClient);

bool wifiInitialized;

int8_t mqttErrorRate=0;

#if defined(__SAM3X8E__)
void watchdogSetup(void) {}    //Do not remove - strong re-definition WDT Init for DUE
#endif

bool cleanConf()
{
  if (!root) return true;
  bool clean = true;
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
    clean = false;
}

debugSerial<<F("Stopping channels ...")<<endl;
timerHandlerBusy++;
//Stoping the channels
if (items)
{
aJsonObject * item = items->child;
    while (item)
    {
        if (item->type == aJson_Array && aJson.getArraySize(item)>0)
        {
            if (item->type == aJson_Array && aJson.getArraySize(item)>0)
            {
                Item it(item->name);
                if (it.isValid()) it.Stop();
                yield();

            }
        item = item->next;
        }
    } 
}
pollingItem = NULL;
debugSerial<<F("Stopped")<<endl;
delay(100);
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
    deviceName = NULL;
    topics = NULL;
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
 timerHandlerBusy--;     
 return clean;
}

bool isNotRetainingStatus() {
  return (lanStatus != RETAINING_COLLECTING);
}
// Custom HTTP request handler
// Return values: 
// 1 - work completed. Dont need to respond, just close socket
// 0 - no match continue with default lib behavior
// 401 - not authorized
// 400 - bad request
// 200 | CONTENT_TYPE - ok + content_type
//  ... same sor other http codes
// if response != "" - put response in http answer

uint16_t httpHandler(Client& client, String request, uint8_t method, long contentLength, bool authorized, String& response )
{
  #ifdef OTA  
    //String response = "";
    debugSerial<<method<<F(" ")<<request<<endl;
    if (method == HTTP_GET && request == (F("/"))) 
        {
          
         ArduinoOTA.sendHttpResponse(client,301,false);  // Send only HTTP header, no close socket 
         client.println(
#ifdef CORS
//Redirect to cloud PWA application               
             String(F("Location: " CORS "/pwa"))
#else
             String(F("Location: /index.html"))
#endif   

             +String(F("?mac="))+sysConf.getMACString()
             +String(F("&ip="))+ toString( Ethernet.localIP())
             +String(F("&port="))+ OTA_PORT 
             +String(F("&name="))+deviceName
             );

         client.println();
         delay(100);
         client.stop();
         return 1;
        }
    

    if (method == HTTP_POST && request.startsWith(F("/item/")))
       {
        if (!authorized) return 401;   
        request.remove(0,6);      
        String body=client.readStringUntil('\n');
        Item   item((char*)request.c_str());
          if (!item.isValid() || !item.Ctrl((char*) body.c_str())) return 400;

        itemCmd ic;
        ic.loadItem(&item,SEND_COMMAND|SEND_PARAMETERS);
        char buf[32];
        response=ic.toString(buf, sizeof(buf));    
        return 200 | HTTP_TEXT_PLAIN;         
       }
    else if (method == HTTP_GET && request.startsWith(F("/item/")))
       {
        if (!authorized) return 401;   
        request.remove(0,6);
        Item   item((char*)request.c_str());
          if (!item.isValid()) return 400; 

         if (item.itemType == CH_GROUP)
           {if (item.isActive()) item.setCmd(CMD_ON); else item.setCmd(CMD_OFF);} 

         itemCmd ic;
         ic.loadItem(&item,SEND_COMMAND|SEND_PARAMETERS);

         char buf[32];
         response=ic.toString(buf, sizeof(buf));
         return 200 | HTTP_TEXT_PLAIN;  
        
       }  
    else if (method == HTTP_GET && request.startsWith(F("/ram/")))
    {
    if (!authorized) return 401;  
    aJsonObject * dumpObject; 
    request.remove(0,5);
    
    if (request == "." && items)  dumpObject = items;
    else if (request != "" && items)
        {
         dumpObject =  aJson.getObjectItem(items, request.c_str());
         if (!dumpObject) return 404;
        }
    else dumpObject = root;

    ArduinoOTA.sendHttpResponse(client,200 | HTTP_TEXT_JSON,false);  // Send only HTTP header, no close socket 
    client.println();
   // char* outBuf = (char*) malloc(MAX_JSON_CONF_SIZE); /* XXX: Dynamic size. */
   // if (outBuf == NULL) return 500;
    aJsonStream socketStream = aJsonStream(&client);
  //aJsonStringStream stringStream(NULL, outBuf, MAX_JSON_CONF_SIZE);
    aJson.print(dumpObject, &socketStream);
    delay(100);
    client.stop();
    return 1;
  
  //size_t res = sysConfStream.write((byte*) outBuf,len);
  //free (outBuf);

    }

    else if (method == HTTP_POST && request.startsWith(F("/command/"))) 
        {
        int result = 400;    
        if (!authorized) return 401;    
        request.remove(0,9);      
        String body=client.readStringUntil('\n');  
        
        request+=" ";
        request+=body;

        debugSerial<<F("Cmd: ")<<request<<endl;
        if (request.equalsIgnoreCase(F("reboot "))) ArduinoOTA.sendHttpResponse(client,200);  
        const char* res=request.c_str();
        result =  cmd_parse((char*) res);   
        if (! result) return 404;
        return result;      
        } 
    else if (method == HTTP_POST && request.startsWith(F("/config.json"))) 
        {
        sysConf.setLoadHTTPConfig(false);
        infoSerial<<(F("Config changed locally, portal disabled"))<<endl;
        sysConf.setETAG("");
        return 0;    
        }
#endif     
    return 0;  //Unknown
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
    if (!payload || !length) {debugSerial<<F("\n")<<F("Empty: [")<<topic<<F("]")<<endl;return;}
    payload[length] = 0;

    int fr = freeRam();

    debugSerial<<F("\n")<<fr<<F(":[")<<topic<<F("] ");

    if (fr < 250+MQTT_TOPIC_LENGTH) {
        errorSerial<<F("OutOfMemory!")<<endl;
        return;// -2;
    }
    
    statusLED.flash(ledBLUE);
    debugSerial<<F("\"")<<(char*)payload<<F("\"")<<endl;
   
    short pfxlen  = 0;
    char * itemName = NULL;
    char * subItem = NULL;
    char savedTopic[MQTT_TOPIC_LENGTH] = "";

// in Retaining status - trying to restore previous state from retained output topic. Retained input topics are not relevant.
if  (lanStatus == RETAINING_COLLECTING) 
{
        pfxlen=inTopic(topic,T_OUT);
        if (!pfxlen) // There is not status topic
           {
            if (mqttClient.isRetained()) 
                {
                pfxlen=inTopic(topic,T_BCST);
                if (!pfxlen) pfxlen = inTopic(topic,T_DEV);   
                if (!pfxlen)  return; // Not command topic ever
                //itemName=topic+pfxlen; 
                if (strrchr(topic,'$')) return;
                //if (itemName[0]=='$') return;// -6; //Skipping homie stuff
                debugSerial<<F("CleanUp retained topic ")<<topic<<endl;
                mqttClient.deleteTopic(topic);
                }
            return;     
           } 
 }
else
{
        pfxlen=inTopic(topic,T_DEV);
        if (!pfxlen) pfxlen = inTopic(topic,T_BCST);  
           else // Personal device topic
               strncpy(savedTopic,topic,sizeof(savedTopic)-1);    
}   
    
    if (!pfxlen) {
        debugSerial<<F("Skipping..")<<endl;
        return;// -3;
    }

    itemName=topic+pfxlen;
    // debugSerial<<itemName<<endl;
    if(!strcmp_P(itemName,CMDTOPIC_P) && payload && (strlen((char*) payload)>1)) {
        mqttClient.deleteTopic(topic);
        cmd_parse((char *)payload);
        return;// -4;
    }
  //if (itemName[0]=='$') return;// -6; //Skipping homie stuff
  if (strrchr(topic,'$')) return;

  Item item(itemName);
  if (item.isValid() && (item.Ctrl((char *)payload)>0) && savedTopic[0] && lanStatus != RETAINING_COLLECTING)       

  //if  (lanStatus != RETAINING_COLLECTING && (mqttClient.isRetained())) 
        {
           debugSerial<<F("Complete. Remove topic ")<<savedTopic<<endl;
           mqttClient.deleteTopic(savedTopic);   
        }
 
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
    const char defaultPassword[] PROGMEM = "password";
    void setupOTA(void)
    {       char passwordBuf[16];
            if (!sysConf.getOTApwd(passwordBuf, sizeof(passwordBuf))) 
                        {
                        strcpy_P(passwordBuf,defaultPassword);
                        errorSerial<<F("DEFAULT password for OTA API. Use otapwd command to set")<<endl;
                        }

            debugSerial<<passwordBuf<<endl;            
            ArduinoOTA.begin(Ethernet.localIP(), "Lighthub", passwordBuf, InternalStorage, sysConfStream);
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

//    udpSyslogClient.begin(SYSLOG_LOCAL_SOCKET);

        udpSyslogArr = aJson.getObjectItem(root, "syslog");
        if (udpSyslogArr && (n = aJson.getArraySize(udpSyslogArr))) {
        char *syslogServer = getStringFromConfig(udpSyslogArr, 0);
         
        if (n>1) syslogPort = aJson.getArrayItem(udpSyslogArr, 1)->valueint;

        _inet_ntoa_r(Ethernet.localIP(),syslogDeviceHostname,sizeof(syslogDeviceHostname));
        infoSerial<<F("Syslog params:")<<syslogServer<<":"<<syslogPort<<":"<<syslogDeviceHostname<<endl;

        udpSyslogClient.begin(SYSLOG_LOCAL_SOCKET);
        udpSyslog.server(syslogServer, syslogPort);
        udpSyslog.deviceHostname(syslogDeviceHostname);

        if (mqttArr) deviceName = getStringFromConfig(mqttArr, 0);
        if (deviceName) udpSyslog.appName(deviceName);
            else udpSyslog.appName(lighthub);
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
    switch (lanStatus) {

        case INITIAL_STATE:

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

                #ifdef MDNS_ENABLE

                char mdnsName[32] = "LightHub";
                SetBytes(sysConf.mac+4,2,mdnsName+8);   
                mdnsName[8+4]='\0'; 

                if (!MDNS.begin(mdnsName)) 
                errorSerial<<F("Error setting up MDNS responder!")<<endl;
                else    infoSerial<<F("mDNS responder started: ")<<mdnsName<<F(".local")<<endl;
                MDNS.addService("http", "tcp", OTA_PORT);

                #endif
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

                #ifdef IPMODBUS
                setupIpmodbus();
                #endif
                initializedListeners = true;
        }
        lanStatus = LIBS_INITIALIZED;
        break;

        case LIBS_INITIALIZED:
            statusLED.set(ledRED|ledGREEN|((configLoaded)?ledBLINK:0));
            if (configLocked) return LIBS_INITIALIZED;      
            if (sysConf.getLoadHTTPConfig())
               {     
                    if (!configOk)
                            {
                            if (loadConfigFromHttp()==200) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
                            else if (configLoaded)   {
                                                        infoSerial<<F("Continue with previously loaded config")<<endl;
                                                        lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
                                                        }
                            
                            else if (Ethernet.localIP()) lanStatus = DO_READ_RE_CONFIG;

                            else lanStatus = DO_REINIT; //Load from NVRAM
                            }
                        else 
                            {
                            infoSerial<<F("Config is valid")<<endl;
                            lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;  
                            }
               } else 
               {
                   if (configLoaded)
                      lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;  
                   else 
                      lanStatus = DO_READ_RE_CONFIG;  

                   infoSerial<<F("Loading config from portal disabled. use get ON to enable")<<endl;    
               }        
            break;

        case IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER:
            wdt_res();
            statusLED.set(ledRED|ledGREEN|((configLoaded)?ledBLINK:0));
            if (!mqttArr || ((aJson.getArraySize(mqttArr)) < 2)) 
                {
                infoSerial<<F("No MQTT configured")<<endl;    
                lanStatus=OPERATION_NO_MQTT;
                } 
            else ip_ready_config_loaded_connecting_to_broker();
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
             // problem here - if no sockets - DHCP will failed. finally (())
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

       case DO_READ_RE_CONFIG: // Pause and re-read EEPROM
             timerLanCheckTime = millis();
             lanStatus = READ_RE_CONFIG;
             //statusLED.set(ledRED|((configLoaded)?ledBLINK:0));
             break;

        case READ_RE_CONFIG: // Restore config from FLASH, re-init LAN
            if (isTimeOver(timerLanCheckTime,millis(),TIMEOUT_REINIT))
                {
                    debugSerial<<F("Restoring config from EEPROM")<<endl; 
                    if (loadConfigFromEEPROM()) lanStatus = IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;//2;
                    else {
                        //timerLanCheckTime = millis();// + 5000;
                        lanStatus = DO_REINIT;//-10;
                    }
                }
            break;

        case DO_NOTHING:
        case OPERATION_NO_MQTT:
        ;
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
              ///      if (mqttClient.connected()) mqttClient.disconnect(); ??
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
  strncat_P(topic, state_P, sizeof(topic)-1);
  strncpy_P(buf, ready_P, sizeof(buf)-1);
  mqttClient.publish(topic,buf,true);

  //strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, name_P, sizeof(topic)-1);
  strncpy_P(buf, nameval_P, sizeof(buf)-1);
  strncat_P(buf,(verval_P),sizeof(buf)-1);
  mqttClient.publish(topic,buf,true);

  //strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, stats_P, sizeof(topic)-1);
  strncpy_P(buf, statsval_P, sizeof(buf)-1);
  mqttClient.publish(topic,buf,true);

  #ifndef NO_HOMIE

//  strncpy_P(topic, outprefix, sizeof(topic));
  setTopic(topic,sizeof(topic),T_DEV);
  strncat_P(topic, homie_P, sizeof(topic)-1);
  strncpy_P(buf, homiever_P, sizeof(buf)-1);
  mqttClient.publish(topic,buf,true);
  configLocked++;
  if (items) {
    char datatype[32]="\0";
    char format [64]="\0";
      aJsonObject * item = items->child;
      
      int nodesLen=0;
      while (items && item) {
          if (item->type == aJson_Array && aJson.getArraySize(item)>0) {
///              strncat(buf,item->name,sizeof(buf));
///              strncat(buf,",",sizeof(buf));
                 nodesLen+=strlen(item->name)+1;
                  switch (  aJson.getArrayItem(item, I_TYPE)->valueint) {
                      case CH_THERMO:
                      strncpy_P(datatype,float_P,sizeof(datatype)-1);
                      format[0]=0;
                      break;

                      case CH_RELAY:
                      case CH_GROUP:
                      strncpy_P(datatype,int_P,sizeof(datatype)-1);
                      strncpy_P(format,intformat_P,sizeof(format)-1);
                      break;

                      case  CH_RGBW:
                      case  CH_RGBWW:
                      case  CH_RGB:
                      strncpy_P(datatype,color_P,sizeof(datatype)-1);
                      strncpy_P(format,hsv_P,sizeof(format)-1);
                      break;
                      case  CH_DIMMER:
                      case  CH_MODBUS:
                      case  CH_PWM:
                      case  CH_VCTEMP:
                      case  CH_VC:
                      strncpy_P(datatype,int_P,sizeof(datatype)-1);
                      strncpy_P(format,intformat_P,sizeof(format)-1);
                      break;
                  } //switch

                  setTopic(topic,sizeof(topic),T_DEV);
                  strncat(topic,item->name,sizeof(topic)-1);
                  strncat(topic,"/cmd/",sizeof(topic)-1);
                  strncat_P(topic,datatype_P,sizeof(topic));
                  mqttClient.publish_P(topic,enum_P,true);

                  setTopic(topic,sizeof(topic),T_DEV);
                  strncat(topic,item->name,sizeof(topic)-1);
                  strncat(topic,"/cmd/",sizeof(topic)-1);
                  strncat_P(topic,format_P,sizeof(topic));
                  mqttClient.publish_P(topic,enumformat_P,true);

                  setTopic(topic,sizeof(topic),T_DEV);
                  strncat(topic,item->name,sizeof(topic)-1);
                  strncat(topic,"/set/",sizeof(topic)-1);
                  strncat_P(topic,datatype_P,sizeof(topic)-1);
                  mqttClient.publish(topic,datatype,true);

                  if (format[0])
                  {
                  setTopic(topic,sizeof(topic),T_DEV);
                  strncat(topic,item->name,sizeof(topic)-1);
                  strncat(topic,"/set/",sizeof(topic));
                  strncat_P(topic,format_P,sizeof(topic)-1);
                  mqttClient.publish(topic,format,true);
                  }
          }  //if
         yield(); 
         item = item->next; 
         }   //while

          //strncpy_P(topic, outprefix, sizeof(topic));
          setTopic(topic,sizeof(topic),T_DEV);
          strncat_P(topic, nodes_P, sizeof(topic)-1);
          //debugSerial<<topic<<"->> "<<nodesLen<<endl;
          
          mqttClient.beginPublish(topic,nodesLen-1,true);
          item = items->child;
          while (items && item)
          {
                if (item->type == aJson_Array && aJson.getArraySize(item)>0) 
                {
                    mqttClient.write((uint8_t*)item->name,strlen(item->name));
                    if (item->next) mqttClient.write(',');
                }
         yield(); 
         item = item->next; 
         }   
          mqttClient.endPublish(); 
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
          errorSerial<<F("At least device name and broker IP must be configured")<<endl;  
          lanStatus = DO_READ_RE_CONFIG;
          return;
        }


    deviceName = getStringFromConfig(mqttArr, 0);
    if (!deviceName) deviceName = (char*) lighthub;

    infoSerial<<F("Device Name:")<<deviceName<<endl;
    
    #if defined  (MDNS_ENABLE) && ! defined (WIFI_ENABLE)             
     if (!mdns.setName(deviceName))
                      errorSerial<<("Error updating MDNS name!")<<endl;
                else    infoSerial<<F("mDNS name updated: ")<<deviceName<<F(".local")<<endl;
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
            debugSerial.println(buf);
            mqttClient.subscribe(buf);

            setTopic(buf,sizeof(buf),T_DEV);
            strncat(buf, "#", sizeof(buf));
            debugSerialPort.println(buf);
            mqttClient.subscribe(buf);

            onMQTTConnect();
            // if (_once) {DMXput(); _once=0;}
            lanStatus = RETAINING_COLLECTING;//4;
            timerLanCheckTime = millis();// + 5000;
            infoSerial<<F("Awaiting for retained topics")<<endl;
        } else
           {
            errorSerial<<F("failed, rc=")<<mqttClient.state()<<F(" try again in 5 seconds")<<endl;
            timerLanCheckTime = millis();// + 5000;
#ifdef RESTART_LAN_ON_MQTT_ERRORS
            mqttErrorRate++;
                        if(mqttErrorRate>50){
                            errorSerial<<F("Too many MQTT connection errors. Restart LAN")<<endl;
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
        infoSerial<<endl;
        lanStatus = HAVE_IP_ADDRESS;
    }
  }//DHCP

        #ifdef MDNS_ENABLE
        char mdnsName[32] = "LightHub";
        SetBytes(sysConf.mac+4,2,mdnsName+8);

        if(!mdns.begin(Ethernet.localIP(), mdnsName))
                        errorSerial<<F("Error setting up MDNS responder!")<<endl;
                else    infoSerial<<F("mDNS responder started.")<<endl;    

        mdns.removeAllServiceRecords();

        char txtRecord[32] = "\x10mac=";
        SetBytes(sysConf.mac,6,txtRecord+5);
       
        strncat(mdnsName,"._http",sizeof(mdnsName));
        if (!mdns.addServiceRecord(mdnsName, OTA_PORT, MDNSServiceTCP, txtRecord))  
                        errorSerial<<("Error setting up service record!")<<endl;
                else    infoSerial<<F("Service record: ")<<mdnsName<<F(".local")<<endl;             
        #endif

#endif //Ethernet
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
    //char *owItem = NULL;

    SetBytes(addr, 8, addrstr);
    addrstr[17] = 0;
    if (!root) return;
    //printFloatValueToStr(currentTemp,valstr);
    debugSerial<<endl<<F("T:")<<currentTemp<<F("<")<<addrstr<<F(">")<<endl;
    aJsonObject *owObj = aJson.getObjectItem(owArr, addrstr);     
    if ((currentTemp != -127.0) && (currentTemp != 85.0) && (currentTemp != 0.0))
        executeCommand(owObj,-1,itemCmd(currentTemp).setSuffix(S_VAL));

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

int cmdFunctionHelp(int arg_cnt, char **args)
{
    printFirmwareVersionAndBuildOptions();
    printCurentLanConfig();
//    printFreeRam();
    infoSerial<<F("\nUse these commands: 'help' - this text\n"
                          "'mac de:ad:be:ef:fe:00' set and store MAC-address in EEPROM\n"
                          "'ip [ip[,dns[,gw[,subnet]]]]' - set static IP\n"
                          "'save' - save current config in NVRAM; ON|OFF - enable/disable autosave\n"
                          "'get' [config addr]' - get config from pre-configured URL and store addr, ON|OFF - enable/disable download on startup\n"
                          "'load' - load config from NVRAM\n"
                          "'pwd' - define and store MQTT password\n"
                          "'otapwd' - define and store HTTP API password\n"
                          "'log [serial_loglevel] [udp_loglevel]' - define log level (0..7)\n"
                          "'kill' - test watchdog\n"
                          "'clear' - clear EEPROM\n"
                          "'reboot' - reboot controller");
return 200;                            
}

void printCurentLanConfig() {
    infoSerial << F("Current LAN config(ip,dns,gw,subnet):");
    printIPAddress(Ethernet.localIP());
    #if not defined(ESP8266) and not defined(ESP32)
    printIPAddress(Ethernet.dnsServerIP());
    #endif
    printIPAddress(Ethernet.gatewayIP());
    printIPAddress(Ethernet.subnetMask());

}

int cmdFunctionKill(int arg_cnt, char **args) {
    for (byte i = 1; i < 20; i++) {
        delay(1000);
        infoSerial<<i;
    };
return 500;    
}

int cmdFunctionReboot(int arg_cnt, char **args) {
    infoSerial<<F("Soft rebooting...");
    softRebootFunc();
return 500;    
}

void applyConfig() {
    if (!root || configLocked) return;
configLocked++;
infoSerial<<F("Applying config")<<endl;
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
        maxChannels = aJson.getArrayItem(dmxoutArr, numParams-1)->valueint;

        #ifdef _artnet
          aJsonObject *artnetArr = aJson.getObjectItem(root, "artnet");
          if (artnetArr)
          {   
             uint8_t artnetMinCh = 1;
             uint8_t artnetMaxCh = maxChannels;  
             short artnetNumParams;

             if (artnetNumParams=aJson.getArraySize(artnetArr)>=2)
             {
                 artnetMinCh = aJson.getArrayItem(artnetArr, 0)->valueint;
                 if (artnetMinCh<1) artnetMinCh = 1; 
                 artnetMaxCh = aJson.getArrayItem(artnetArr, 1)->valueint;
                 if (artnetMaxCh>maxChannels) artnetMaxCh=maxChannels;
             } 
             infoSerial<<F("Artnet start. Channels:")<<artnetMinCh<<F("-")<<artnetMaxCh<<endl;
             artnetSetup();
             artnetSetChans(artnetMinCh,artnetMaxCh);
             //artnetInitialized=true;
          }  
        #endif
        DMXoutSetup(maxChannels);
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
                            it.setFlag(SEND_COMMAND);
                            if (it.itemVal) it.setFlag(SEND_PARAMETERS);
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
if (ethClient.connected()) 
                    {
                    ethClient.stop(); //Refresh MQTT connection
                    lanStatus=IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
                    }
if (lanStatus == OPERATION_NO_MQTT) lanStatus=IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;

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
#ifdef SYSLOG_ENABLE
    infoSerial<<F("\nSyslog ");
    printBool(syslogInitialized);
#endif
#ifdef _artnet
    infoSerial<<F("\nArtnet ");
    printBool(artnet);
#endif

    infoSerial << endl;
    infoSerial<<F("RAM=")<<freeRam()<<endl;
}

int cmdFunctionLoad(int arg_cnt, char **args) {

    if (!loadConfigFromEEPROM()) 
                                    {
                                        lanStatus=DO_REINIT;
                                        return 500;
                                    }
 return 200;                                   
}


int loadConfigFromEEPROM()
{
    if (configLocked) return 0;
    infoSerial<<F("Loading Config from EEPROM")<<endl;
    #if defined(FS_STORAGE)
    sysConfStream.open("/config.json",'r');
    #else
    sysConfStream.open(FN_CONFIG_JSON,'r');
    #endif

    if (sysConfStream.peek() == '{') {
        aJsonStream as = aJsonStream(&sysConfStream);
        cleanConf();
        root = aJson.parse(&as);
        sysConfStream.close();
        if (!root) {
            errorSerial<<F("load failed")<<endl;
            sysConf.setETAG("");
        //    sysConfStream.close();
            return 0;
        }
        infoSerial<<F("Loaded from EEPROM")<<endl;
        applyConfig();
        sysConf.loadETAG();
        //ethClient.stop(); //Refresh MQTT connect to get retained info
        return 1;
    } else if (sysConfStream.available())
          {
           sysConfStream.putEOF(); //truncate garbage        
           sysConf.setETAG("");
           sysConf.saveETAG();
           }
    sysConfStream.close();   
    infoSerial<<F("No stored config")<<endl;
    return 0;
}

int cmdFunctionSave(int arg_cnt, char **args)
{ 
if (arg_cnt>1)
{
    if (!strcasecmp_P(args[1],ON_P)) sysConf.setSaveSuccedConfig(true);
    if (!strcasecmp_P(args[1],OFF_P)) sysConf.setSaveSuccedConfig(false);
    infoSerial<<F("Config autosave:")<<sysConf.getSaveSuccedConfig()<<endl;
    return 200;
}
    #if defined(FS_STORAGE)
    sysConfStream.open("/config.json",'w');
    #else
    sysConfStream.open(FN_CONFIG_JSON,'w');
    #endif

#if defined(__SAM3X8E__) 
  long configBufSize = min(MAX_JSON_CONF_SIZE,freeRam()-1024);
  debugSerial<<"Allocate "<<configBufSize<<" bytes for buffer"<<endl;
  char* outBuf = (char*) malloc(configBufSize); /* XXX: Dynamic size. */
  if (!outBuf)
    {
      sysConfStream.close();  
      errorSerial<<"Can't allocate RAM"<<endl;
      return 500;
    }
  infoSerial<<F("Saving config to EEPROM..")<<endl;
  aJsonStringStream stringStream(NULL, outBuf, configBufSize);
  aJson.print(root, &stringStream);
  int len = strlen(outBuf);
  outBuf[len++]= EOFchar;
  
  size_t res = sysConfStream.write((byte*) outBuf,len);
  free (outBuf);
  infoSerial<<res<< F(" bytes from ")<<len<<F(" are saved to EEPROM")<<endl;
#else
    aJsonStream jsonEEPROMStream = aJsonStream(&sysConfStream);
    infoSerial<<F("Saving config to EEPROM..");
    aJson.print(root, &jsonEEPROMStream);
    //sysConfStream.putEOF();
    //sysConfStream.flush();
    sysConfStream.close();
    infoSerial<<F("Saved to EEPROM")<<endl;
#endif
sysConf.saveETAG();
return 200;
}


int cmdFunctionLoglevel(int arg_cnt, char **args)
{
int res = 400;    
if (arg_cnt>1)
        {
        serialDebugLevel=atoi(args[1]);
        sysConf.setSerialDebuglevel(serialDebugLevel);
        res = 200;
        }

if (arg_cnt>2)
        {
        udpDebugLevel=atoi(args[2]);
        sysConf.setUdpDebuglevel(udpDebugLevel);
        res = 200;
        }
infoSerial<<F("Serial debug level:")<<serialDebugLevel<<F("\nSyslog debug level:")<<udpDebugLevel<<endl;
return res;
}

int cmdFunctionIp(int arg_cnt, char **args)
{
    IPAddress ip0(0, 0, 0, 0);
    IPAddress ip;
/*
    #if defined(ARDUINO_ARCH_AVR) || defined(__SAM3X8E__) || defined(NRF5)
    DNSClient dns;
    #define _inet_aton(cp, addr)   dns._inet_aton(cp, addr)
    #else
    #define _inet_aton(cp, addr)   _inet_aton(cp, addr)
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
    return 200;
}

int cmdFunctionClearEEPROM(int arg_cnt, char **args){
   #ifdef FS_STORAGE
   if (SPIFFS.format()) infoSerial<<F("FS Formatted\n");
   #endif
   if (sysConf.clear()) infoSerial<<F("EEPROM cleared\n");
       #if defined(FS_STORAGE)

    sysConfStream.open("/config.json",'r');
    #else
    sysConfStream.open(FN_CONFIG_JSON,'r');
    #endif
    sysConfStream.putEOF();
    sysConfStream.close();
   return 200;
}

int cmdFunctionPwd(int arg_cnt, char **args)
{ //char empty[]="";
    if (arg_cnt)
         sysConf.setMQTTpwd(args[1]);
    else sysConf.setMQTTpwd();
    infoSerial<<F("MQTT Password updated\n");
    return 200;
}

int cmdFunctionOTAPwd(int arg_cnt, char **args)
{ //char empty[]="";
    if (arg_cnt)
         sysConf.setOTApwd(args[1]);
    else sysConf.setOTApwd();
    infoSerial<<F("OTA Password updated\n");
    return 200;
}

int cmdFunctionSetMac(int arg_cnt, char **args) {
    char dummy;
    uint8_t mac[6];
    if (sscanf(args[1], "%x:%x:%x:%x:%x:%x%c", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5], &dummy) < 6) {
        errorSerial<<F("could not parse: ")<<args[1];
        return 400;
    }
    sysConf.setMAC(mac);
    printMACAddress();
    infoSerial<<F("Updated\n");
    return 200;
}

int cmdFunctionGet(int arg_cnt, char **args) {

if (arg_cnt>1)
{
    if (!strcasecmp_P(args[1],ON_P)) {sysConf.setLoadHTTPConfig(true); return 200;};
    if (!strcasecmp_P(args[1],OFF_P)) {sysConf.setLoadHTTPConfig(false); return 200;};
    
    infoSerial<<F("Loading HTTP config on startup:")<<sysConf.getLoadHTTPConfig()<<endl;

    sysConf.setServer(args[1]);
    infoSerial<<args[1]<<F(" Saved")<<endl;
}

if (lanStatus>=HAVE_IP_ADDRESS)
{


int retCode=loadConfigFromHttp();
    if (retCode==200)
    {
       lanStatus =IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
       return 200;
    }
    else if (retCode == -1)
    {
     debugSerial<<F("Releasing socket and retry")<<endl;   
     configOk=false;
     lanStatus=LIBS_INITIALIZED;
     ethClient.stop(); // Release MQTT socket
     return 201; 
    }
    // Not Loaded
    if (configLoaded) lanStatus =IP_READY_CONFIG_LOADED_CONNECTING_TO_BROKER;
       else lanStatus = DO_READ_RE_CONFIG;
    return retCode; 
}
errorSerial<<F("No IP adress")<<endl;
return 500;
}

void printBool(bool arg) { (arg) ? infoSerial<<F("+") : infoSerial<<F("-"); }

const char * headerKeys[]={"ETag"};

void headerHandlerProc(String header)
{
    debugSerial<<header<<endl;
    //ETag: W/"51e-17bffcd0547"
  if (header.startsWith(F("ETag: ")))
        {
            sysConf.setETAG(header);
        }
} 

int loadConfigFromHttp()
{
    //macAddress * mac = sysConf.getMAC();
    int responseStatusCode = 0;
    char URI[64];
    char configServer[32]="";
if (!sysConf.getServer(configServer,sizeof(configServer)))
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
   
    String ETAG = sysConf.getETAG();

    http_client_parameter get_header[] = {
    {"If-None-Match",NULL},{NULL,NULL}
    };
    get_header[0].value = ETAG.c_str();
    

    debugSerial<<F("free ")<<freeRam()<<endl;delay(100);
    hclient.setHeaderHandler(headerHandlerProc);
    // FILE is the return STREAM type of the HTTPClient
    configStream = hclient.getURI(URI,NULL,get_header);
    responseStatusCode = hclient.getLastReturnCode();
    //debugSerial<<F("http retcode ")<<responseStatusCode<<endl;delay(100);
    //wdt_en();
    
    if (configStream != NULL) {
        if (responseStatusCode == 200) {

            infoSerial<<F("got Config\n"); delay(500);
            aJsonFileStream as = aJsonFileStream(configStream);
            noInterrupts();
            cleanConf();
            root = aJson.parse(&as);
            interrupts();
            hclient.closeStream(configStream);  // this is very important -- be sure to close the STREAM

            if (!root) 
                {
                sysConf.setETAG("");    
                errorSerial<<F("Config parsing failed\n");
                return 0;
                } 
            else {
            applyConfig();
            if (configLoaded && sysConf.getSaveSuccedConfig()) cmdFunctionSave(0,NULL);
            infoSerial<<F("Done.\n");
            return 200; 
            }

        } 
        else if (responseStatusCode == 304) 
        {
            infoSerial<<F("Config not changed\n");
            hclient.closeStream(configStream); 
            return responseStatusCode;
        }
        else 
          {
            errorSerial<<F("ERROR: Server returned ");
            hclient.closeStream(configStream); 
            errorSerial<<responseStatusCode<<endl;
            return responseStatusCode;
           }

    } else {
        debugSerial<<F("failed to connect\n");
            return -1;
           }
#endif
#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32) || defined (NRF5) //|| defined(ARDUINO_ARCH_AVR)//|| defined(ARDUINO_ARCH_ESP32) //|| defined(ARDUINO_ARCH_ESP8266)
    #if defined(WIFI_ENABLE)
    WiFiClient configEthClient;
    #else
    EthernetClient configEthClient;
    #endif
    //String response;
    HttpClient htclient = HttpClient(configEthClient, configServer, 80);
    //htclient.stop(); //_socket =MAX
    htclient.setHttpResponseTimeout(4000);
    wdt_res();
    //debugSerial<<"making GET request");get
    debugSerial<<F("Before request: Free:")<<freeRam()<<endl;
    htclient.beginRequest();
    responseStatusCode = htclient.get(URI);
    htclient.sendHeader("If-None-Match",sysConf.getETAG());
    htclient.endRequest();

    if (responseStatusCode == HTTP_SUCCESS)
    {
    // read the status code and body of the response
    responseStatusCode = htclient.responseStatusCode();
    
    while (htclient.headerAvailable()) 
                    if (htclient.readHeaderName().equalsIgnoreCase("ETAG")) sysConf.setETAG(htclient.readHeaderValue());
   
    //response = htclient.responseBody();
   
    wdt_res();
    infoSerial<<F("HTTP Status code: ")<<responseStatusCode<<endl;

    if (responseStatusCode == 200) {
        aJsonStream socketStream = aJsonStream(&htclient);
        debugSerial<<F("Free:")<<freeRam()<<endl;
        cleanConf();
        debugSerial<<F("Configuration cleaned")<<endl;
        debugSerial<<F("Free:")<<freeRam()<<endl;
        //root = aJson.parse((char *) response.c_str());
        root = aJson.parse(&socketStream);
        htclient.stop();
        if (!root) {
                    errorSerial<<F("Config parsing failed\n");
                    sysConf.setETAG("");
                    return 0;
                    } 
        else {
            debugSerial<<F("Parsed. Free:")<<freeRam()<<endl;
            //debugSerial<<response;
            applyConfig();
            infoSerial<<F("Done.\n");
            if (configLoaded && sysConf.getSaveSuccedConfig()) cmdFunctionSave(0,NULL);
            return 200;
        }
      } 
        else if (responseStatusCode == 304) 
      {
          errorSerial<<F("Config not changed\n");
           htclient.stop();
          return responseStatusCode;
      }
       else  {
          errorSerial<<F("Config retrieving failed\n");
           htclient.stop();
          return responseStatusCode;
      }
    } 
     else    
    {
        errorSerial<<F("Connect failed\n");
        return -1;
    }
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) //|| defined (NRF5)
    #if defined(WIFI_ENABLE)
        WiFiClient configEthClient;
    #else
        EthernetClient configEthClient;
    #endif

    HTTPClient httpClient;

    String fullURI = "http://";
    fullURI+=configServer;
    fullURI+=URI;
    
    #if defined(ARDUINO_ARCH_ESP8266) 
    
    httpClient.begin(configEthClient,fullURI);
    #else
    httpClient.begin(fullURI);
    #endif
    httpClient.addHeader("If-None-Match",sysConf.getETAG());
    httpClient.collectHeaders(headerKeys,1);
    responseStatusCode = httpClient.GET();

    if (responseStatusCode > 0) {
        infoSerial.printf("[HTTP] GET... code: %d\n", responseStatusCode);
        if (responseStatusCode == HTTP_CODE_OK) 
        {
            WiFiClient * stream = httpClient.getStreamPtr();
            if (stream)
            {
            aJsonStream socketStream = aJsonStream(stream);
            sysConf.setETAG(httpClient.header("ETag"));
            //String response = httpClient.getString();
            //debugSerial<<response;
            cleanConf();
            //root = aJson.parse((char *) response.c_str());
            root = aJson.parse(&socketStream);
    
            httpClient.end();
            } else
                    {   
                        httpClient.end();
                        return -1;
                    }
            if (!root) {
                sysConf.setETAG("");
                errorSerial<<F("Config parsing failed\n");
                return 0;
            } else {
                applyConfig();
                if (configLoaded && sysConf.getSaveSuccedConfig()) cmdFunctionSave(0,NULL);
                infoSerial<<F("Done.\n");   
                return 200;             
            }
        } 
        else if (responseStatusCode == HTTP_CODE_NOT_MODIFIED) 
        {
          httpClient.end();
          errorSerial<<F("Config not changed\n");
          return responseStatusCode;
        } 
        else
        {        
            httpClient.end();
            errorSerial<<F("Config retrieving failed\n");
            return responseStatusCode;
        }
    } else 
        {
        errorSerial.printf("[HTTP] GET... failed, error: %s\n", httpClient.errorToString(responseStatusCode).c_str());
        httpClient.end();
        return responseStatusCode;
        }
#endif
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

void TimerHandler(void)
{   
    timerHandlerBusy++;
    interrupts();
    timerCount=micros();
     if (configLoaded && !timerHandlerBusy) 
                    {
                    inputLoop(CHECK_INTERRUPT);
                    #ifdef DMX_SMOOTH
                    DMXOUT_propagate();
                    #endif
                    }
    timerCount=micros()-timerCount;
    timerHandlerBusy--;
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

  #if (SERIAL_BAUD)
  debugSerialPort.begin(SERIAL_BAUD);
  #else 

  #if not defined (__SAM3X8E__)
  debugSerialPort.begin();
  #endif
  delay(1000);
  #endif

  #ifndef ESP_EEPROM_SIZE
  #define ESP_EEPROM_SIZE 4096-10
  #endif

  #if defined (FS_STORAGE)
    #if defined(FS_PREPARE)
            //Initialize File System
            #if defined ARDUINO_ARCH_ESP32
            if(SPIFFS.begin(true))
            #else 
            if(SPIFFS.begin())
            #endif
            {
                debugSerialPort.println("SPIFFS Initialize....ok");
            }
            else
            {
                debugSerialPort.println("SPIFFS Initialization...failed");
            }
    #endif
 #else
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(ESP_EEPROM_SIZE); 
        int streamSize = EEPROM.length();
        sysConfStream.setSize(streamSize-EEPROM_offsetJSON);
        debugSerial<<F("FLASH Init: ")<<streamSize<<endl;
    #endif
 #endif
//debugSerialPort << "Checkin EEPROM integrity (signature)"<<endl;

    if (!sysConf.isValidSysConf()) 
                {
                debugSerialPort.println(F("No valid EEPROM data. Initializing."));    
                sysConf.clear();
                }                   
  //  scan_i2c_bus();

  serialDebugLevel=sysConf.getSerialDebuglevel();
  udpDebugLevel=sysConf.getUdpDebuglevel();

  #if defined(__SAM3X8E__)
  memset(&UniqueID,0,sizeof(UniqueID));
  #endif

  #if defined(M5STACK)
   // Initialize the M5Stack object
   M5.begin();
  #endif

    setupCmdArduino();
    printFirmwareVersionAndBuildOptions();

    
#ifdef SD_CARD_INSERTED
    sd_card_w5100_setup();
#endif
    setupMacAddress();

#ifdef _modbus
        #ifdef CONTROLLINO
        //set PORTJ pin 5,6 direction (RE,DE)
        DDRJ |= B01100000;
        //set RE,DE on LOW
        PORTJ &= B10011111;
        #else
        pinMode(TXEnablePin, OUTPUT);
        #endif
    modbusSerial.begin(MODBUS_SERIAL_BAUD,dimPar);
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

//#ifdef _artnet
//    artnetSetup();
//#endif

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

    #if defined(W5500_CS_PIN) && ! defined(WIFI_ENABLE)
        Ethernet.init(W5500_CS_PIN);
        infoSerial<<F("Use W5500 pin: ");
        infoSerial<<QUOTE(W5500_CS_PIN)<<endl;
    #endif

        loadConfigFromEEPROM();       
}

void printFirmwareVersionAndBuildOptions() {
    infoSerial<<F("\nLazyhome.ru LightHub controller ")<<F(QUOTE(PIO_SRC_REV))<<F(" C++ version:")<<F(QUOTE(__cplusplus))<<endl;

infoSerial<<F("\nConfig server:")<<F(CONFIG_SERVER);

#ifdef CUSTOM_FIRMWARE_MAC
infoSerial<<F("\nFirmware MAC Address " QUOTE(CUSTOM_FIRMWARE_MAC));
#endif

#ifdef CONTROLLINO
    infoSerial<<F("\n(+)CONTROLLINO");
#endif
#ifndef WATCH_DOG_TICKER_DISABLE
    infoSerial<<F("\n(+)WATCHDOG");
#else
    infoSerial<<F("\n(-)WATCHDOG");
#endif
#ifdef DISABLE_FREERAM_PRINT
    infoSerial<<F("\n(-)FreeRam printing");
#else
    infoSerial<<F("\n(+)FreeRam printing");
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
    infoSerial<<F("\n(+)MODBUS " QUOTE(MODBUS_DIMMER_PARAM) " at " QUOTE(modbusSerial) " speed:"  QUOTE(MODBUS_SERIAL_BAUD));
#else
    infoSerial<<F("\n(-)MODBUS");
#endif

#ifndef OWIRE_DISABLE
    infoSerial<<F("\n(+)OWIRE");
        #ifdef USE_1W_PIN
        infoSerial<<F("\n(-)DS2482-100 USE_1W_PIN=")<<QUOTE(USE_1W_PIN);
        #else
        infoSerial<<F("\n(+)DS2482-100");
        #endif
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
    infoSerial<<F("\n(+)AC HAIER on " QUOTE(AC_Serial));
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
infoSerial<<F("\n(+)UARTBRIDGE " QUOTE(MODULE_UATRBRIDGE_UARTA) "<=>" QUOTE(MODULE_UATRBRIDGE_UARTB));
#else
infoSerial<<F("\n(-)UARTBRIDGE");
#endif

#ifdef MDNS_ENABLE
infoSerial<<F("\n(+)MDNS");
#else
infoSerial<<F("\n(-)MDNS");
#endif

#ifndef RELAY_DISABLE
infoSerial<<F("\n(+)PWM_RELAY");
#else
infoSerial<<F("\n(-)PWM_RELAY");
#endif

#ifndef MULTIVENT_DISABLE
infoSerial<<F("\n(+)MULTIVENT");
#else
infoSerial<<F("\n(-)MULTIVENT");
#endif

#ifdef HUMIDIFIER_ENABLE
infoSerial<<F("\n(+)HUMIDIFIER");
#endif

#ifdef ELEVATOR_ENABLE
infoSerial<<F("\n(+)ELEVATOR");
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
    //cmdInit(uint32_t(SERIAL_BAUD));
    cmdInit();
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
    cmdAdd("log",cmdFunctionLoglevel);
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
        if (artnet) artnet->read();  ///hung if network not initialized
#endif
#ifdef MDNS_ENABLE
        #ifndef WIFI_ENABLE
        yield();
        mdns.run();
        #elif defined(ARDUINO_ARCH_ESP8266)
        MDNS.update();
        #endif
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

#ifdef IPMODBUS
if (initializedListeners) ipmodbusLoop();
#endif

}

void owIdle(void) {
#ifdef _artnet
    if (artnet && (lanStatus>=HAVE_IP_ADDRESS)) artnet->read();
#endif

wdt_res();
inputLoop(CHECK_INTERRUPT);
    return; //?????

#ifdef _dmxin
    yield();
    DMXCheck();
#endif

#if defined (_espdmx)
    yield();
    dmxout.update();
#endif

#ifdef IPMODBUS
if (initializedListeners) ipmodbusLoop();
#endif
}
void ethernetIdle(void){
ethernetIdleCount++;
    wdt_res();
    yield();
    inputLoop(CHECK_INTERRUPT);
    yield();
    cmdPoll();
ethernetIdleCount--;
};

void modbusIdle(void) {
    wdt_res();
    statusLED.poll();
    yield();
    cmdPoll();
    yield();
    inputLoop(CHECK_INPUT);

    if (lanLoop() > HAVE_IP_ADDRESS) 
    { // Begin network runners
        yield();
        mqttClient.loop();
#ifdef _artnet
        if (artnet) artnet->read();
#endif
#if defined(OTA)
        yield();
        ArduinoOTA.poll();
#endif
#ifdef MDNS_ENABLE
        #ifndef WIFI_ENABLE
        mdns.run();
        #elif defined(ARDUINO_ARCH_ESP8266)
        MDNS.update();
        #endif
#endif        
    } //End network runners

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

// POLLINT_FAST - as often AS possible every item
// POLLING_1S   - once per second every item
// POLLING_SLOW - just one item every 1S (Note: item::Poll() should return true if some action done - it will postpone next SLOW POLLING)
void pollingLoop(void) {
if (!items) return;   
bool secExpired = isTimeOver(timerPollingCheck,millis(),INTERVAL_SLOW_POLLING);
if (secExpired) timerPollingCheck = millis();

configLocked++;
    aJsonObject * item = items->child;
    while (items && item)
        if (item->type == aJson_Array && aJson.getArraySize(item)>1) {
            Item it(item);
            if (it.isValid()) {
               it.Poll((secExpired)?POLLING_1S:POLLING_FAST);
            } //isValid
            yield();
            item = item->next;
        }  //if
configLocked--;
// SLOW POLLING
    boolean done = false;
    if (lanStatus == RETAINING_COLLECTING) return;
   
    if (secExpired)    
        {
        while (pollingItem && !done) {
            if (pollingItem->type == aJson_Array) {
                Item it(pollingItem);
                uint32_t ret = it.Poll(POLLING_SLOW);
                if (ret)
                {
                 ////// timerPollingCheck = millis();// +  ret;  //INTERVAL_CHECK_MODBUS;
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

enum heaterMode {HEATER_HEAT,HEATER_OFF,HEATER_ERROR};

void thermoRelay(int pin, heaterMode on)
{   
    int thermoPin = abs(pin);
    pinMode(thermoPin, OUTPUT);
    
    if (on == HEATER_ERROR)
            {
            digitalWrite(thermoPin, LOW); 
            debugSerial<<F(" BYPASS")<<endl;
            }

    else if (on == HEATER_HEAT)
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
                        if (isTimeOver(tStore.timestamp16,millisNZ(8) & 0xFFFF,PERIOD_THERMOSTAT_FAILED >> 8,0xFFFF))
                        {
                        errorSerial<<thermoItem->name<<F(" Alarm Expired\n");
                        mqttClient.publish("/alarm/snsr", thermoItem->name);
                        tStore.timestamp16=0; //Stop termostat
                        thermostat.setExt(tStore.asint);
                        thermoRelay(thermoPin,HEATER_ERROR);
                        }
                         else
                         { // Not expired yet
                            if (curTemp > THERMO_OVERHEAT_CELSIUS) mqttClient.publish("/alarm/ovrht", thermoItem->name); 

                            if (!active) thermoRelay(thermoPin,HEATER_OFF);//OFF 
                               else if (curTemp < thermoSetting - THERMO_GIST_CELSIUS) thermoRelay(thermoPin,HEATER_HEAT);//ON
                                    else if (curTemp >= thermoSetting) thermoRelay(thermoPin,HEATER_OFF);//OFF
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

