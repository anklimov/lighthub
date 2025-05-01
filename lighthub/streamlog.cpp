#include "streamlog.h"
#include <Arduino.h>
#include "statusled.h"
#include "utils.h"

#ifdef SYSLOG_ENABLE
 char logBuffer[LOGBUFFER_SIZE];
 int  logBufferPos=0;
 uint32_t silentTS=0;
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
      ledPattern=_ledPattern;
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
                if (syslog && (!silentTS || isTimeOver(silentTS,millis(),30000UL))) 
                            
                            {
                            uint32_t ts = millis();  
                            syslog->log(severity,(char *)logBuffer);
                            if (millis() - ts > 100UL)
                                {
                                    #if !defined(noSerial)
                                    if (serialPort) serialPort->println(F("Syslog suspended"));
                                    #endif
                                    silentTS = millisNZ();
                                } 
                             else 
                             {
                                   if (silentTS)
                                   {
                                   #if !defined(noSerial)
                                   if (serialPort) serialPort->println(F("Syslog resumed"));
                                   #endif
                                   silentTS = 0;  
                                   syslog->log(severity,F("Syslog resumed"));
                                   } 
                             }
                            }
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
