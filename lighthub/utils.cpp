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

*/

#include "utils.h"
#include "options.h"
#include "stdarg.h"
#include <Wire.h>
#include "main.h"

#include "item.h"
#include <PubSubClient.h>
#include <HardwareSerial.h>

#ifdef CRYPT
#include "SHA256.h"
#endif

#ifndef debugSerialPort
#define debugSerialPort Serial
#endif

extern int8_t configLocked;
extern int8_t ethernetIdleCount;
extern PubSubClient mqttClient;



#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32)
#include <malloc.h>
#endif

#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}
#endif

const char outTopic[] PROGMEM = OUTTOPIC;
const char inTopic[] PROGMEM = INTOPIC;
const char homeTopic[] PROGMEM = HOMETOPIC;
extern char *deviceName;
extern aJsonObject *topics;


void PrintBytes(uint8_t *addr, uint8_t count, bool newline) {
    if (!addr) return;
    for (uint8_t i = 0; i < count; i++) {
        infoSerial<< _HEX(addr[i] >> 4);
        infoSerial<< _HEX(addr[i] & 0x0f);
    }
    if (newline)
        infoSerial<<endl;
}

const char HEXSTR[] = "0123456789ABCDEF";

void SetBytes(uint8_t *addr, uint8_t count, char *out) {
    // debugSerialPort.println("SB:");
    if (!addr || !out) return;
    for (uint8_t i = 0; i < count; i++) {
        *(out++) = HEXSTR[(addr[i] >> 4)];
        *(out++) = HEXSTR[(addr[i] & 0x0f)];
    }
    *out = 0;

}


byte HEX2DEC(char i, bool *err) {
    byte v=0;
    if ('a' <= i && i <= 'f') { v = i - 97 + 10; }
    else if ('A' <= i && i <= 'F') { v = i - 65 + 10; }
    else if ('0' <= i && i <= '9') { v = i - 48; }
    else if (err) *err = true; 
    return v;
}

bool SetAddr(char *in, uint8_t *addr) {
bool err=false;
    if (!addr || !in) return false;
    for (uint8_t i = 0; i < 8; i++) {
        *addr = HEX2DEC(*in++,&err) << 4;
        *addr++ |= HEX2DEC(*in++,&err);
    }
return !err;    
}

// chan is pointer to pointer to string
// Function return first retrived integer and move pointer to position next after ','
long getInt(char **chan) {
    if (chan && *chan && **chan)
    {
    //Skip non-numeric values
    while (**chan && !(**chan == '-' || (**chan >= '0' && **chan<='9'))) *chan += 1;
    long ch = atol(*chan);

    //Move pointer to next element (after ,)
    *chan = strchr(*chan, ',');
    if (*chan) *chan += 1;
    //debugSerialPort.print(F("Par:")); debugSerialPort.println(ch);
    return ch;
   }
   return 0;
}


// chan is pointer to pointer to string
// Function return first retrived number and move pointer to position next after ','
itemCmd getNumber(char **chan) {
    itemCmd val(ST_TENS,CMD_VOID);
    if (chan && *chan && **chan)
    {
    //Skip non-numeric values
    while (**chan && !(**chan == '-' || (**chan >= '0' && **chan<='9'))) *chan += 1;

    long   fractnumbers = 0;
    short  fractlen = 0;
    short  intlen   = 0;
    bool   negative = false;

    char * intptr = * chan;
    if (*intptr == '-') 
                    {
                    negative=true;    
                    intptr ++;
                    }
    while (isDigit(*(intptr+intlen))) intlen++; 

    char * fractptr = strchr(*chan,'.');
    if (fractptr) 
    {
      fractptr ++;
      while (isDigit(*(fractptr+fractlen))) fractlen++; 

      for (short i=0;i<TENS_FRACT_LEN;i++) 
         {
          fractnumbers = fractnumbers * 10;    
          if (isDigit(*(fractptr+i))) fractnumbers += constrain(*(fractptr+i)-'0',0,9);  
         }
    }
    
    if (!fractlen) val.Int((int32_t) atol(*chan));
    else if (fractlen<=TENS_FRACT_LEN && intlen+TENS_FRACT_LEN<=9)
        {
         long intpart = atol(*chan);   
         val.Tens_raw(intpart*TENS_BASE+((negative)?-fractnumbers:fractnumbers));
        }
    else 
      val.Float(atof(*chan)); 
    
    //Move pointer to next element (after ,)
    *chan = strchr(*chan, ',');
    if (*chan) *chan += 1;
   
   }
   //val.debugOut();
   return val;
}

