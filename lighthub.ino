/* Copyright © 2017 Andrey Klimov. All rights reserved.

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
 * 1809 strip out
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

*/

//define NOETHER

#include <Ethernet.h>
#include <PubSubClient2.h>
#include <SPI.h>
#include "utils.h"
#include "owTerm.h"

//#include "led_sysdefs.h"
//#include "pixeltypes.h"
//#include  "hsv2rgb.h"
#include <string.h>
#include <ModbusMaster.h>
#include "aJSON.h"
#include "HTTPClient.h"
#include <Cmd.h>
#include "stdarg.h"
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "dmx.h"
#include "item.h"
#include "inputs.h"
#include <avr/wdt.h>

// Hardcoded definitions
//Thermostate histeresys
#define GIST 2  
#define serverip "192.168.88.2"
IPAddress server(192, 168, 88, 2); //TODO - configure it
//char* inprefix=("/myhome/in/");
//char* outprefix=("/myhome/s_out/");
//char* subprefix=("/myhome/in/#");

#define inprefix "/myhome/in/"
const char outprefix[] PROGMEM  = "/myhome/s_out/";
#define subprefix "/myhome/in/#"
//

aJsonObject *root  =  NULL;
aJsonObject *items =  NULL;
aJsonObject *inputs =  NULL;

aJsonObject *mqttArr =  NULL;
aJsonObject *modbusArr = NULL;
aJsonObject *owArr =  NULL;

unsigned long modbuscheck=0;
unsigned long incheck    =0;
unsigned long lanCheck   =0;
unsigned long thermocheck=0;

aJsonObject * modbusitem= NULL; 


// CommandLine instance.
//CommandLine commandLine(Serial, "> ");


bool owReady = false; 
int  lanStatus = 0;

ModbusMaster node(2,0x60); //TODO dynamic alloc


byte mac[6];

EthernetClient ethClient;
PubSubClient client(ethClient);


int modbusSet(int addr, uint16_t _reg, int _mask, uint16_t value);
int freeRam (void) ;


int itemCtrl2(char* name,int r,int g, int b, int w)
{
  aJsonObject *itemArr= aJson.getObjectItem(items, name);    

       if (itemArr && (itemArr->type==aJson_Array))
       { 
         
        short itemtype = aJson.getArrayItem(itemArr,0)->valueint;
        short itemaddr = aJson.getArrayItem(itemArr,1)->valueint;  
       switch (itemtype){
       case 0: //Dimmed light       
////         if (is_on)  DmxSimple.write(5, 255); //ArduinoDmx2.TxBuffer[itemaddr[ch]]=255;//
////         else DmxSimple.write(itemaddr, map(Par1,0,100,0,255)); //ArduinoDmx2.TxBuffer[itemaddr[ch]]=map(Par1,0,100,0,255);//
       break;
 
       case 1: //Colour RGBW
        DmxSimple.write(itemaddr+3, w);
       case 2: // RGB
      {
   
        DmxSimple.write(itemaddr,   r);
        DmxSimple.write(itemaddr+1, g);
        DmxSimple.write(itemaddr+2, b);
          
         break; }    
                  
        case 7: //Group
        aJsonObject *groupArr= aJson.getArrayItem(itemArr, 1);      
        if (groupArr && (groupArr->type==aJson_Array))
        { aJsonObject *i =groupArr->child;
          while (i)
            { //Serial.println(i->valuestring);
            itemCtrl2(i->valuestring,r,g,b,w);
              i=i->next;}
        }
       } //itemtype
     //  break;
          } //if have correct array 
  }

 
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length]=0;
  Serial.print(F("["));
  Serial.print(topic);
  Serial.print("] ");

  int fr = freeRam();
  if (fr<250) {Serial.println(F("OOM!"));return;}
 
 char subtopic[20]="";
 int cmd = 0;
  
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

 // short intopic  = strncmp(topic,F(inprefix),strlen(inprefix));
 // short outtopic = strncmp(topic,F(outprefix),strlen(outprefix));

 cmd= txt2cmd((char*) payload);
 char * t;
 if (t=strrchr (topic,'/')) strncpy(subtopic,t+1 , 20);
   
   
 
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
  static short _once=1;

