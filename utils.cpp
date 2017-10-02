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