#if defined(ARDUINO_ARCH_ESP32) 
unsigned long freeRam ()
{   
    return esp_get_free_heap_size();//heap_caps_get_free_size();

    }
#endif

#if defined(ESP8266)
unsigned long freeRam ()
{   
    return system_get_free_heap_size();
    }
#endif

#if defined(__AVR__)
unsigned long freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif

#if defined(ARDUINO_ARCH_STM32)
extern char _end;
extern "C" char *sbrk(int i);

unsigned long freeRam() {
    char *heapend = sbrk(0);
    register char *stack_ptr asm( "sp" );
    struct mallinfo mi = mallinfo();

    return stack_ptr - heapend + mi.fordblks;
}

#endif

#if defined(__SAM3X8E__)
extern char _end;
extern "C" char *sbrk(int i);

unsigned long  freeRam()  {
    char *ramstart = (char *) 0x20070000;
    char *ramend = (char *) 0x20088000;
    char *heapend = sbrk(0);
    register char *stack_ptr asm( "sp" );
    struct mallinfo mi = mallinfo();

    return stack_ptr - heapend + mi.fordblks;
}

#endif

#if  defined(NRF5)
extern char _end;
extern "C" char *sbrk(int i);

unsigned long freeRam() {
    char *ramstart = (char *) 0x20070000;
    char *ramend = (char *) 0x20088000;
    char *heapend = sbrk(0);
    register char *stack_ptr asm( "sp" );
    //struct mallinfo mi = mallinfo();

    return stack_ptr - heapend;// + mi.fordblks;
}

#endif


void parseBytes(const char *str, char separator, byte *bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, separator);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}


void printFloatValueToStr(char *valstr, float value) {
    #if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
    sprintf(valstr, "%2.2f", value);
    #endif
    #if defined(__AVR__)
    sprintf(valstr, "%d", (int)value);
    int fractional = 10.0*((float)abs(value)-(float)abs((int)value));
    int val_len =strlen(valstr);
    valstr[val_len]='.';
    valstr[val_len+1]='0'+fractional;
    valstr[val_len+2]='\0';
    #endif
    #if defined(__SAM3X8E__)
    sprintf(valstr, "%2.2f",value);
    #endif
}

#define ARDBUFFER 16 //Buffer for storing intermediate strings. Performance may vary depending on size.

int log(const char *str, ...)//TODO: __FlashStringHelper str support
{
    int i, count=0, j=0, flag=0;
    char temp[ARDBUFFER+1];
    for(i=0; str[i]!='\0';i++)  if(str[i]=='%')  count++; //Evaluate number of arguments required to be printed

    va_list argv;
    va_start(argv, count);
    for(i=0,j=0; str[i]!='\0';i++) //Iterate over formatting string
    {
        if(str[i]=='%')
        {
            //Clear buffer
            temp[j] = '\0';
            debugSerialPort.print(temp);
            j=0;
            temp[0] = '\0';

            //Process argument
            switch(str[++i])
            {
                case 'd': debugSerial.print(va_arg(argv, int));
                    break;
                case 'l': debugSerial.print(va_arg(argv, long));
                    break;
                case 'f': debugSerial.print(va_arg(argv, double));
                    break;
                case 'c': debugSerial.print((char)va_arg(argv, int));
                    break;
                case 's': debugSerial.print(va_arg(argv, char *));
                    break;
                default:  ;
            };
        }
        else
        {
            //Add to buffer
            temp[j] = str[i];
            j = (j+1)%ARDBUFFER;
            if(j==0)  //If buffer is full, empty buffer.
            {
                temp[ARDBUFFER] = '\0';
                debugSerial.print(temp);
                temp[0]='\0';
            }
        }
    };

    debugSerialPort.println(); //Print trailing newline
    return count + 1; //Return number of arguments detected
}

/* Code borrowed from http://forum.arduino.cc/index.php?topic=289190.0
Awesome work Mark T!*/


