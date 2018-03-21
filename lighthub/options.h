// Configuration of drivers enabled
#define _dmxin
#define _dmxout
#define _owire
#define _modbus  
#define _artnet

#if defined(ESP8266)
#define __ESP__
#endif

#if defined(__AVR__)
//All options available
#define modbusSerial Serial2
#define dmxin DMXSerial
#define dmxout DmxSimple 
#endif

#if defined(__SAM3X8E__)
#define modbusSerial Serial2
#define dmxout DmxDue1 
#define dmxin  DmxDue1
#endif

#if defined(__ESP__)
#undef _dmxin
#undef _modbus
#define _espdmx
#define modbusSerial Serial1
#endif

#ifndef _dmxout
#undef _artnet
#endif

#define Q(x) #x
#define QUOTE(x) Q(x)