#pragma once
//#ifndef _FLASHSTREAM_H_
//#define _FLASHSTREAM_H_



#include <Stream.h>
#include <Arduino.h>
#include "seekablestream.h"
//#include "config.h"

#if defined(FS_STORAGE)
#include <FS.h>
#if defined ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#endif
#endif

#define FN_CONFIG_JSON 1
#define FN_CONFIG_BIN  2


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
                filename = "";
            };   
   virtual int open(String _filename, char mode) override;
   virtual int available() override;             
   virtual int read() override;  
   virtual int peek() override;  
   virtual unsigned int seek  (unsigned int _pos = 0) override;    
   virtual void close() override;
   virtual void flush() override;
   virtual size_t write(uint8_t ch);              
   using Print::write; 
   virtual void putEOF() override {}; 
   virtual ~flashStream ();
};

#else
#define MAX_STREAM_SIZE 4096
class flashStream : public seekableStream 
{
protected:
unsigned int pos;  
unsigned int startPos; 
char openmode ;

public:
    flashStream():seekableStream(MAX_STREAM_SIZE),pos(0),startPos(0),openmode('\0'){};
    void setSize(unsigned int _size);
    int open(short fileNum, char mode='\0') ; 
    virtual int open(String _filename, char mode='\0') override;
    virtual unsigned int seek(unsigned int _pos = 0); 
    virtual int available() override;    
    virtual int read() ;
    virtual int peek() ; 
    virtual void flush();      
    virtual size_t write(uint8_t ch) ;       
    
    //#if defined(__SAM3X8E__)
    //virtual size_t write(const uint8_t *buffer, size_t size) override;
    //#else 
    using Print::write;//(const uint8_t *buffer, size_t size);         
    //#endif    

    #if defined(ESP8266) || defined(ESP32)      
    virtual void putEOF() override ;          
    #endif        

    virtual void close() override;  
};
#endif

//#endif