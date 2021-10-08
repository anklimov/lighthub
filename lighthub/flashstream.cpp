#include "flashstream.h"
#include "systemconfigdata.h"

#include <main.h>
#include <WiFiOTA.h>



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


#if defined(__SAM3X8E__)
DueFlashStorage EEPROM;
#endif

#ifdef NRF5
NRFFlashStorage EEPROM;
#endif

#ifdef ARDUINO_ARCH_STM32
NRFFlashStorage EEPROM;
#endif

#if defined(FS_STORAGE)


 int flashStream::open(String _filename, char mode) 
   {
                  char modestr[4];
                  modestr[0]=mode; modestr[1]='b'; modestr[2]='+'; modestr[3]='\0';
                  if (fs) fs.close();
                  filename=_filename;

                  if (fs =  SPIFFS.open(_filename,modestr))
                     {
                        openedMode=mode;
                        streamSize = DEFAULT_FILESIZE_LIMIT;

                        if (_filename.endsWith(".json")) {contentType=HTTP_TEXT_JSON;textMode=true;streamSize = MAX_JSON_CONF_SIZE; }
                           else if (_filename.endsWith(".bin")) {contentType=HTTP_OCTET_STREAM;textMode=false;streamSize = SYSCONF_SIZE;  }
                                else if (_filename.endsWith(".txt")) {contentType=HTTP_TEXT_PLAIN;textMode=true;}
                                     else if (_filename.endsWith(".html")) {contentType=HTTP_TEXT_HTML;textMode=true;} 
                                          else if (_filename.endsWith(".gif")) {contentType=HTTP_IMAGE_GIF;textMode=false;}
                                               else if (_filename.endsWith(".jpg")) {contentType=HTTP_IMAGE_JPEG;textMode=false;}


                        debugSerial<<(F("Opened ("))<<modestr<<(F(")"))<<filename<<endl;
                        return fs;
                     }
                  else 
                     {
                         openedMode='\0';  
                         debugSerial<<("Open error (")<<modestr<<(")")<<filename<<endl; 
                         return 0;
                     }   
   }; 
               
    int flashStream::available() 
                { 
                    if (!fs) return 0;
                    if (textMode && peek()==EOFchar) return 0;  
                    return fs.available();  
                };
                
    int flashStream::read() {
       return fs.read();
       };

    int flashStream::peek() {
       return fs.peek();
       };

    unsigned int flashStream::seek  (unsigned int _pos)  
    {   
        debugSerial<<(F("Seek:"))<<_pos;
        if (_pos>streamSize) _pos=streamSize;
        unsigned int res = fs.seek(_pos,SeekSet);
        debugSerial<<(F(" Res:"))<<res<<endl;
        return res;
      };         
    void flashStream::close()  {fs.close();  };
    void flashStream::flush()  {fs.flush();  };
    size_t flashStream::write(uint8_t ch) 
                {
                    return fs.write(ch);
                    };
   
 flashStream::~flashStream () {if (fs) fs.close();} ;


#else
    void flashStream::setSize(unsigned int _size) {streamSize=_size;}; 
    int  flashStream::open(short fileNum, char mode) 
                {

                 switch (fileNum) {
                   case FN_CONFIG_JSON:     
                        pos = 0;
                        streamSize = MAX_JSON_CONF_SIZE;  
                        startPos = EEPROM_offsetJSON;
                        textMode = true;
                        contentType = HTTP_TEXT_JSON;
                        return 1;    
                    
                  case FN_CONFIG_BIN:                    
                        pos = 0;
                        startPos = SYSCONF_OFFSET;
                        streamSize = SYSCONF_SIZE;
                        textMode =false;
                        contentType = HTTP_OCTET_STREAM;
                        return 1;

                   default:
                   return 0;
            }          
};


     int flashStream::open(String _filename, char mode) 
                {
                  if (_filename == "/config.json") return open (FN_CONFIG_JSON,mode);
                  else if (_filename == "/config.bin")  return open (FN_CONFIG_BIN,mode);
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
     unsigned int flashStream::seek(unsigned int _pos) 
        {   pos=min(_pos, streamSize);
            debugSerial<<F("Seek:")<<pos<<endl;
            return pos;
        };
     int flashStream::available() { 
            //debugSerial<<pos<<"%"<<streamSize<<";"; 
            if (textMode && peek()==EOFchar) return 0; 
            return (pos<streamSize);
            };
     int flashStream::read() 
            {
                int ch = peek(); 
                pos++; 
                //debugSerial<<"<"<<(char)ch;
                return ch;
                };
     int flashStream::peek() 
            {    
                if (pos<streamSize) 
                     return EEPROM.read(startPos+pos);
                else return -1;    
            };
    void flashStream::flush() {
        #if defined(ESP8266) || defined(ESP32)
        if (EEPROM.commitReset())
                infoSerial<<"Commited to FLASH"<<endl;
        else    errorSerial<<"Commit error. len:"<<EEPROM.length()<<endl;       
        #endif
   };
          
    size_t flashStream::write(uint8_t ch) 
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
    size_t flashStream::write(const uint8_t *buffer, size_t size) 
            {     
                  EEPROM.write(startPos+pos,(byte*)buffer,size);
                  pos+=size;
                return size;          
            };
     
   #endif    

    #if defined(ESP8266) || defined(ESP32)      
     void flashStream::putEOF()  
            {
                if (textMode) write (EOFchar);
                EEPROM.commit();
            };
    #endif        

 void flashStream::close()  
    {
       flush();
    }    


#endif