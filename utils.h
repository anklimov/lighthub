#include <Arduino.h>
void PrintBytes(uint8_t* addr, uint8_t count, bool newline=0); 
void SetBytes(uint8_t* addr, uint8_t count, char * out);
void SetAddr(char * out,  uint8_t* addr);
uint8_t HEX2DEC(char i);