#ifdef NOETHER
lanStatus=-11;
#endif

//Serial.println(lanStatus);
switch (lanStatus) 

{
case 0: //Ethernet.begin(mac,ip);
Serial.println(F("Starting lan"));
if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    lanStatus = -10;
    lanCheck=millis()+5000;
  } else
  {
  printIPAddress();
  lanStatus = 1;
  }
break;  
 case 1:
 
   lanStatus=getConfig(0,NULL); //from server
   if (artnet) artnet->begin();
  
   break; 
 case 2: // IP Ready, Connecting & subscribe
//Arming Watchdog
  wdt_enable(WDTO_8S);
  
if (!client.connected() && mqttArr && (aJson.getArraySize(mqttArr)>1)) {
     char *c=aJson.getArrayItem(mqttArr,1)->valuestring;
     Serial.print(F("Attempting MQTT connection..."));
      if (client.connect(c)) {
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
                         
}                  
//static long mtnCnt=0;  
// Maintain dynamic IP
//if (millis()>mtnCnt)
{
//mtnCnt=millis()+2;
wdt_disable();
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
  wdt_enable(WDTO_8S);
  }

return lanStatus;

}


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

  
  
 //client.publish(addrstr, valstr); 
  
  //Serial.print(addrstr);
  //Serial.print(" = ");
  //Serial.println(valstr);

}


void modbusIdle(void) ;

void _handleHelp(int arg_cnt, char **args)
//(char* tokens)
{
  Serial.println(F("Use the commands 'help', 'save', 'get' or 'item'."));
}

void _kill(int arg_cnt, char **args)
{
  for (short i=9;i>0;i--) {delay(1000);Serial.println(i);};
}

#define EEPROM_offset 40

void parseConfig()
{    int mc,incnt;
            //DMX out is configured
            aJsonObject *dmxoutArr = aJson.getObjectItem(root, "dmx");
            if (dmxoutArr && aJson.getArraySize(dmxoutArr)==2)
                {   
                    DmxSimple.usePin(aJson.getArrayItem(dmxoutArr,0)->valueint);
                    DmxSimple.maxChannel(mc=aJson.getArrayItem(dmxoutArr,1)->valueint);
                    Serial.print(F("DMX out started. Channels: "));
                    Serial.println(mc);
                }
            
            //DMX in is configured
            dmxArr=    aJson.getObjectItem(root, "dmxin"); 
            if (dmxArr && (incnt=aJson.getArraySize(dmxArr)))
                {
                 DMXinSetup(incnt*4); 
                 Serial.print(F("DMX in started. Channels:"));
                 Serial.println(incnt*4);
                }
                
            items =    aJson.getObjectItem(root,"items"); 
            modbusitem = items->child;
            inputs =    aJson.getObjectItem(root,"in"); 
            
            modbusArr= aJson.getObjectItem(root, "modbus");
            mqttArr=   aJson.getObjectItem(root, "mqtt"); 
            owArr=     aJson.getObjectItem(root, "ow"); 
          
          
         Serial.println(F("Configured:"));
        
         Serial.print(F("items ")); printBool(items);
         Serial.print(F("inputs ")); printBool(inputs);
         Serial.print(F("modbus ")); printBool(modbusArr);
         Serial.print(F("mqtt ")); printBool(mqttArr);
         Serial.print(F("1-wire ")); printBool(owArr);

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

}

void _loadConfig (int arg_cnt, char **args) {loadConfig(arg_cnt,args);restoreState();}
int loadConfig (int arg_cnt, char **args)
//(char* tokens)
{
          Serial.println(F("loading Config")); 
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
  for (short i=0;i<6;i++) { EEPROM.update(i, mac[i]);}
  Serial.println(F("Updated"));
  
}

