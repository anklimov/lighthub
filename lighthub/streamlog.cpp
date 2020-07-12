#include "streamlog.h"
#include <Arduino.h>

#ifdef SYSLOG_ENABLE
Streamlog::Streamlog (HardwareSerial * _serialPort, int _severity , Syslog * _syslog )
{
      serialPort=_serialPort;
      severity=_severity;
      syslog=_syslog;
}
#else
Streamlog::Streamlog (HardwareSerial * _serialPort, int _severity)
{
      serialPort=_serialPort;
      severity=_severity;
}
#endif

void Streamlog::begin(unsigned long speed)
{
  if (serialPort) serialPort->begin(speed);
};

void Streamlog::end()
{
  if (serialPort) serialPort->end();
};

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
  if (ch=='\n')
              {
                logBuffer[logBufferPos]=0;
                if (syslog) syslog->log(severity,(char *)logBuffer);
                logBufferPos=0;
              }
      else
        {
          if (logBufferPos<LOGBUFFER_SIZE-1 && (ch!='\r')) logBuffer[logBufferPos++]=ch;
        }
#endif
  if (serialPort) return serialPort->write(ch);

  return 1;
};
