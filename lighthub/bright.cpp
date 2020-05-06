#include "bright.h"

uint8_t getBright(uint8_t percent)
{
 int index = map(percent,0,100,0,255);
 if (index>255) index=255;
 return getBright255(index);
}

uint8_t getBright255(uint8_t percent)
{
  #ifdef BRIGHT_LINEAR
    return percent;
  #else
  int val = stepvar[index];
  if (val>255) val=255;
  Serial.print(F("Bright:"));
  Serial.print(percent);
  Serial.print(F("->"));
  Serial.println(val);
  return val;
  #endif
}
