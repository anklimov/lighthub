#include <Arduino.h>
#include "itemCmd.h"
#include "main.h"
#include "Streaming.h"
#include "item.h"

#ifdef ADAFRUIT_LED
#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif
//#include "hsv2rgb.h"

int txt2cmd(char *payload) {
  int cmd = CMD_UNKNOWN;
  if (!payload || !payload[0]) return cmd;

  // Check for command
  if (*payload == '-' || (*payload >= '0' && *payload <= '9')) cmd = CMD_VOID;
  else if (*payload == '%') cmd = CMD_UP;
  else if (*payload == '{') cmd = CMD_JSON;
  else if (*payload == '#') cmd = CMD_RGB;
  else
  {
    for(uint8_t i=1; i<commandsNum ;i++)
        if (strncmp_P(payload, commands_P[i], strlen_P(commands_P[i])) == 0)
             {
//             debugSerial<< i << F(" ") << pgm_read_word_near(&serialModes_P[i].mode)<< endl;
             return i;
           }
  }


  return cmd;
}

/*!
     \brief Constructor with definition of type and command
     \param type - type of value (ST_???, ST_VOID by default) 
     \param code - code of command (CMD_VOID by default)
*/
itemCmd::itemCmd(uint8_t _type, uint8_t _code)
{
  cmd.aslong=0;
  param.aslong=0;

  cmd.itemArgType=_type;
  cmd.cmdCode=_code;
}


/*!
     \brief Constructor with definition of FLOAT value in storage
     \param float
     \param type - type of value (ST_FLOAT or ST_FLOAT_CELSIUS or ST_FLOAT_FARENHEIT) - optional
*/
itemCmd::itemCmd(float val)
{
  cmd.aslong=0;
  param.aslong=0;

  cmd.itemArgType=ST_FLOAT;
  param.asfloat=val;
}

itemCmd itemCmd::setChanType(short chanType)
{
  switch (chanType)
  {
    case CH_RGB:
    case CH_RGBW:
    case CH_SPILED:
    cmd.itemArgType=ST_HSV;
    break;
    case CH_AC:
    case CH_THERMO:
    case CH_VCTEMP:
    cmd.itemArgType=ST_FLOAT_CELSIUS;
    break;
    case CH_DIMMER:
    case CH_MOTOR:
    case CH_PWM:
    case CH_RELAY:
    cmd.itemArgType=ST_PERCENTS;
    break;
    default:
    cmd.itemArgType=ST_PERCENTS;
  }
  return *this;
}

itemCmd itemCmd::setDefault()
{
  switch (cmd.itemArgType){
    case ST_FLOAT_CELSIUS: param.asfloat=20.;
    break;
    case ST_FLOAT_FARENHEIT: param.asfloat=75.;
    break;
    case ST_HSV:
    case ST_HS:
                param.h=100; param.s=0; param.v=100;
    break;
    case ST_HSV255: param.h=100; param.s=0; param.v=255;
    break;
    case ST_PERCENTS: param.v=100;
    break;
    case ST_PERCENTS255: param.v=255;
    break;
    default:
    param.asInt32=0;
  }
  return *this;
}

bool itemCmd::setH(uint16_t h)
{
  int par=h;
  switch (cmd.itemArgType)
  {
    case ST_VOID:
      cmd.itemArgType=ST_HSV;
    case ST_HSV:
    case ST_HSV255:
      if (par>365) par=365;
      if (par<0) par=0;
      param.h=par;
      break;

    default:
  //  debugSerial<<F("Can't assign HUE to type ")<<cmd.itemArgType<<endl;
    return false;
  }
  return true;
}

bool itemCmd::setS(uint8_t s)
{
  int par=s;
  switch (cmd.itemArgType)
  {
    case ST_VOID:
      cmd.itemArgType=ST_HSV;
    case ST_HSV:
     case ST_HSV255:
      if (par>100) par=100;
      param.s=par;
      break;
      /*
    case ST_HSV255:
      if (par>255) par=255;
      if (par<0) par=0;
      param.s=par;
      break;*/
    default:
//    debugSerial<<F("Can't assign saturation to type ")<<cmd.itemArgType<<endl;
    return false;
  }
  return true;
}



