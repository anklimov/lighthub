#pragma once
#ifndef _FLASHSTREAM_H_
#define _FLASHSTREAM_H_

#include <main.h>
#include <WiFiOTA.h>

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
#include "seekablestream.h"

#define EOF 255

#if defined(FS_STORAGE)
class flashStream : public seekableStream
{
private:
String       filename; 
char         openedMode; 
File         fs;
public:
flashStream():seekableStream(65535) 
            {
                openedMode='\0';
                //fs = 0;
                filename = "";
               // open('r');
            };

   virtual int open(String _filename, char mode) override
   {
                  char modestr[2];
                  modestr[0]=mode; modestr[1]=0;
                  filename=_filename;

                  if (fs =  SPIFFS.open(_filename,modestr))
                     {
                        openedMode=mode;
                        //fs.seek(savedPos);

                        if (_filename.endsWith(".json")) {contentType=HTTP_TEXT_JSON;textMode=true;}
                           else if (_filename.endsWith(".bin")) {contentType=HTTP_OCTET_STREAM;textMode=false;}
                                else if (_filename.endsWith(".txt")) {contentType=HTTP_TEXT_PLAIN;textMode=true;}


                        debugSerial<<("Opened/")<<modestr<<(":")<<filename<<endl;
                        return fs;
                     }
                  else 
                     {
                         openedMode='\0';  
                         debugSerial<<("Open error/")<<modestr<<(":")<<filename<<endl; 
                         return 0;
                     }   




   }; 
   virtual int open(char mode='\0') 
            {   
               if (!mode  && openedMode) mode=openedMode;
               if (!mode  && !openedMode) mode='r'; 
               if (openedMode!=mode)
                 { 
                  char modestr[2];
                  modestr[0]=mode;modestr[1]=0;  

                  unsigned int savedPos=fs.position();
                  if (fs) fs.close();

                  if (fs =  SPIFFS.open(filename,modestr))
                     {
                        openedMode=mode;
                        fs.seek(savedPos);
                        debugSerial<<("Opened/")<<modestr<<(":")<<filename<<endl;
                     }
                  else 
                     {
                         openedMode='\0';  
                         debugSerial<<("Open error/")<<modestr<<(":")<<filename<<endl; 
                     }   
            }
       return fs;
    }                  
   virtual int available() override
                { 
                    if (!fs) return 0;
                    if (textMode && peek()==EOF) return 0;  
                    return fs.available();  
                };
                
   virtual int read() {open('r');return fs.read();};
   virtual int peek() {open('r'); return fs.peek();};
   virtual unsigned int seek  (unsigned int _pos = 0) override {return open();fs.seek(_pos,SeekSet);};         
   virtual void close() override {fs.close();  };
   virtual void flush() override {fs.flush();  };
   virtual size_t write(uint8_t ch) {open('w'); return fs.write(ch);};
   using Print::write;
   
virtual ~flashStream () {if (fs) fs.close();} ;
};

#else


class flashStream : public seekableStream 
{
protected:
unsigned int pos;  
unsigned int startPos; 


public:
flashStream(unsigned int _size=4096 ):seekableStream(_size) 
            {
         //   pos = 0; startPos = _startPos; 
            };

    void setSize(unsigned int _size) {streamSize=_size;}; 

    virtual int open(String _filename, char mode='\0') 
                {
                  if (_filename == "config.json")
                    {
                        pos = 0;
                        streamSize = _size;  
                        startPos = EEPROM_offsetJSON;
                        textMode = true;
                        contentType = HTTP_TEXT_JSON;

                        return 1;    
                    }
                  else if (_filename == "config.bin")
                    {
                        pos = 0;
                        startPos = SYSCONF_OFFSET;
                        streamSize = SYSCONF_SIZE;
                        textMode =false;
                        contentType = HTTP_OCTET_STREAM;

                        return 1;
                    }
                  else return 0;    
                };
/*
    virtual int  open(unsigned int _startPos=0, unsigned int _size=4096 char mode='\0') 
            {
                pos = 0;
                startPos = _startPos;
                streamSize = _size;
                return 1;
            };       
*/
    virtual unsigned int seek(unsigned int _pos = 0) 
        {   pos=min(_pos, streamSize);
            debugSerial<<F("Seek:")<<pos<<endl;
            return pos;
        };
    virtual int available() { 
            //debugSerial<<pos<<"%"<<streamSize<<";"; 
            if (textMode && peek()==EOF) return 0; 
            return (pos<streamSize);
            };
    virtual int read() 
            {
                int ch = peek(); 
                pos++; 
                //debugSerial<<"<"<<(char)ch;
                return ch;
                };
    virtual int peek() 
            {    
                if (pos<streamSize) 
                     return EEPROM.read(pos);
                else return -1;    
            };
   virtual void flush() {
        #if defined(ESP8266) || defined(ESP32)
        if (EEPROM.commitReset())
                infoSerial<<"Commited to FLASH"<<endl;
        else    errorSerial<<"Commit error. len:"<<EEPROM.length()<<endl;       
        #endif
   };
          
   virtual size_t write(uint8_t ch) 
            {
                //debugSerial<<">"<<(char)ch;
               #if defined(__AVR__)
                  EEPROM.update(startPos+pos++,(char)ch);
                  return 1;
               #elif  defined(__SAM3X8E__)
                  return EEPROM.write(startPos+pos++,(char)ch);
               #else 
                  EEPROM.write(startPos+pos++,(char)ch);
                  return 1;  
               #endif                
            };
    
   #if defined(__SAM3X8E__)
   virtual size_t write(const uint8_t *buffer, size_t size) override
            {     
                  //debugSerial<<("Write from:")<<pos<<" "<<size<<" bytes"<<endl;    
                  EEPROM.write(startPos+pos,(byte*)buffer,size);
                  pos+=size;
                return size;          
            };
   #else 
   using Print::write;//(const uint8_t *buffer, size_t size);         
   #endif    

    #if defined(ESP8266) || defined(ESP32)      
    virtual void putEOF() override 
            {
                write (255);
                EEPROM.commit();
            };
    #endif        

virtual void close() override 
    {
       flush();
    }    

};
#endif

#endif