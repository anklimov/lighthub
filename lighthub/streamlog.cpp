#include "streamlog.h"
#include <Arduino.h>
#include "statusled.h"

#ifdef SYSLOG_ENABLE
 char logBuffer[LOGBUFFER_SIZE];
 int  logBufferPos=0;
#endif

uint8_t serialDebugLevel = 7;
uint8_t udpDebugLevel =7;

#if defined (STATUSLED)
extern StatusLED statusLED;
#endif

#ifdef SYSLOG_ENABLE
extern bool syslogInitialized;
Streamlog::Streamlog (SerialPortType * _serialPort, uint8_t _severity , Syslog * _syslog, uint8_t _ledPattern )
{
      serialPort=_serialPort;
      severity=_severity;
      syslog=_syslog;
      ledPattern=_ledPattern;
}
#else
Streamlog::Streamlog (SerialPortType * _serialPort, uint8_t _severity,  uint8_t _ledPattern)
{
      serialPort=_serialPort;
      severity=_severity;
}
#endif
/*
void Streamlog::begin(unsigned long speed)
{
  if (serialPort) serialPort->begin(speed);
};

void Streamlog::end()
{
  if (serialPort) serialPort->end();
};
*/

int Streamlog::available(void)
{
  if (serialPort) return serialPort->available();
  return 0;
};

int Streamlog::peek(void)
{
  if (serialPort) return serialPort->peek();
  return 0;
};

int Streamlog::read(void)
{
  if (serialPort) return serialPort->read();
  return 0;
};


void Streamlog::flush(void)
{
  if (serialPort) serialPort->flush();

};

size_t Streamlog::write(uint8_t ch)
{
#ifdef SYSLOG_ENABLE
if (syslogInitialized && (udpDebugLevel>=severity))
  {
  if (ch=='\n')
              {
                logBuffer[logBufferPos]=0;
                if (syslog) syslog->log(severity,(char *)logBuffer);
                logBufferPos=0;
              }
      else
        {
          if ((logBufferPos<LOGBUFFER_SIZE-1) && (ch!='\r')) logBuffer[logBufferPos++]=ch;
        }
   }
#endif

  #if defined (STATUSLED)
  if ((ch=='\n') && ledPattern) statusLED.flash(ledPattern);
  #endif
  #if !defined(noSerial)
  if (serialPort && (serialDebugLevel>=severity)) serialPort->write(ch);
  #endif
  return 1;
};