//! Setup color tempetature parameter for HSV or HSV255 types. It must be 0..100 value.
//! 0 - cold, 100 - warm light
bool itemCmd::setColorTemp(uint8_t t)
{
  int par=t;
  switch (cmd.itemArgType)
  {
    case ST_VOID:
      cmd.itemArgType=ST_HSV;
    case ST_HSV:
    case ST_HS:
      if (par>100) par=100;
      // value 0 is reserved for default/uninitialized value. Internally data stored in 1..101 range (7 bits)
      param.colorTemp=par+1;
      break;
    case ST_HSV255:
      if (par>100) par=100;
      if (par<0) par=0;
      param.colorTemp=par+1;
      break;
    default:
    return false;
  }
  return true;
}

//! Setup color tempetature parameter from HSV or HSV255 types. return 0..100 value in success.
//! -1 - if no value stored
int8_t itemCmd::getColorTemp()
{
  if (cmd.itemArgType!=ST_HSV and cmd.itemArgType!=ST_HSV255 and cmd.itemArgType!=ST_HS) return -1;
  return param.colorTemp-1;
}

uint16_t itemCmd::getH()
{
  return param.h;
}

uint16_t itemCmd::getS()
{
  return param.s;
}

bool itemCmd::incrementPercents(int16_t dif)
{ int par=param.v;
  switch (cmd.itemArgType)
  {
    case ST_PERCENTS:
    case ST_HSV:
     par+=dif;
     if (par>100) par=100;
     if (par<0) par=0;
    break;
    case ST_PERCENTS255:
    case ST_HSV255:
     par+=dif;
     if (par>255) par=255;
     if (par<0) par=0;
    break;
   default: return false;
  }
  param.v=par;
  return true;
}

bool itemCmd::incrementH(int16_t dif)
{ int par=param.h;
  switch (cmd.itemArgType)
  {
  case ST_HSV:
  case ST_HSV255:
   par+=dif;
   if (par>365) par=0;
   if (par<0) par=365;
   break;
  default: return false;
  break;
}
param.h=par;
return true;
}

bool itemCmd::incrementS(int16_t dif)
{int par=param.s;
  switch (cmd.itemArgType)
  {
    //case ST_PERCENTS:
    case ST_HSV:
    case ST_HSV255:
     par+=dif;
     if (par>100) par=100;
     if (par<0) par=0;
    break;
    /*
    //case ST_PERCENTS255:
    case ST_HSV255:
     par+=dif;
     if (par>255) par=255;
     if (par<0) par=0;
    break; */
   default: return false;
  }
  param.s=par;
  return true;

}


