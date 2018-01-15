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

*/

#include "utils.h"

void PrintBytes(uint8_t* addr, uint8_t count, bool newline) {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(addr[i]>>4, HEX);
    Serial.print(addr[i]&0x0f, HEX);
  }
  if (newline)
    Serial.println();
}

const char HEXSTR[]="0123456789ABCDEF";

void SetBytes(uint8_t* addr, uint8_t count, char * out) {
 // Serial.println("SB:");
  for (uint8_t i = 0; i < count; i++) {
    *(out++)=HEXSTR[(addr[i]>>4)];
    *(out++)=HEXSTR[(addr[i]&0x0f)];
  }
  *out=0;
   
}


byte HEX2DEC(char i)
{ byte v;  
if      ('a' <= i && i <='f') { v=i-97+10; }
        else if ('A' <= i && i <='F') { v=i-65+10; }
        else if ('0' <= i && i <='9') { v=i-48;    }
  return v;
  }

void SetAddr(char * out,  uint8_t* addr) {
 
  for (uint8_t i = 0; i < 8; i++) {
    *addr=HEX2DEC(*out++)<<4;
    *addr++|=HEX2DEC(*out++);
     }
}


int getInt(char ** chan)
{
  int ch = atoi(*chan);
  *chan=strchr(*chan,',');
  
  if (*chan) *chan+=1;
  //Serial.print(F("Par:")); Serial.println(ch);
  return ch;
  
}

