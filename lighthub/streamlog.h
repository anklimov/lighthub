#include <Print.h>
#include <UARTClass.h>
#include <Syslog.h>
#include <inttypes.h>

#define LOG_DEBUG 7
#define LOG_INFO 6
#define LOG_ERROR 3

#ifndef LOGBUFFER_SIZE
#define LOGBUFFER_SIZE 80
#endif

static uint8_t serialDebugLevel = 7;
static uint8_t udpDebugLevel =7;

#ifdef SYSLOG_ENABLE
static char logBuffer[LOGBUFFER_SIZE];
static int  logBufferPos=0;
#endif

class Streamlog : public Print
{
  public:
    Streamlog (UARTClass * _serialPort, int _severity = LOG_DEBUG, Syslog * _syslog = NULL);
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
    UARTClass *serialPort;
    Syslog * syslog;
};
