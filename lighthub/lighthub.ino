/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.

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

Todo
===
A/C control/Dimmer ?
rotary encoder local ctrl
analog in local ctrl
Smooth regulation/fading
PID Termostat out
dmx relay out
Relay array channel
Relay DMX array channel
Config URL configuration

todo DUE related:
HTTP
PWM freq fix
Config webserver

todo ESP:
Ethernet - to wifi portation
DMX-OUT deploy on USART1
Config webserver

*/
#if defined(__ESP__)
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#endif

// Configuration of drivers enabled
#include "options.h"


#include <PubSubClient.h>
#include <SPI.h>
#include "utils.h"

#include <string.h>
#include <ModbusMaster.h>
#include "aJSON.h"
#include <Cmd.h>
#include "stdarg.h"

#if defined(__AVR__)
#include "HTTPClient.h"
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#define wdt_en()   wdt_enable(WDTO_8S)
#define wdt_dis()  wdt_disable()
#define wdt_res()  wdt_reset()
#else
#include <ArduinoHttpClient.h>
#endif

#if defined(__SAM3X8E__)
#include <DueFlashStorage.h>
          DueFlashStorage EEPROM;
#include <watchdog.h>
#define wdt_res() watchdogReset()
#define wdt_en()
#define wdt_dis()

#else
#include <EEPROM.h>
#endif

#if defined(__ESP__)
#include "esp.h"
#define wdt_res()
#define wdt_en()
#define wdt_dis()


#else

#ifdef Wiz5500
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif

EthernetClient ethClient;
#endif

#ifdef _owire
#include "owTerm.h"
#endif

#if defined(_dmxin) || defined(_dmxout) || defined (_artnet)
#include "dmx.h"
#endif

#include "item.h"
#include "inputs.h"

#ifdef _artnet
#include <Artnet.h>
extern Artnet *artnet;
#endif

// Hardcoded definitions
//Thermostate histeresys
#define GIST 2
//#define serverip "192.168.88.2"
//IPAddress server(192, 168, 88, 2); //TODO - configure it
//char* inprefix=("/myhome/in/");
//char* outprefix=("/myhome/s_out/");
//char* subprefix=("/myhome/in/#");

#define inprefix "/myhome/in/"
const char outprefix[] PROGMEM  = "/myhome/s_out/";
#define subprefix "/myhome/in/#"

aJsonObject *root  =  NULL;
aJsonObject *items =  NULL;
aJsonObject *inputs =  NULL;

aJsonObject *mqttArr =  NULL;
aJsonObject *modbusArr = NULL;
aJsonObject *owArr =  NULL;
aJsonObject *dmxArr = NULL;

unsigned long modbuscheck=0;
unsigned long incheck    =0;
unsigned long lanCheck   =0;
unsigned long thermocheck=0;

aJsonObject * modbusitem= NULL;

bool owReady = false;
int  lanStatus = 0;

#ifdef _modbus
ModbusMaster node;
#endif

byte mac[6];


PubSubClient client(ethClient);


void watchdogSetup(void){}    //Do not remove - strong re-definition WDT Init for DUE

// MQTT Callback routine
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length]=0;
  Serial.print(F("["));
  Serial.print(topic);
  Serial.print("] ");

  int fr = freeRam();
  if (fr<250) {Serial.println(F("OOM!"));return;}

 #define sublen 20
 char subtopic[sublen]="";
 int cmd = 0;

  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

 // short intopic  = strncmp(topic,F(inprefix),strlen(inprefix));
 // short outtopic = strncmp(topic,F(outprefix),strlen(outprefix));

 cmd= txt2cmd((char*) payload);
 char * t;
 if (t=strrchr (topic,'/')) strncpy(subtopic,t+1 , sublen-1);



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

   Item item (subtopic);
   if (item.isValid())
   {
     switch (cmd)
     {
      case 0:
      {
      short i=0;
      int Par[3];

      while (payload && i<3)
          Par[i++]=getInt((char**)&payload);

      item.Ctrl(0,i,Par);
      }
      break;

      case -1: //Not known command
      case -2: //JSON input (not implemented yet
      break;

      case CMD_ON:

       if (item.getEnableCMD(500)) item.Ctrl(cmd); //Accept ON command not earlier then 500 ms after set settings (Homekit hack)
           else Serial.println("on Skipped");

      break;
      default: //some known command
      item.Ctrl(cmd);

     } //ctrl
   } //valid json
  } //no1wire
}

