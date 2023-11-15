#pragma once
#include <Print.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <inttypes.h>

#if defined (STM32)
#include <USBSerial.h>
#endif

#ifndef LOGBUFFER_SIZE
#define LOGBUFFER_SIZE 80
#endif

#ifdef SYSLOG_ENABLE
#include <Syslog.h>
#endif

extern uint8_t serialDebugLevel;
extern uint8_t udpDebugLevel;


#ifndef SerialPortType
#define SerialPortType HardwareSerial
#endif

#define LOG_TRACE 9
#define LOG_DEBUG 7
#define LOG_INFO  6
#define LOG_ERROR 3

#define traceSerial if (serialDebugLevel>=LOG_TRACE || udpDebugLevel>=LOG_TRACE) debugSerial
class Streamlog : public Print
{
  public:
    #ifdef SYSLOG_ENABLE
    Streamlog (SerialPortType * _serialPort, uint8_t _severity = LOG_DEBUG, Syslog * _syslog = NULL, uint8_t _ledPattern = 0);
    #else
    Streamlog (SerialPortType * _serialPort, uint8_t _severity = LOG_DEBUG, uint8_t _ledPattern = 0);
    #endif
    //void begin(unsigned long speed);
    //void end() ;

    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
    size_t write(uint8_t ch);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool() {return true;};
  private:
    uint8_t severity;
    SerialPortType *serialPort;
    uint8_t ledPattern;
    #ifdef SYSLOG_ENABLE
    Syslog * syslog;
    #endif
};
