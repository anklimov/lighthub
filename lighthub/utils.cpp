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

#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32F1)
#include <malloc.h>
#endif

#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}
#endif

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


int getInt(char **chan) {
    int ch = atoi(*chan);
    *chan = strchr(*chan, ',');

    if (*chan) *chan += 1;
    //Serial.print(F("Par:")); Serial.println(ch);
    return ch;

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

#if defined(ARDUINO_ARCH_STM32F1)
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
#pragma message(VAR_NAME_VALUE(debugSerial))
#pragma message(VAR_NAME_VALUE(SERIAL_BAUD))