itemCmd itemCmd::assignFrom(itemCmd from)
{
  bool RGBW_flag   = false;
  bool HSV255_flag = false;
  cmd.suffixCode=from.cmd.suffixCode;

  switch (cmd.itemArgType){ //Destination
     case ST_HSV:
     case ST_PERCENTS:
        switch (from.cmd.itemArgType)
             {
              case ST_RGBW:
                   param.w=from.param.w;
              case ST_RGB:
                   param.r=from.param.r;
                   param.g=from.param.g;
                   param.b=from.param.b;
                   cmd.itemArgType=from.cmd.itemArgType; //Changing if type
                   break;
              case ST_HSV:
                   param.v=from.param.v;
              case ST_HS:
                   param.h=from.param.h;
                   param.s=from.param.s;
                   break;
              case ST_PERCENTS:
                   param.v=from.param.v;
                   break;
              case ST_HSV255:
                   param.h=from.param.h;
                   //param.s=map(from.param.s,0,255,0,100);
                   param.s=from.param.s;
                   param.v=map(from.param.v,0,255,0,100);
                   break;
              case ST_PERCENTS255:
                   param.v=map(from.param.v,0,255,0,100);
                   break;
              case ST_FLOAT:
                   param.v =  (int) from.param.asfloat;
                    break;
              default:
                  debugSerial<<F("Wrong Assignment ")<<from.cmd.itemArgType<<F("->")<<cmd.itemArgType<<endl;
              }
              break;
        case ST_HSV255:
        case ST_PERCENTS255:
           switch (from.cmd.itemArgType)
             {
               case ST_RGBW:
                    param.w=from.param.w;
               case ST_RGB:
                    param.r=from.param.r;
                    param.g=from.param.g;
                    param.b=from.param.b;
                    cmd.itemArgType=from.cmd.itemArgType;
                    break;
              case ST_HS:
                   param.v=map(from.param.v,0,100,0,255);
              case ST_HSV:
                   param.h=from.param.h;
                   param.s=from.param.s;
                   //param.s=map(from.param.s,0,100,0,255);
                   break;
              case ST_PERCENTS:
                   param.v=map(from.param.v,0,100,0,255);
                   break;
              case ST_HSV255:
                   param.h=from.param.h;
                   param.s=from.param.s;
                   param.v=from.param.v;
                   break;
              case ST_PERCENTS255:
                   param.v=from.param.v;
                   break;
              default:
                       debugSerial<<F("Wrong Assignment ")<<from.cmd.itemArgType<<F("->")<<cmd.itemArgType<<endl;
              }
             break;
        case ST_VOID:
             cmd.itemArgType=from.cmd.itemArgType;

        case ST_INT32:
        case ST_UINT32:
        case ST_FLOAT:
        case ST_FLOAT_CELSIUS:
        case ST_FLOAT_FARENHEIT:
           param.asInt32=from.param.asInt32;
           break;

        case ST_RGBW:
             RGBW_flag=true;
        case ST_RGB:
        switch (from.cmd.itemArgType)
          {
            case ST_RGBW:
            case ST_RGB:
                  param.asInt32=from.param.asInt32;
            break;
            // Those types are not possible to apply over RGB without convertion toward HSV
            case ST_PERCENTS255:
            case ST_PERCENTS:
            case ST_HS:
            {

             #ifndef ADAFRUIT_LED  
             // Restoring HSV from RGB
              CRGB rgb;
              rgb.r = param.r;
              rgb.g = param.g;
              rgb.b = param.b;    
              CHSV hsv = rgb2hsv_approximate(rgb);
              #endif
 
                 switch (from.cmd.itemArgType){
                      case ST_PERCENTS255:
                            #ifndef ADAFRUIT_LED 
                                from.param.h = map(hsv.h, 0, 255, 0, 365);
                                from.param.s = map(hsv.s, 0, 255, 0, 100);
                            #else
                                from.param.h=100;
                                from.param.s=0;    
                            #endif    
                      from.cmd.itemArgType=ST_HSV255;  
                      break;
                      case ST_PERCENTS:
                            #ifndef ADAFRUIT_LED  
                                from.param.h = map(hsv.h, 0, 255, 0, 365);
                                from.param.s = map(hsv.s, 0, 255, 0, 100);
                            #else
                                from.param.h=100;
                                from.param.s=0;   
                            #endif
                      from.cmd.itemArgType=ST_HSV255;         
                      break;
                      case ST_HS:

                            #ifndef ADAFRUIT_LED     
                                from.param.v = hsv.v;
                            #else
                                from.param.v=255;
                            #endif     
                      from.cmd.itemArgType=ST_HSV255;
                      break;
                 }
             }
           //Converting  current obj to HSV
           
           debugSerial<<F("Updated:"); from.debugOut();
           param=from.param;
           cmd=from.cmd;
           return *this;

           case ST_HSV255:
                HSV255_flag=true;
           case ST_HSV:
                { // HSV_XX to RGB_XX translation code
                int rgbSaturation;
                int rgbValue;

                if (HSV255_flag)
                  {
                    //rgbSaturation = from.param.s;
                    rgbSaturation =map(from.param.s, 0, 100, 0, 255);
                    rgbValue      = from.param.v;
                  }
                else
                  {
                    rgbSaturation =map(from.param.s, 0, 100, 0, 255);
                    rgbValue =     map(from.param.v, 0, 100, 0, 255);
                  }

                  if (RGBW_flag)
                  {
                      if (rgbSaturation < 128) { // Using white
                                      param.w=map((127 - rgbSaturation) * rgbValue, 0, 127*255, 0, 255);
                                      int rgbvLevel = map (rgbSaturation,0,127,0,255*2);
                                      rgbValue = map(rgbValue, 0, 255, 0, rgbvLevel);
                                      rgbSaturation = map(rgbSaturation, 0, 127, 100, 255);
                                      if (rgbValue>255) rgbValue = 255;
                                     }
                      else
                      {
                        rgbSaturation = map(rgbSaturation, 128, 255, 100, 255);
                        param.w=0;
                      }
                    debugSerial<<F("Converted S:")<<rgbSaturation<<F(" Converted V:")<<rgbValue<<endl;
                  }
                  #ifdef ADAFRUIT_LED
                    Adafruit_NeoPixel strip(0, 0, 0);
                    uint32_t rgb = strip.ColorHSV(map(from.param.h, 0, 365, 0, 65535), rgbSaturation, rgbValue);
                    param.r=(rgb >> 16)& 0xFF;
                    param.g=(rgb >> 8) & 0xFF;
                    param.b=rgb & 0xFF;
                  #else
                    CRGB rgb = CHSV(map(from.param.h, 0, 365, 0, 255), rgbSaturation, rgbValue);
                    param.r=rgb.r;
                    param.g=rgb.g;
                    param.b=rgb.b;
                  #endif
                  debugSerial<<F("RGBx: ");
                  debugOut();
                  break;
                }
            default:
                    debugSerial<<F("Wrong Assignment ")<<from.cmd.itemArgType<<F("->")<<cmd.itemArgType<<endl;
         } //Translation to RGB_XX
        break;
     } //Destination
    return *this;
}