__attribute__ ((section (".ramfunc")))
// #pragma optimize("", off)
uint32_t ReadUniqueID( uint32_t * pdwUniqueID )
{
    unsigned int status ;

#if defined(__SAM3X8E__)

    /* Send the Start Read unique Identifier command (STUI) by writing the Flash Command Register with the STUI command.*/
    EFC1->EEFC_FCR = (0x5A << 24) | EFC_FCMD_STUI;
    do
    {
        status = EFC1->EEFC_FSR ;
    } while ( (status & EEFC_FSR_FRDY) == EEFC_FSR_FRDY ) ;

    /* The Unique Identifier is located in the first 128 bits of the Flash memory mapping. So, at the address 0x400000-0x400003. */
    pdwUniqueID[0] = *(uint32_t *)IFLASH1_ADDR;
    pdwUniqueID[1] = *(uint32_t *)(IFLASH1_ADDR + 4);
    pdwUniqueID[2] = *(uint32_t *)(IFLASH1_ADDR + 8);
    pdwUniqueID[3] = *(uint32_t *)(IFLASH1_ADDR + 12);

    /* To stop the Unique Identifier mode, the user needs to send the Stop Read unique Identifier
       command (SPUI) by writing the Flash Command Register with the SPUI command. */
    EFC1->EEFC_FCR = (0x5A << 24) | EFC_FCMD_SPUI ;

    /* When the Stop read Unique Unique Identifier command (SPUI) has been performed, the
       FRDY bit in the Flash Programming Status Register (EEFC_FSR) rises. */
    do
    {
        status = EFC1->EEFC_FSR ;
    } while ( (status & EEFC_FSR_FRDY) != EEFC_FSR_FRDY ) ;

   
    return  *(uint32_t *)(IFLASH1_ADDR + 128); // dont remove: SAM defect workaround - MPU dont leave Unique Identifier mode until read flash out UID of range 
    
#else
return 0;    
#endif
}
//#pragma optimize("", on)


int _inet_aton(const char* aIPAddrString, IPAddress& aResult)
{
    // See if we've been given a valid IP address
    const char* p =aIPAddrString;
    while (*p &&
           ( (*p == '.') || (*p >= '0') || (*p <= '9') ))
    {
        p++;
    }

    if (*p == '\0')
    {
        // It's looking promising, we haven't found any invalid characters
        p = aIPAddrString;
        int segment =0;
        int segmentValue =0;
        while (*p && (segment < 4))
        {
            if (*p == '.')
            {
                // We've reached the end of a segment
                if (segmentValue > 255)
                {
                    // You can't have IP address segments that don't fit in a byte
                    return 0;
                }
                else
                {
                    aResult[segment] = (byte)segmentValue;
                    segment++;
                    segmentValue = 0;
                }
            }
            else
            {
                // Next digit
                segmentValue = (segmentValue*10)+(*p - '0');
            }
            p++;
        }
        // We've reached the end of address, but there'll still be the last
        // segment to deal with
        if ((segmentValue > 255) || (segment > 3))
        {
            // You can't have IP address segments that don't fit in a byte,
            // or more than four segments
            return 0;
        }
        else
        {
            aResult[segment] = (byte)segmentValue;
            return 1;
        }
    }
    else

    {
        return 0;
    }
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *_inet_ntoa_r(IPAddress addr, char *buf, int buflen)
{
  short n;
  char intbuf[4];


buf[0]=0;
for(n = 0; n < 4; n++) {
  if (addr[n]>255) addr[n]=-1;
  itoa(addr[n],intbuf,10);
  strncat(buf,intbuf,buflen);
  if (n<3) strncat(buf,".",buflen);
}
  return buf;
}

String toString(const IPAddress& address){
  return String() + address[0] + "." + address[1] + "." + address[2] + "." + address[3];
}

void printIPAddress(IPAddress ipAddress) {
    for (byte i = 0; i < 4; i++)
#ifdef WITH_PRINTEX_LIB
            (i < 3) ? infoSerial << (ipAddress[i]) << F(".") : infoSerial << (ipAddress[i])<<F(", ");
#else
            (i < 3) ? infoSerial << _DEC(ipAddress[i]) << F(".") : infoSerial << _DEC(ipAddress[i]) << F(" ");
#endif
}


char* setTopic(char* buf, int8_t buflen, topicType tt, const char* suffix)
{
  aJsonObject *_root = NULL;
  aJsonObject *_l2 = NULL;

if (topics && topics->type == aJson_Object)
  {
    _root = aJson.getObjectItem(topics, "root");
    switch (tt) {
      case T_OUT:
      _l2 = aJson.getObjectItem(topics, "out");
      break;
      case T_BCST:
      _l2 = aJson.getObjectItem(topics, "bcst");
      break;
    }


    }
if  (_root && _root->type == aJson_String) strncpy(buf,_root->valuestring,buflen);
  else strncpy_P(buf,homeTopic,buflen);
strncat(buf,"/",buflen);

if (_l2 && _l2->type == aJson_String) strncat(buf,_l2->valuestring,buflen);
  else
  switch (tt) {
    case T_DEV:
    strncat(buf,deviceName,buflen);
    break;
    case T_OUT:
    strncat_P(buf,outTopic,buflen);
    break;
    case T_BCST:
    strncat_P(buf,inTopic,buflen); /////
    break;
  }
strncat(buf,"/",buflen);
if (suffix) strncat(buf,suffix,buflen);

return buf;

}



void printUlongValueToStr(char *valstr, unsigned long value) {
    ultoa(value,valstr,10);
    /*
    char buf[11];
    int i=0;
    for(;value>0;i++){
        unsigned long mod = value - ((unsigned long)(value/10))*10;
        buf[i]=mod+48;
        value = (unsigned long)(value/10);
    }

    for(int n=0;n<=i;n++){
        valstr[n]=buf[i-n-1];
    }
    valstr[i]='\0';*/
}


void scan_i2c_bus() {
    byte error, address;
    int nDevices;

     debugSerial<<(F("Scanning...\n"));

     nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

         if (error == 0)
        {
            debugSerial<<(F("\nI2C device found at address "));
        //    if (address<16)
        //        debugSerial<<("0");
            debugSerial<<(address);

             nDevices++;
        }
        else if (error==4)
        {
            debugSerial<<(F("\nUnknow error at address "));
      //      if (address<16)
      //          debugSerial<<("0");
            debugSerial<<(address);
        }
    }
    if (nDevices == 0)
        debugSerial<<(F("No I2C devices found\n"));
    else
        debugSerial<<(F("done\n"));
}


