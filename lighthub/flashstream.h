#pragma once
#ifndef _FLASHSTREAM_H_
#define _FLASHSTREAM_H_

#include <main.h>

#if defined(FS_STORAGE)
#include <FS.h>
#include <SPIFFS.h>
#endif

#if defined(ARDUINO_ARCH_AVR) 
#include <EEPROM.h>
#endif

#if defined(ESP32) && !defined(FS_STORAGE)
#include <EEPROM.h>
#endif

#if defined(ARDUINO_ARCH_ESP8266) 
#include <ESP_EEPROM.h>
#endif


#if defined(__SAM3X8E__)
#include <DueFlashStorage.h>
extern DueFlashStorage EEPROM;
#endif

#ifdef NRF5
#include <NRFFlashStorage.h>   //STUB
extern NRFFlashStorage EEPROM;
#endif

#ifdef ARDUINO_ARCH_STM32
#include <NRFFlashStorage.h>   //STUB
extern NRFFlashStorage EEPROM;
#endif


#include <Stream.h>
#include <Arduino.h>

class seekableStream : public Stream 
{
protected:    
unsigned int streamSize;       
public:
seekableStream(unsigned int size):Stream(),streamSize(size) {};
unsigned int getSize() {return streamSize;}
virtual unsigned int seek(unsigned int _pos = 0) = 0;
};

#if defined(FS_STORAGE)
class flashStream : public seekableStream
{
private:
 File fs;
public:
flashStream(String _filename):seekableStream(65535) 
            {
                fs = SPIFFS.open(_filename, "a+");
                if (!fs) SPIFFS.open(_filename, "w+");
            }  
   virtual int available() { return fs.available();  };
   virtual int read() {return fs.read();};
   virtual int peek() { return fs.peek();};
   virtual unsigned int seek(unsigned int _pos = 0){return fs.seek(_pos,SeekSet);};         
   virtual void flush() {return fs.flush();};
   virtual size_t write(uint8_t ch) {return fs.write(ch);};
   using Print::write;
   void putEOF(){write (255);};
virtual ~flashStream () {if (fs) fs.close();} ;
};

#else


class flashStream : public seekableStream 
{
protected:
unsigned int pos;  
unsigned int startPos; 


public:
flashStream(unsigned int _startPos=0, unsigned int _size=4096 ):seekableStream(_size) 
            {
            pos = _startPos; startPos = _startPos; 
             #if defined(ESP8266)
             size_t len = EEPROM.length();
             if (len) EEPROM.commit();
             EEPROM.begin(len+streamSize); //Re-init
             #endif
            };

    virtual unsigned int seek(unsigned int _pos = 0) 
        {   pos=min(startPos+_pos, startPos+streamSize);
            //debugSerial<<F("Seek:")<<pos<<endl;
            return pos;
        };
    virtual int available() { return (pos<streamSize);};
    virtual int read() 
            {
                int ch = peek(); 
                pos++; 
                return ch;
                };
    virtual int peek() 
            {    
                if (pos<streamSize) 
                     return EEPROM.read(pos);
                else return -1;    
            };
   virtual void flush() {
        #if defined(ESP8266)
        EEPROM.commit();
        #endif
   };
          
   virtual size_t write(uint8_t ch) 
            {
                #if defined(__AVR__)
                  EEPROM.update(pos++,(char)ch);
                  return 1;
                #endif   

                #if defined(__SAM3X8E__)
                return EEPROM.write(pos++,(char)ch);
                #endif   
            return 0;                
            };
    
   #if defined(__SAM3X8E__)
   virtual size_t write(const uint8_t *buffer, size_t size) override
            {     
                  //debugSerial<<("Write from:")<<pos<<" "<<size<<" bytes"<<endl;    
                  EEPROM.write(pos,(byte*)buffer,size);
                  pos+=size;
                return size;          
            };
   #else 
   using Print::write;//(const uint8_t *buffer, size_t size);         
   #endif          
    void putEOF(){write (255);
    #if defined(ESP8266)
    EEPROM.commit();
    #endif
    };

};
#endif

#endif