#ifndef __ESP__
void printIPAddress()
{
  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(F("."));
  }
  Serial.println();
}
#endif

void printMACAddress()
{
  Serial.print(F("My mac address: "));
  for (byte thisByte = 0; thisByte < 6; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(mac[thisByte], HEX);
    Serial.print(F(":"));
  }

  Serial.println();
}

void restoreState()
{
 // Once connected, publish an announcement... // Once connected, publish an announcement...
          client.publish("/myhome/out/RestoreState","ON");
};



int getConfig (int arg_cnt, char **args);
int  lanLoop() {

#ifdef NOETHER
lanStatus=-14;
#endif

//Serial.println(lanStatus);
switch (lanStatus)

{
//Initial state
case 0: //Ethernet.begin(mac,ip);

#ifdef __ESP__
//WiFi.mode(WIFI_STA);
//wifiMulti.addAP("Smartbox", "");
if((wifiMulti.run() == WL_CONNECTED)) lanStatus=1;
#else
Serial.println(F("Starting lan"));
wdt_dis();
if (Ethernet.begin(mac,12000) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    lanStatus = -10;
    lanCheck=millis()+60000;
  } else
  {
  printIPAddress();
  lanStatus = 1;
  }
 wdt_en();
 wdt_res();
#endif
break;
//Have IP address
 case 1:

   lanStatus=getConfig(0,NULL); //got config from server or load from NVRAM
   #ifdef _artnet
   if (artnet) artnet->begin();
   #endif

   break;
 case 2: // IP Ready, config loaded, Connecting broker  & subscribe
//Arming Watchdog
 wdt_res();
{
short n=0;
int port=1883;
char empty=0;
char * user = &empty;
char * pass = &empty;

if (!client.connected() && mqttArr && ((n=aJson.getArraySize(mqttArr))>1)) {
     char *c=aJson.getArrayItem(mqttArr,0)->valuestring;
     char *servername=aJson.getArrayItem(mqttArr,1)->valuestring;
     if (n>=3) port=aJson.getArrayItem(mqttArr,2)->valueint;

     if (n>=4) user=aJson.getArrayItem(mqttArr,3)->valuestring;
     if (n>=5) pass=aJson.getArrayItem(mqttArr,4)->valuestring;

     client.setServer(servername,port);

     Serial.print(F("Attempting MQTT connection to "));
     Serial.print(servername);
     Serial.print(F(":"));
     Serial.print(port);
     Serial.print(F(" user:"));
     Serial.print(user);
     Serial.print(F(" ..."));

      if (client.connect(c,user,pass)) {
          Serial.print(F("connected as "));Serial.println(c);


          // ... and resubscribe
          client.subscribe(subprefix);


          restoreState();
          // if (_once) {DMXput(); _once=0;}
          lanStatus=3;
       } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 5 seconds"));
      lanCheck=millis()+5000;
      lanStatus=-12;
      }
}
break;
}
 case 3: //operation
 if (!client.connected()) lanStatus=2;
 break;

//Awaiting address
case  -10: if (millis()>lanCheck)
                    lanStatus=0;
break;
//Reconnect
case  -12: if (millis()>lanCheck)

                    lanStatus=2;
break;
 // read or Re-read config
 case -11:
            if (loadConfig(0,NULL)) lanStatus=2;
              else  {lanCheck=millis()+5000;lanStatus=-10;}
break;

 case -14: ;
 // do notghing with net
}