#if defined(__SAM3X8E__)
void softRebootFunc() {
    RSTC->RSTC_CR = 0xA5000005;
}
#endif

#if defined(NRF5) || defined (ARDUINO_ARCH_STM32)
void softRebootFunc() {
    debugSerial<<"Not implemented"<<endl;
}
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void softRebootFunc(){
    debugSerial<<F("ESP.restart();");
    ESP.restart();
}
#endif

#if defined(ARDUINO_ARCH_AVR)
void softRebootFunc(){
void (*RebootFunc)(void) = 0;
RebootFunc();
}
#endif


bool isTimeOver(uint32_t timestamp, uint32_t currTime, uint32_t time, uint32_t modulo)
{
  uint32_t endTime;
  if (!time) return true;

  if (modulo) endTime = (timestamp + time) % modulo;
     else endTime = timestamp + time;   

  return   ((currTime>endTime) && (currTime <timestamp)) ||
              ((timestamp<endTime) && ((currTime>endTime) || (currTime <timestamp)));
}
//millis() - tmr1 >= MY_PERIOD



bool executeCommand(aJsonObject* cmd, int8_t toggle)
{
  //itemCmd _itemCmd;
  return executeCommand(cmd,toggle,itemCmd());
}

bool executeCommand(aJsonObject* cmd, int8_t toggle, itemCmd _itemCmd, aJsonObject* defaultItem, aJsonObject* defaultEmit)
//bool executeCommand(aJsonObject* cmd, int8_t toggle, char* defCmd)
{
//char * legacyString =NULL;
aJsonObject *item = NULL;
aJsonObject *emit = NULL;
aJsonObject *icmd = NULL;
aJsonObject *ecmd = NULL;
char cmdType = 0;

if (cmd) cmdType = cmd->type;
   
switch (cmdType)
{
  case aJson_Array:  //array - recursive iterate
  {
  configLocked++;
  aJsonObject * command = cmd->child;
  while (command)
          {
          executeCommand(command,toggle,_itemCmd,defaultItem,defaultEmit);
          command = command->next;
          }
  configLocked--;
  }
  break;
  
  case aJson_Object: //Modern way
     {
        aJsonObject *act = aJson.getObjectItem(cmd, "act");            
        if (act) return executeCommand(act,toggle,_itemCmd);

        item = aJson.getObjectItem(cmd, "item");
        emit = aJson.getObjectItem(cmd, "emit");
    

        switch (toggle)
            {
            case 0:
            icmd = aJson.getObjectItem(cmd, "icmd");
            ecmd = aJson.getObjectItem(cmd, "ecmd");
            break;
            case 1:
            icmd = aJson.getObjectItem(cmd, "irev");
            ecmd = aJson.getObjectItem(cmd, "erev");
            //no *rev parameters - fallback
            if (!icmd) icmd = aJson.getObjectItem(cmd, "icmd");
            if (!ecmd) ecmd = aJson.getObjectItem(cmd, "ecmd");
        }
    }
   //Continue   
   case aJson_String: //legacy
    if (!icmd) icmd=cmd;
    if (!ecmd) ecmd=cmd;
   //Continue    
   case 0: // NULL command object  
    {
    if (!item) item = defaultItem;
    if (!emit) emit = defaultEmit;

    char * itemCommand = NULL;
    char Buffer[16];
    if(icmd && icmd->type == aJson_String) itemCommand = icmd->valuestring;
    //else    itemCommand = _itemCmd.toString(Buffer,sizeof(Buffer));

    char * emitCommand;
    if(ecmd && ecmd->type == aJson_String) emitCommand = ecmd->valuestring;
    else    emitCommand = _itemCmd.toString(Buffer,sizeof(Buffer));

    //debugSerial << F("IN:") << (pin) << F(" : ") <<endl;
    if (item) {
                if (itemCommand)
                    debugSerial << F("Item: ")<< item->valuestring<< F(" -> ")<<itemCommand<<endl;
                else debugSerial << F("ItemCmd: ")<<item->valuestring<< F(" -> ");_itemCmd.debugOut();
                }
   




    if (emit && emitCommand && emit->type == aJson_String) {
    debugSerial << F("Emit: ")<<emit->valuestring<< F(" -> ")<<emitCommand<<endl;    
    /*
    TODO implement
    #ifdef WITH_DOMOTICZ
        if (getIdxField())
        {  (newValue) ? publishDataToDomoticz(0, emit, "{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"On\"}",
            : publishDataToDomoticz(0,emit,"{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"Off\"}",getIdxField());	                                               getIdxField())
                    : publishDataToDomoticz(0, emit,
                                            "{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"Off\"}",
                                            getIdxField());
                        } else
    #endif
    */


    char addrstr[MQTT_TOPIC_LENGTH];
    strncpy(addrstr,emit->valuestring,sizeof(addrstr));
    if (mqttClient.connected() && !ethernetIdleCount)
    {
    if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
    mqttClient.publish(addrstr, emitCommand , true);
    }

    } // emit

    if (item &&  item->type == aJson_String) {
    //debugSerial <<F("Controlled item:")<< item->valuestring <<endl;
        Item it(item->valuestring);
        if (it.isValid()) 
        {
        int fr = freeRam();                           
        if (fr < minimalMemory) 
            {
            errorSerial<<F("CTRL/exec: OutOfMemory: ")<<fr<<endl;
            return -1;
            }     

        if (itemCommand) it.Ctrl(itemCommand);
            else it.Ctrl(_itemCmd);
        }
        }
    return true;
    }
default:
return false;
} //switch type
return false;
}