bool itemCmd::isCommand()
{
  return (cmd.cmdCode);
}

bool itemCmd::isValue()
{
return (cmd.itemArgType);
}

bool itemCmd::isColor()
{
return (cmd.itemArgType==ST_HS || cmd.itemArgType==ST_HSV || cmd.itemArgType==ST_HSV255 || cmd.itemArgType==ST_RGB || cmd.itemArgType==ST_RGBW);
}


long int itemCmd::getInt()
{
  switch (cmd.itemArgType) {

    case ST_INT32:
    case ST_UINT32:
    case ST_RGB:
    case ST_RGBW:
      return param.aslong;

    case ST_PERCENTS:
    case ST_PERCENTS255:
    case ST_HSV:
    case ST_HSV255:
      return param.v;

    case ST_FLOAT:
    case ST_FLOAT_CELSIUS:
    case ST_FLOAT_FARENHEIT:
      return int (param.asfloat);

 
    default:
    return 0;
  }
}


short itemCmd::getPercents(bool inverse)
{
  switch (cmd.itemArgType) {

    case ST_PERCENTS:
    case ST_HSV:
      if (inverse) return 100-param.v; else return param.v;

    case ST_PERCENTS255:
    case ST_HSV255:
      if (inverse) return map(param.v,0,255,100,0);
          else return map(param.v,0,255,0,100);

    default:
    return 0;
  }
}

short itemCmd::getPercents255(bool inverse)
{
  switch (cmd.itemArgType) {

    case ST_PERCENTS:
    case ST_HSV:
    if (inverse) return map(param.v,0,100,255,0);
            else return map(param.v,0,100,0,255);

    case ST_PERCENTS255:
    case ST_HSV255:
    if (inverse) return 255-param.v; else return param.v;

    default:
    return 0;
  }
}

uint8_t itemCmd::getCmd()
{
  return cmd.cmdCode;
}

uint8_t itemCmd::getArgType()
{
  return cmd.itemArgType;
}

itemCmd itemCmd::setArgType(uint8_t type)
{
   cmd.itemArgType=type & 0xF;
  return *this;
}

uint8_t itemCmd::getCmdParam()
{
  if (isCommand()) return cmd.cmdParam;
  return 0;
}

itemCmd itemCmd::Percents(int i)
    {
      if (i<0) i=0;
      if (i>100) i=100;

      switch (cmd.itemArgType)
      {
       case ST_HSV:
       case ST_PERCENTS:
         param.v=i;
       break;

       case ST_HSV255:
       case ST_PERCENTS255:
          param.v=map(i,0,100,0,255);
       break;
       default:
       cmd.itemArgType=ST_PERCENTS;
       param.v=i;
      }
      return *this;
    }

    itemCmd itemCmd::Percents255(int i)
        {
          if (i<0) i=0;
          if (i>255) i=255;

          switch (cmd.itemArgType)
          {
           case ST_HSV:
           case ST_PERCENTS:
             param.v=map(i,0,255,0,100);
           break;

           case ST_HSV255:
           case ST_PERCENTS255:
              param.v=i;
            break;
           default:
           cmd.itemArgType=ST_PERCENTS255;
           param.v=i;
          }
          return *this;
        }

itemCmd itemCmd::Int(int32_t i)
        {
          cmd.itemArgType=ST_INT32;
          param.asInt32=i;
          return *this;
        }



itemCmd itemCmd::HSV(uint16_t h, uint8_t s, uint8_t v)
{
  cmd.itemArgType=ST_HSV;
  param.h=h;
  param.s=s;
  param.v=v;

  return *this;
}

itemCmd itemCmd::HSV255(uint16_t h, uint8_t s, uint8_t v)
{
  cmd.itemArgType=ST_HSV255;
  param.h=h;
  param.s=s;
  param.v=v;

  return *this;
}


