#ifndef _SEEKABLESTREAM_H_
#define _SEEKABLESTREAM_H_

#include <Stream.h>
#include <Arduino.h>

#define EOFchar 255

class seekableStream : public Stream 
{
protected:    
unsigned int streamSize;  
bool         textMode;
uint16_t     contentType;

public:
seekableStream(unsigned int size):Stream(),streamSize(size) {};
virtual bool    checkPermissions(char mode) {return true;};
unsigned int    getSize() {return streamSize;}
void            setSize (unsigned int size) {streamSize = size;};
virtual unsigned int seek(unsigned int _pos = 0) = 0;
virtual int     open(String _filename, char mode) = 0; 
virtual void    close() = 0;
virtual uint16_t getContentType() {return contentType;};
virtual void    putEOF() {if (textMode) write (EOFchar);};
};

#endif