itemCmd mapInt(int32_t arg, aJsonObject* map)
{
  itemCmd _itemCmd;
  return _itemCmd.Int(arg);
}

//Same as millis() but never return 0 or -1
unsigned long millisNZ(uint8_t shift)
{
 unsigned long now = millis()>>shift;
 if (!now || !(now+1)) now=1;
 return now;
}

struct serial_st
{
  const char verb[4];
  const serialParamType mode;
};


const serial_st serialModes_P[] PROGMEM =
{
  { "8E1", (serialParamType) SERIAL_8E1},//(uint16_t) US_MR_CHRL_8_BIT | US_MR_NBSTOP_1_BIT | UART_MR_PAR_EVEN },
  { "8N1", (serialParamType) SERIAL_8N1},
  { "8E2", (serialParamType) SERIAL_8E2},
  { "8N2", (serialParamType) SERIAL_8N2},
  { "8O1", (serialParamType) SERIAL_8O1},
  { "8O2", (serialParamType) SERIAL_8O2},
//  { "8M1", SERIAL_8M1},
//  { "8S1", SERIAL_8S1},
  { "7E1", (serialParamType) SERIAL_7E1},//(uint16_t) US_MR_CHRL_8_BIT | US_MR_NBSTOP_1_BIT | UART_MR_PAR_EVEN },
  { "7E2", (serialParamType) SERIAL_7E2},
  { "7O1", (serialParamType) SERIAL_7O1},
  { "7O2", (serialParamType) SERIAL_7O2}
#ifndef ARDUINO_ARCH_STM32
  ,{ "7N1", (serialParamType) SERIAL_7N1}   
  ,{ "7N2", (serialParamType) SERIAL_7N2}
#endif  
//  { "7M1", SERIAL_7M1},
//  { "7S1", SERIAL_7S1}
} ;

