#ifndef _TEMPLATE_STREAM_H_
#define _TEMPLATE_STREAM_H_

#include <Stream.h>
#include <aJSON.h>
#include <streamlog.h>
//#define KEYLEN 8
extern aJsonObject * topics;

class templateStream : public Stream
{
public:
    templateStream(char *s, short sfx=0) : str(s), pos(0), val(NULL), valpos(0), bucket(0),suffix(sfx) {buffer[0]='\0'; }

    // Stream methods
    virtual int available() { return str[pos]; }
    virtual int read() {
        if (bucket) 
            
            {   int ch = bucket;
                bucket=0; 
                return ch;
            };

         if (str[pos]=='$')
         {
            if (str[pos+1]=='{')
            {  
                unsigned int i = 0;
                while (str[pos+2+i] && str[pos+2+i]!='}') i++;
                if (i && (str[pos+2+i]=='}'))
                        { 
                         str[pos+2+i]='\0';
                         val=resolveKey(str+pos+2);
                         valpos=0;
                         str[pos+2+i]='}';
                         pos+=3+i;
                        }
            }    
         }

         if (val)
         {  
            char ch = val[valpos];
            if (ch)
                {
                  valpos++;
                  return ch;
                }
            else val = NULL;    

         }

         if (str)
         {
            char ch = str[pos];
            if (ch)
                {
                    pos++;
                    return ch;
                }
            else    
                    {
                    str=NULL;    
                    return 0;
                    }
              
         }
         else return -1;  
  
         }
    virtual int peek() 
    { 
        int bucket = read();
        return bucket;       
        }

    virtual void flush() { };
    // Print methods
    virtual size_t write(uint8_t c) { return 0; };
    virtual char * resolveKey(char *key)
        {
            if (topics && topics->type == aJson_Object)
                {
                 aJsonObject *valObj = aJson.getObjectItem(topics, key);
                 if (valObj->type == aJson_String) return valObj->valuestring;

                }
             if (suffix && (suffix<suffixNum) && !strcmp(key,"sfx"))  
                            {
                            //debugSerial<<F("Template: Suffix=")<<suffix<<endl;    
                            buffer[0]='/';       
                            strncpy_P(buffer+1,suffix_P[suffix],sizeof(buffer)-2);    
                            return buffer;
                            }
              
            return NULL;
        }

private:
    char *str;
    unsigned int pos;
    char *val;
    unsigned int valpos;
    int bucket;
    short suffix;
    char buffer[8];
};

#endif // _TEMPLATE_STREAM_H_