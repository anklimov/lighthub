#include <flashstream.h>

#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
#endif

#ifdef NRF5
NRFFlashStorage EEPROM;
#endif

#ifdef ARDUINO_ARCH_STM32
NRFFlashStorage EEPROM;
#endif