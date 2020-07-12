#include <Print.h>
#include <HardwareSerial.h>
#include <inttypes.h>

#ifndef LOGBUFFER_SIZE
#define LOGBUFFER_SIZE 80
#endif

#ifdef SYSLOG_ENABLE
#include <Syslog.h>
static char logBuffer[LOGBUFFER_SIZE];
static int  logBufferPos=0;
#endif

#define LOG_DEBUG 7
#define LOG_INFO 6
#define LOG_ERROR 3

static uint8_t serialDebugLevel = 7;
static uint8_t udpDebugLevel =7;

class Streamlog : public Print
{
  public:
    #ifdef SYSLOG_ENABLE    
    Streamlog (HardwareSerial * _serialPort, int _severity = LOG_DEBUG, Syslog * _syslog = NULL);
    #else
    Streamlog (HardwareSerial * _serialPort, int _severity = LOG_DEBUG);
    #endif
      //    {serialPort=_serialPort;severity=_severity; syslog=_syslog; }
    void begin(unsigned long speed);
    void end() ;

    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
    size_t write(uint8_t ch);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool() {return true;};
  private:
    uint16_t severity;
    HardwareSerial *serialPort;
    #ifdef SYSLOG_ENABLE
    Syslog * syslog;
    #endif
};