itemCmd itemCmd::HS(uint16_t h, uint8_t s)
{
  cmd.itemArgType=ST_HS;
  param.h=h;
  param.s=s;

  return *this;
}


itemCmd itemCmd::RGB(uint8_t r, uint8_t g, uint8_t b)
{
  cmd.itemArgType=ST_RGB;
  param.r=r;
  param.g=g;
  param.b=b;

  return *this;
}

itemCmd itemCmd::RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
  cmd.itemArgType=ST_RGBW;
  param.r=r;
  param.g=g;
  param.b=b;
  param.w=w;

  return *this;
}

itemCmd itemCmd::Int(uint32_t i)
                {
                  cmd.itemArgType=ST_UINT32;
                  param.asUint32=i;
                  return *this;
                }


itemCmd itemCmd::Cmd(uint8_t i)
    {
    //      cmd.itemArgType=ST_COMMAND;
          cmd.cmdCode=i;
          return *this;
    }


uint8_t itemCmd::getSuffix()
{
  return cmd.suffixCode;
}

itemCmd itemCmd::setSuffix(uint8_t suffix)
{
  cmd.suffixCode=suffix;
  return *this;
}

bool itemCmd::loadItem(Item * item, bool includeCommand)
{
  if (item && item->isValid())
  {
  short subtype =item->getSubtype();
  if (subtype)
        {
          param.asInt32=item->getVal();
          cmd.itemArgType= subtype;
          if (includeCommand) cmd.cmdCode=item->getCmd();
          debugSerial<<F("Loaded :");
          debugOut();
          return 1;
        }
  }
return false;
}

bool itemCmd::saveItem(Item * item, bool includeCommand)
{
  if (item && item->isValid())
  {
  item->setVal(param.asInt32);
  item->setSubtype(cmd.itemArgType);
  if (includeCommand) item->setCmd(cmd.cmdCode);
  debugSerial<<F("Saved :");
  debugOut();
  return true;
  }
return false;
}



char * itemCmd::toString(char * Buffer, int bufLen, int sendFlags )
     {

       if (!Buffer || !bufLen) return NULL;
       *Buffer=0;
       char * argPtr=Buffer;
       if (isCommand() && (sendFlags & SEND_COMMAND))
                        {
                          int len;
                          strncpy_P(Buffer, commands_P[cmd.cmdCode], bufLen);
                          strncat(Buffer, " ", bufLen);
                          len=strlen(Buffer);
                          argPtr+=len;
                          bufLen-=len;
                        }
       if (sendFlags & SEND_PARAMETERS)
       switch (cmd.itemArgType)
       { short colorTemp;

         case ST_PERCENTS:
         case ST_PERCENTS255:
            snprintf(argPtr, bufLen, "%u", param.v);
        break;
         case ST_UINT32:
            snprintf(argPtr, bufLen, "%lu", param.asUint32);
         break;
         case ST_INT32:
            snprintf(argPtr, bufLen, "%ld", param.asInt32);

         break;
         case ST_HSV:
         case ST_HSV255:
         colorTemp=getColorTemp();

         if (colorTemp<0) 
              snprintf(argPtr, bufLen, "%d,%d,%d", param.h, param.s, param.v);
            else   
              snprintf(argPtr, bufLen, "%d,%d,%d,%d", param.h, param.s, param.v, colorTemp);    

         break;
         case ST_HS:
         snprintf(argPtr, bufLen, "%d,%d", param.h, param.s);

         break;
         case ST_FLOAT_CELSIUS:
         case ST_FLOAT_FARENHEIT:
         case ST_FLOAT:
         snprintf(argPtr, bufLen, "%.1f", param.asfloat);
         break;
         case ST_RGB:
         snprintf(argPtr, bufLen, "%d,%d,%d", param.r, param.g, param.b);
         break;

         case ST_RGBW:
         snprintf(argPtr, bufLen, "%d,%d,%d,%d", param.r, param.g, param.b,param.w);
         break;

         case ST_STRING:
         strncpy(argPtr, param.asString,bufLen);

         break;
         default:
         ;
       }
      return Buffer;
     }

     void itemCmd::debugOut()
     {
       char buf[32];
       toString(buf,sizeof(buf));
       debugSerial<<buf<<F(" AT:")<<getArgType()<<F(" Suff:")<<getSuffix()<<endl;
     }