{
#ifndef __ESP__
wdt_dis();
if (lanStatus>0)
 switch (Ethernet.maintain())
  {
    case 1:
      //renewed fail
      Serial.println(F("Error: renewed fail"));
      lanStatus = -10;
      break;

    case 2:
      //renewed success
      Serial.println(F("Renewed success"));

      //print your local IP address:
      printIPAddress();
      break;

    case 3:
      //rebind fail
      Serial.println(F("Error: rebind fail"));
      lanStatus = -10;
      break;

    case 4:
      //rebind success
      Serial.println(F("Rebind success"));

      //print your local IP address:
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
void Changed (int i, DeviceAddress addr, int val)
{
  char addrstr[32]="NIL";
  char addrbuf[17];
  char valstr[16]="NIL";
  char *owEmit = NULL;
  char *owItem = NULL;

  //PrintBytes(addr,8);
  // Serial.print("Emit: ");
   SetBytes(addr,8,addrbuf);addrbuf[17]=0;

  //Serial.println(addrbuf);

  aJsonObject *owObj = aJson.getObjectItem(owArr, addrbuf);
  if (owObj)
  {
    owEmit = aJson.getObjectItem(owObj, "emit")->valuestring;
    if (owEmit)
              {
                strncpy(addrbuf,owEmit,sizeof(addrbuf));
                Serial.print(owEmit);Serial.print("=");Serial.println(val);
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

  if ((val==-127) || (val==85))
      {
//        Serial.print("Temp err ");Serial.println(t);
       return;
      }

  strcpy_P (addrstr,outprefix);
  strncat (addrstr,addrbuf,sizeof(addrstr));
  //snprintf(addrstr,sizeof(addrstr),"%s%s",F(outprefix),addrbuf);
  sprintf(valstr,"%d",val);
  client.publish(addrstr, valstr);

   if (owItem)
      {
      thermoSetCurTemp(owItem,val);  ///TODO: Refactore using Items interface
      }
}
#endif

void modbusIdle(void) ;

void _handleHelp(int arg_cnt, char **args)
//(char* tokens)
{
  Serial.println(F("Use the commands: 'help' - this text\n'save' - save config in NVRAM\n'get' - get config from pre-configured URL\n'load' - load config from NVRAM\n'kill' - test watchdog"));
}

void _kill(int arg_cnt, char **args)
{
  for (short i=17;i>0;i--) {delay(1000);Serial.println(i);};
}

#define EEPROM_offset 32+6

void parseConfig()
{    int mc,incnt;
            //DMX out is configured
            aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
            #ifdef _dmxout
            if (dmxoutArr && aJson.getArraySize(dmxoutArr)==2)
                {
                   DMXoutSetup(mc=aJson.getArrayItem(dmxoutArr,1)->valueint,aJson.getArrayItem(dmxoutArr,0)->valueint);
                    Serial.print(F("DMX out started. Channels: "));
                    Serial.println(mc);
                }
            #endif
            //DMX in is configured
            #ifdef _dmxin
            dmxArr=    aJson.getObjectItem(root, "dmxin");
            if (dmxArr && (incnt=aJson.getArraySize(dmxArr)))
                {
                 DMXinSetup(incnt*4);
                 Serial.print(F("DMX in started. Channels:"));
                 Serial.println(incnt*4);
                }
            #endif

            items =    aJson.getObjectItem(root,"items");
            modbusitem = items->child;
            inputs =    aJson.getObjectItem(root,"in");

            #ifdef _modbus
            modbusArr= aJson.getObjectItem(root, "modbus");
            #endif

            mqttArr=   aJson.getObjectItem(root, "mqtt");

            #ifdef _owire
            owArr=     aJson.getObjectItem(root, "ow");
            #endif

         Serial.println(F("Configured:"));

         Serial.print(F("items ")); printBool(items);
         Serial.print(F("inputs ")); printBool(inputs);
         Serial.print(F("modbus ")); printBool(modbusArr);
         Serial.print(F("mqtt ")); printBool(mqttArr);
         Serial.print(F("1-wire ")); printBool(owArr);

          #ifdef _owire
          if (owArr && !owReady)
                {
                aJsonObject * item= owArr->child;
                owReady=owSetup(&Changed);

                while (item)
                {
                if ((item->type==aJson_Object) )
                  {
                    DeviceAddress addr;
                    //Serial.print(F("Add:")),Serial.println(item->name);
                    SetAddr(item->name,addr);
                    owAdd(addr);
                    }
                item=item->next;
                  }

                  }
          #endif
}

void _loadConfig (int arg_cnt, char **args) {loadConfig(arg_cnt,args);restoreState();}
int loadConfig (int arg_cnt, char **args)
//(char* tokens)
{         char ch;
          Serial.println(F("loading Config"));

          ch=EEPROM.read(EEPROM_offset);
          if (ch=='{')
          {
          aJsonEEPROMStream as=aJsonEEPROMStream(EEPROM_offset);
          aJson.deleteItem(root);
          root = aJson.parse(&as);
          Serial.println();
          if (!root)
          {
            Serial.println(F("load failed"));
            return 0;
            }
            Serial.println(F("Loaded"));
            parseConfig();
            return 1;
          }
          else
          {
            Serial.println(F("No stored config"));
            return 0;

          }
}

void _mqttConfigReq (int arg_cnt, char **args) {mqttConfigReq(arg_cnt,args);restoreState();}


int mqttConfigReq (int arg_cnt, char **args)
//(char* tokens)
{
 char buf[25] ="/";
 Serial.println(F("request MQTT Config"));
 SetBytes((uint8_t*)mac,6,buf+1);
 buf[13]=0;
 strncat(buf,"/resp/#",25);
 Serial.println(buf);
 client.subscribe(buf);
 buf[13]=0;
 strncat(buf,"/req/conf",25);
 Serial.println(buf);
 client.publish(buf,"1");

 }


int mqttConfigResp (char * as)
{
          Serial.println(F("got MQTT Config"));

          //aJsonEEPROMStream as=aJsonEEPROMStream(EEPROM_offset);

          //aJson.deleteItem(root);
          root = aJson.parse(as);
          Serial.println();
          if (!root)
          {
            Serial.println(F("load failed"));
            return 0;
            }
            Serial.println(F("Loaded"));
            parseConfig();
            return 1;
}

void _saveConfig(int arg_cnt, char **args)
//(char* tokens)
{
  aJsonEEPROMStream es=aJsonEEPROMStream(EEPROM_offset);
  Serial.println(F("Saving config.."));
  aJson.print(root,&es);
  es.putEOF();
  Serial.println(F("Saved"));

}


void _setConfig(int arg_cnt, char **args)
{

  //Serial.print("Got:");
  //Serial.println(args[1]);
  if (sscanf(args[1], "%x:%x:%x:%x:%x:%x%с",
           &mac[0],
           &mac[1],
           &mac[2],
           &mac[3],
           &mac[4],
           &mac[5]) < 6)
{
    Serial.print(F("could not parse: "));
    Serial.println(args[1]);
    return;
}
  printMACAddress();
  for (short i=0;i<6;i++) { EEPROM.write(i, mac[i]);}
  Serial.println(F("Updated"));

}

void _getConfig(int arg_cnt, char **args) {getConfig(arg_cnt,args);restoreState();}

void printBool (bool arg)
{if (arg) Serial.println(F("on")); else Serial.println(F("off"));}


void saveFlash(short n, char* str)
{}

void loadFlash(short n, char* str)
{}

int getConfig (int arg_cnt, char **args)
//(char *tokens)
{


    int returnCode =0;
    char ch;
    char URI   [32];
    char server[32] = "lazyhome.ru";
    if (arg_cnt>0) {
                    strncpy(server,args[1],sizeof(server)-1);
                    saveFlash(0,server);
                    }
    else loadFlash(0,server);

    snprintf(URI, sizeof(URI), "/%02x-%02x-%02x-%02x-%02x-%02x.config.json",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(F("Config URI: "));Serial.print(F("http://"));Serial.print(server);Serial.println(URI);

    #if defined(__AVR__)
    FILE* result;
    //byte hserver[] = { 192,168,88,2 };
    wdt_dis();
    HTTPClient hclient(server,80);


   // FILE is the return STREAM type of the HTTPClient
    result = hclient.getURI( URI);
    returnCode = hclient.getLastReturnCode();
    wdt_en();
    if (result!=NULL) {
      if (returnCode==200) {

        Serial.println(F("got Config"));
        aJsonFileStream as=aJsonFileStream(result);
        aJson.deleteItem(root);
        root = aJson.parse(&as);
        hclient.closeStream(result);  // this is very important -- be sure to close the STREAM

        if (!root)
          {
            Serial.println(F("Config parsing failed"));
            lanCheck=millis()+15000;
           return -11;
            }
            else
          {
            char * outstr=aJson.print(root);
            Serial.println(outstr);
            free (outstr);

            parseConfig();


          }

      }
    else {
      Serial.print(F("ERROR: Server returned "));
      Serial.println(returnCode);
      lanCheck=millis()+5000;
      return -11;
       }

    }
    else {
      Serial.println(F("failed to connect"));
      Serial.println(F(" try again in 5 seconds"));
      lanCheck=millis()+5000;
      return -11;
          }

   #else
  //Non AVR code
  String response;

  HttpClient htclient = HttpClient(ethClient, server, 80);
  htclient.setHttpResponseTimeout(4000);
  wdt_res();
  //Serial.println("making GET request");
  htclient.beginRequest();
  htclient.get(URI);
  htclient.endRequest();


  // read the status code and body of the response
  returnCode = htclient.responseStatusCode();
  response = htclient.responseBody();
  htclient.stop();
  wdt_res();
  Serial.print("HTTP Status code: ");
  Serial.println(returnCode);
  //Serial.print("GET Response: ");

    if (returnCode==200)
    {
    aJson.deleteItem(root);
    root = aJson.parse((char*) response.c_str());

        if (!root)
          {
            Serial.println(F("Config parsing failed"));
           // lanCheck=millis()+15000;
           return -11; //Load from NVRAM
            }
            else
          {
            /*
            char * outstr=aJson.print(root);
            Serial.println(outstr);
            free (outstr);
             */
            Serial.println(response);
            parseConfig();


          }
    }
    else
       {
            Serial.println(F("Config retrieving failed"));
            //lanCheck=millis()+15000;
            return -11; //Load from NVRAM
            }


   #endif
    return 2;
}

#define TXEnablePin 13
void preTransmission()
{
  digitalWrite(TXEnablePin, 1);
}

void postTransmission()
{
  //modbusSerial.flush();
  digitalWrite(TXEnablePin, 0);
}

void setup() {
 cmdInit(115200);

 Serial.println(F("\nLazyhome.ru LightHub controller v0.97"));

 cmdAdd("help", _handleHelp);
 cmdAdd("save", _saveConfig);
 cmdAdd("load", _loadConfig);
 cmdAdd("get",  _getConfig);
 cmdAdd("set",  _setConfig);
 cmdAdd("kill", _kill);
 cmdAdd("req", _mqttConfigReq);


  #ifdef __ESP__
  espSetup();
  #endif

  short macvalid=0;
  byte defmac[6]={0xDE,0xAD,0xBE,0xEF,0xFE,0};

  for (short i=0;i<6;i++)
                            {
                            mac[i]=EEPROM.read(i);
                            if (mac[i]!=0 && mac[i]!=0xff) macvalid=1;
                            }
  if (!macvalid)
              {
                Serial.println(F("Invalid MAC: set default"));
                memcpy(mac,defmac,6);
              }
  printMACAddress();

  loadConfig(0,NULL);

  #ifdef _modbus
  pinMode(TXEnablePin,OUTPUT);
  modbusSerial.begin(9600);

  node.idle(&modbusIdle);
    // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  #endif

  delay(20);

  owReady=0;

  #ifdef _owire
  if (net) net->idle(&owIdle);
  #endif

  //client.setServer(server, 1883);
  client.setCallback(callback);


 #ifdef _artnet
 ArtnetSetup();
 #endif

#if defined(__SAM3X8E__)
//  checkForRemoteSketchUpdate();
#endif


}


void loop(){
  wdt_res();

 //commandLine.update();
  cmdPoll();
if (lanLoop() >1)
  {
  client.loop();
  #ifdef _artnet
  if (artnet) artnet->read();
  #endif
  }

#ifdef _owire
if (owReady && owArr)      owLoop();
#endif

#ifdef _dmxin
//    unsigned long lastpacket = DMXSerial.noDataSince();
     DMXCheck();
#endif
   // if (lastpacket && (lastpacket%10==0)) Serial.println(lastpacket);



 #ifdef _modbus
 if (modbusArr && items) modbusLoop();
 #endif

 #ifdef _owire
 if (items)   thermoLoop();
 #endif

 if (inputs)  inputLoop();

#if defined (_espdmx)
 dmxout.update();
#endif

}

// Idle handlers
void owIdle(void)
{
#ifdef _artnet
  if (artnet) artnet->read();
#endif

  wdt_res();
  return;///
  Serial.print("o");

if (lanLoop() == 1) client.loop();
//if (owReady) owLoop();

#ifdef _dmxin
 DMXCheck();
#endif

#if defined (_espdmx)
 dmxout.update();
#endif

 //modbusLoop();
 }


void modbusIdle(void)
{
  //Serial.print("m");

    wdt_res();
if (lanLoop() > 1)
            {
              client.loop();
              #ifdef _artnet
              if (artnet) artnet->read();
              #endif
             }

//if (owReady) owLoop();
#ifdef _dmxin
 DMXCheck();
#endif

#if defined (_espdmx)
 dmxout.update();
#endif

 //modbusloop();
 }



// Loops



void inputLoop(void)
{

 if (millis()>incheck)
  {

  aJsonObject * input= inputs->child;

 while (input)
                {
                if ((input->type==aJson_Object) )
                  {
                    Input in(input);
                    in.Pool();
                    }
                input=input->next;
                  }


  incheck=millis()+50;

  }

}

void modbusLoop(void)
{
 boolean done=false;
 if (millis()>modbuscheck)
  {
     while (modbusitem && !done)
     {
      if (modbusitem->type==aJson_Array)
          {
            switch (aJson.getArrayItem(modbusitem, 0)->valueint)
            {
            case CH_MODBUS:
            //case CH_VCTEMP:
            case CH_VC:
              {
              Item it(modbusitem);
              it.Pool();
              modbuscheck=millis()+2000;
              done=true;
              break; //case;
              }
            } //switch

         }//if
     modbusitem=modbusitem->next;
     if (!modbusitem) {modbusitem=items->child;return;} //start from 1-st element
     } //while
  }//if
}


// To be refactored

void thermoLoop(void)
{
#define T_ATTEMPTS 200
#define IET_TEMP     0
#define IET_ATTEMPTS 1

 if (millis()>thermocheck)
  {

  aJsonObject * item= items->child;

     while (item)
     {
      if ((item->type==aJson_Array) && (aJson.getArrayItem(item, 0)->valueint==CH_THERMO) && (aJson.getArraySize(item)>4))
          {
            int pin=aJson.getArrayItem(item, I_ARG)->valueint;
            int temp=aJson.getArrayItem(item, I_VAL)->valueint;

            int cmd=aJson.getArrayItem(item, I_CMD)->valueint;

            aJsonObject * extArr=aJson.getArrayItem(item, I_EXT);

            if (extArr && (aJson.getArraySize(extArr)>1) )
            {
            int curtemp =   aJson.getArrayItem(extArr, IET_TEMP)->valueint;
            if   (!aJson.getArrayItem(extArr, IET_ATTEMPTS)->valueint)
                      {
                        Serial.print(item->name);Serial.println(F(" Expired"));

                        }
             else
                    {
                    if (! (--aJson.getArrayItem(extArr, IET_ATTEMPTS)->valueint)) client.publish("/alarm",item->name);

                    }
            Serial.print(item->name);Serial.print(F(" Set:"));Serial.print(temp); Serial.print(F(" Curtemp:"));Serial.print(curtemp); Serial.print(F( " cmd:")); Serial.print(cmd),

            pinMode(pin,OUTPUT);
            if (cmd==CMD_OFF || cmd==CMD_HALT || aJson.getArrayItem(extArr, IET_ATTEMPTS)->valueint==0) {digitalWrite(pin,LOW);Serial.println(F(" OFF"));}
                else
                {
                  if (curtemp+GIST<temp) {digitalWrite(pin,HIGH);Serial.println(F(" ON"));} //too cold
                  else if(temp<=curtemp) {digitalWrite(pin,LOW);Serial.println(F(" OFF")); } //Reached settings
                        else Serial.println(F(" --")); // Nothing to do
                }

            }
          }
      item=item->next;
     }


  thermocheck=millis()+5000;
  Serial.println(freeRam());
  }

}



short thermoSetCurTemp(char * name, short t)
{
if (items)
  {
  aJsonObject *item= aJson.getObjectItem(items, name);
  if (item && (item->type==aJson_Array) && (aJson.getArrayItem(item, I_TYPE)->valueint==CH_THERMO) && (aJson.getArraySize(item)>=4))
      {
        aJsonObject * extArray =NULL;

        if (aJson.getArraySize(item)==4) //No thermo extension yet
            {
            extArray = aJson.createArray(); //Create Ext Array

            aJsonObject * ocurt=aJson.createItem(t);  //Create int
            aJsonObject * oattempts=aJson.createItem(T_ATTEMPTS); //Create int
            aJson.addItemToArray(extArray,ocurt);
            aJson.addItemToArray(extArray,oattempts);
            aJson.addItemToArray(item,extArray); //Adding to item
            } //if
            else if (extArray=aJson.getArrayItem(item,I_EXT))
            {
             aJsonObject * att = aJson.getArrayItem(extArray, IET_ATTEMPTS);
             aJson.getArrayItem(extArray, IET_TEMP)->valueint=t;
              if (att->valueint == 0)  client.publish("/alarmoff",item->name);
             att->valueint =(int) T_ATTEMPTS;
            } //if


  } //if
} // if items

} //proc
