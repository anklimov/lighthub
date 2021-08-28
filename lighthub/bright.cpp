#include "bright.h"
#include <Arduino.h>

uint8_t getBright255(uint8_t percent255)
{
  #ifdef BRIGHT_LINEAR
    return percent255;
  #else
  return pgm_read_byte_near(&stepvar[percent255]);
  /*
  int val = stepvar[index];
  if (val>255) val=255;
  debugSerialPort.print(F("Bright:"));
  debugSerialPort.print(percent);
  debugSerialPort.print(F("->"));
  debugSerialPort.println(val);
  return val;*/
  #endif
}