void _getConfig(int arg_cnt, char **args) {getConfig(arg_cnt,args);restoreState();}

void printBool (bool arg)
{if (arg) Serial.println(F("on")); else Serial.println(F("off"));}

int getConfig (int arg_cnt, char **args)
//(char *tokens)
{
    FILE* result;
    int returnCode ;
    char ch;
    char URI[32];
    byte hserver[] = { 192,168,88,2 };  


    snprintf(URI, sizeof(URI), "/%02x-%02x-%02x-%02x-%02x-%02x.config.json",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(F("Config URI: "));Serial.println(URI);
    
    HTTPClient hclient(serverip,hserver,80);
   
   // FILE is the return STREAM type of the HTTPClient
    result = hclient.getURI( URI);
    returnCode = hclient.getLastReturnCode();
 
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
  

  
  return 2;
}
 

void setup() {
  //Serial.begin(115200);
   cmdInit(115200);

  Serial.println(F("\nLazyhome.ru LightHub controller v0.91"));
  
  
  for (short i=0;i<6;i++) mac[i]=EEPROM.read(i);
  printMACAddress();
  
    // initialize Modbus communication baud rate
  node.begin(9600,SERIAL_8N1,13);
 
  owReady=0;
    //=owSetup(&Changed);
  
  if (net) net->idle(&owIdle);
  node.idle(&modbusIdle);

  
  client.setServer(server, 1883);
  client.setCallback(callback);
  

  
 ArtnetSetup();


 cmdAdd("help", _handleHelp);
 cmdAdd("save", _saveConfig);
 cmdAdd("load", _loadConfig);
 cmdAdd("get",  _getConfig);
 cmdAdd("set",  _setConfig);
 cmdAdd("kill", _kill);
 cmdAdd("req", _mqttConfigReq);
 
 
}


void loop(){
  wdt_reset();

 //commandLine.update();
  cmdPoll();
if (lanLoop() >1) {client.loop();  if (artnet) artnet->read();}
if (owReady && owArr)      owLoop();

    unsigned long lastpacket = DMXSerial.noDataSince();
   // if (lastpacket && (lastpacket%10==0)) Serial.println(lastpacket);

 DMXCheck();
 if (modbusArr && items) modbusLoop();
 if (items)   thermoLoop();
 if (inputs)  inputLoop();

}

// Idle handlers
void owIdle(void) 
{ if (artnet) artnet->read();
  wdt_reset();

  return;///
  Serial.print("o");

if (lanLoop() == 1) client.loop();
//if (owReady) owLoop();
 DMXCheck();
 //modbusLoop();
 }


void modbusIdle(void) 
{
  //Serial.print("m");
    wdt_reset();

if (lanLoop() > 1) {client.loop();if (artnet) artnet->read();}
//if (owReady) owLoop();
 DMXCheck();
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
#define T_ATTEMPTS 20
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
            if   (!aJson.getArrayItem(extArr, IET_ATTEMPTS)->valueint) {Serial.print(item->name);Serial.println(F(" Expired"));} else aJson.getArrayItem(extArr, IET_ATTEMPTS)->valueint--;
             
            Serial.print(item->name);Serial.print(F(" Set:"));Serial.print(temp); Serial.print(F(" Curtemp:"));Serial.print(curtemp); Serial.print(F( " cmd:")); Serial.print(cmd),   

            pinMode(pin,OUTPUT);
            if (cmd==CMD_OFF || cmd==CMD_HALT) {digitalWrite(pin,LOW);Serial.println(F(" OFF"));}
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
             aJson.getArrayItem(extArray, IET_TEMP)->valueint=t;
             aJson.getArrayItem(extArray, IET_ATTEMPTS)->valueint=(int) T_ATTEMPTS;
            } //if
            
        
  } //if
} // if items

} //proc


int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

