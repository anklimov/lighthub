// Configuration of drivers enabled
#define _dmxin
#define _dmxout
#define _owire
#define _modbus  
#define _artnet


#if defined(__AVR__)
//All options available
#define modbusSerial Serial2 
#endif

#if defined(__SAM3X8E__)
/// DMX not deployed yet           
#undef _dmxin
//#undef _dmxout
#define modbusSerial Serial2
//#include <DmxDue.h>
#define dmxout DmxDue4 
#define dmxin  DmxDue1
#endif

#if defined(ESP_PLATFORM)
#undef _dmxin
#undef _dmxout
//#undef _modbus
#define modbusSerial Serial
#endif

#ifndef _dmxout
#undef _artnet
#endif