#define serialModesNum sizeof(serialModes_P)/sizeof(serial_st)

serialParamType  str2SerialParam(char * str)
{ //debugSerial<<str<<F(" =>");
  for(uint8_t i=0; i<serialModesNum && str;i++)
      if (strcmp_P(str, serialModes_P[i].verb) == 0)
           {

           //debugSerial<< i << F(" ") << pgm_read_word_near(&serialModes_P[i].mode)<< endl;
           if (sizeof(serialModesNum)==4)
             return pgm_read_dword_near(&serialModes_P[i].mode);
           else 
             return pgm_read_word_near(&serialModes_P[i].mode);
         }
  debugSerial<< F("Default serial mode N81 used")<<endl;
  return static_cast<serialParamType> (SERIAL_8N1);
}

bool getPinVal(uint8_t pin)
{
  return (0!=(*portOutputRegister( digitalPinToPort(pin) ) & digitalPinToBitMask(pin)));
}

#ifdef CRYPT
#define HASH_SIZE 32
SHA256 sha256;
extern uint32_t cryptoSalt;
extern char cryptoKey[];

bool checkToken(char * token, char * data)
{
 // Make valid random salted data   
 char saltStr[32];
 printUlongValueToStr(saltStr, cryptoSalt);


 // get hash
 uint8_t result[HASH_SIZE];
 memset(result, 0xAA, sizeof(result));
 
 sha256.reset();
 sha256.update(saltStr,strlen(saltStr));
 sha256.update(cryptoKey,strlen(cryptoKey));
 if (data) sha256.update(data,strlen(data));
 sha256.finalize(result,sizeof(result));
 sha256.clear();
 //hmac<SHA256>(result, HASH_SIZE, cryptoKey, strlen(cryptoKey), testData, strlen(testData));
 
 //for (int i=0;i<HASH_SIZE;i++) {if(result[i]<0x10) debugSerial.print('0'); debugSerial.print(result[i],HEX);}
//debugSerial.println();
for (unsigned int i=0;i<strlen(token)/2;i++)
 {
      uint8_t digit = ((((token[i*2] <= '9') ? token[i*2] - '0' : (token[i*2] & 0x7) + 9) << 4) +
      ((token[i*2+1] <= '9') ? token[i*2+1] - '0' : (token[i*2+1] & 0x7) + 9));
 //debugSerial.print(digit,HEX);     

  if (digit!=result[i])
         {
        debugSerial.println(F("signature Failed"));
        return false;
       }
 }
        debugSerial.println(F("signature Passed"));
        return true;
}

#else
bool checkToken(char * token, char * data)
{return true;}
#endif

  bool isProtectedPin(short pin)
  {
  for (short i=0; i<protectedPinsNum; i++) 
    if (pin==protectedPins[i]) return true;
  return false;    
  }




bool i2cReset(){
debugSerial.println("I2C Reset");

Wire.endTransmission(true);
#if  !defined(ARDUINO_ARCH_ESP8266)
Wire.end();
#endif
pinMode(SCL,OUTPUT);
pinMode(SDA,INPUT);
  //10 сигналов клок
  bool pulse=false;
  for (int i=0; i<20;i++) {
    //i2c_scl_toggle(i2c);
        digitalWrite(SCL,pulse?HIGH:LOW);
        pulse=!pulse;
        delay(10);//10us мкс
  }
Wire.begin();

#ifdef DS2482_100_I2C_TO_1W_BRIDGE
if (oneWire && oneWire->checkPresence())
{        
        oneWire->deviceReset();
#ifndef APU_OFF
        oneWire->setActivePullup();
#endif
        if (oneWire->wireReset())
            debugSerial.println(F("\tReset done"));
        else 
            debugSerial.println(F("\tDS2482 reset error"));   
}
#endif


return true;
}

#pragma message(VAR_NAME_VALUE(debugSerial))
#pragma message(VAR_NAME_VALUE(SERIAL_BAUD))
