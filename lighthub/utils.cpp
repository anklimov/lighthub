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
#include <wire.h>

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
    for (uint8_t i = 0; i < count; i++) {
        Serial.print(addr[i] >> 4, HEX);
        Serial.print(addr[i] & 0x0f, HEX);
    }
    if (newline)
        Serial.println();
}

const char HEXSTR[] = "0123456789ABCDEF";

void SetBytes(uint8_t *addr, uint8_t count, char *out) {
    // Serial.println("SB:");
    for (uint8_t i = 0; i < count; i++) {
        *(out++) = HEXSTR[(addr[i] >> 4)];
        *(out++) = HEXSTR[(addr[i] & 0x0f)];
    }
    *out = 0;

}


byte HEX2DEC(char i) {
    byte v=0;
    if ('a' <= i && i <= 'f') { v = i - 97 + 10; }
    else if ('A' <= i && i <= 'F') { v = i - 65 + 10; }
    else if ('0' <= i && i <= '9') { v = i - 48; }
    return v;
}

void SetAddr(char *out, uint8_t *addr) {

    for (uint8_t i = 0; i < 8; i++) {
        *addr = HEX2DEC(*out++) << 4;
        *addr++ |= HEX2DEC(*out++);
    }
}

// chan is pointer to pointer to string
// Function return first retrived integer and move pointer to position next after ','
int getInt(char **chan) {
    if (chan && *chan && **chan)
    {
    //Skip non-numeric values
    while (**chan && !(**chan == '-' || (**chan >= '0' && **chan<='9'))) *chan += 1;
    int ch = atoi(*chan);

    //Move pointer to next element (after ,)
    *chan = strchr(*chan, ',');
    if (*chan) *chan += 1;
    //Serial.print(F("Par:")); Serial.println(ch);
    return ch;
   }
   return 0;
}


#if defined(ARDUINO_ARCH_ESP32) || defined(ESP8266)
unsigned long freeRam ()
{return system_get_free_heap_size();}
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

unsigned long freeRam() {
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


void printFloatValueToStr(float value, char *valstr) {
    #if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
    sprintf(valstr, "%2.1f", value);
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
    sprintf(valstr, "%2.1f",value);
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
            Serial.print(temp);
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

    Serial.println(); //Print trailing newline
    return count + 1; //Return number of arguments detected
}

/* Code borrowed from http://forum.arduino.cc/index.php?topic=289190.0
Awesome work Mark T!*/


__attribute__ ((section (".ramfunc")))

void ReadUniqueID( uint32_t * pdwUniqueID )
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
#endif
}


int inet_aton(const char* aIPAddrString, IPAddress& aResult)
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
char *inet_ntoa_r(IPAddress addr, char *buf, int buflen)
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


void printIPAddress(IPAddress ipAddress) {
    for (byte i = 0; i < 4; i++)
#ifdef WITH_PRINTEX_LIB
            (i < 3) ? debugSerial << (ipAddress[i]) << F(".") : debugSerial << (ipAddress[i])<<F(", ");
#else
            (i < 3) ? debugSerial << _DEC(ipAddress[i]) << F(".") : debugSerial << _DEC(ipAddress[i]) << F(", ");
#endif
}


char* setTopic(char* buf, int8_t buflen, topicType tt, char* suffix)
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
if  (_root) strncpy(buf,_root->valuestring,buflen);
  else strncpy_P(buf,homeTopic,buflen);
strncat(buf,"/",buflen);

if (_l2) strncat(buf,_l2->valuestring,buflen);
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
    valstr[i]='\0';
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


#pragma message(VAR_NAME_VALUE(debugSerial))
#pragma message(VAR_NAME_VALUE(SERIAL_BAUD))
