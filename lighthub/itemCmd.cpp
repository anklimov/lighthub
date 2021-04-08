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

/*!
     \brief Constructor with loading value from Item
     \param Item
*/
itemCmd::itemCmd(Item *item)
{
  cmd.aslong=0;
  param.aslong=0;

  loadItem(item);
}


itemCmd itemCmd::setChanType(short chanType)
{
  cmd.itemArgType=getStoragetypeByChanType(chanType);
  return *this;
}


uint8_t itemCmd::getStoragetypeByChanType(short chanType)
{
  switch (chanType)
  {
    case CH_RGB:
    case CH_RGBW:
    case CH_SPILED:
    return ST_HSV255;
    break;
    case CH_AC:
    case CH_THERMO:
    case CH_VCTEMP:
    return ST_FLOAT_CELSIUS;
    break;
    case CH_DIMMER:
    case CH_MOTOR:
    case CH_PWM:
    case CH_RELAY:
    case CH_VC:
    return ST_PERCENTS255;
    break;
    default:
    return ST_VOID;
  }

}


itemCmd itemCmd::setDefault()
{
  switch (cmd.itemArgType){
    case ST_FLOAT_CELSIUS: param.asfloat=20.;
    break;
    case ST_FLOAT_FARENHEIT: param.asfloat=75.;
    break;
  
    case ST_HS:
    case ST_HSV255: param.h=100; param.s=0; param.v=255;
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
      cmd.itemArgType=ST_HSV255;
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
      cmd.itemArgType=ST_HSV255;
     case ST_HSV255:
      if (par>100) par=100;
      param.s=par;
      break;
    default:
//    debugSerial<<F("Can't assign saturation to type ")<<cmd.itemArgType<<endl;
    return false;
  }
  return true;
}



//! Setup color tempetature parameter for HSV or HSV255 types. It must be 153..500 (mireds) value.
//! Internally  1 - cold, 101 - warm light
bool itemCmd::setColorTemp(int t)
{
  int par=map(t,153,500,0,100);
  switch (cmd.itemArgType)
  {
    case ST_VOID:
      cmd.itemArgType=ST_HSV255;
    case ST_HS:
    case ST_PERCENTS255:
    case ST_HSV255:
      if (par>100) par=100;
      if (par<0) par=0;
      //debugSerial<<F("Assign color temp:")<<par<<endl;
      // value 0 is reserved for default/uninitialized value. Internally data stored in 1..101 range (7 bits)
      param.colorTemp=par+1;
      break;
    default:
    return false;
  }
  return true;
}

//! Setup color tempetature parameter from HSV or HSV255 types. return 0..100 value in success.
//! -1 - if no value stored
int itemCmd::getColorTemp()
{
  if (!param.colorTemp) return -1;

  switch (cmd.itemArgType)
  {
    case ST_HS:
    case ST_PERCENTS255:
    case ST_HSV255:
  
  return map(param.colorTemp-1,0,100,153,500);
    break;
  }
  return -1;  
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
    case ST_PERCENTS255:
    case ST_HSV255:
     par+=dif;
     if (par>255) par=255;
     if (par<0) par=0;
    param.v=par;
    break;

    case ST_INT32:
    case ST_UINT32:
     par=param.asInt32;
     par+=dif;
     if (par>100) par=100;
     if (par<0) par=0;
     param.asInt32=par;
    break;  

    case ST_FLOAT:
    case ST_FLOAT_CELSIUS:
    case ST_FLOAT_FARENHEIT:
     par=param.asfloat;
     par+=dif;
     if (par>100) par=100;
     if (par<0) par=0;
     param.asfloat=par;
    break;  
 
    case ST_TENS:
    
     par=param.asInt32;
     par+=dif*10;
     if (par>1000) par=1000;
     if (par<0) par=0;
     param.asInt32=par;
    break;  

   default: return false;
  }
  
  return true;
}

bool itemCmd::incrementH(int16_t dif)
{ int par=param.h;
  switch (cmd.itemArgType)
  {
  //case ST_HSV:
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
    case ST_HSV255:
     par+=dif;
     if (par>100) par=100;
     if (par<0) par=0;
    break;

   default: return false;
  }
  param.s=par;
  return true;

}


itemCmd itemCmd::assignFrom(itemCmd from, short chanType)
{
  bool RGBW_flag   = false;
  bool toFarenheit = false;
  short prefferedStorageType = getStoragetypeByChanType(chanType);

  int t=from.getColorTemp();

  if (t>=0) 
              {
              setColorTemp(t);
              }
  cmd.suffixCode=from.cmd.suffixCode;
  
  
  switch (cmd.itemArgType){ //Destination
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
                   param.h=from.param.h;
                   param.s=from.param.s;
                   break;
              case ST_INT32:
              case ST_UINT32:
                   param.v=constrain(from.param.asInt32,0,255);
                   break;
              case ST_TENS:
                   param.v=constrain(from.param.asInt32/10,0,255);   
                   break;                    
              case ST_HSV255:
                   param.h=from.param.h;
                   param.s=from.param.s;
                   param.v=from.param.v;
                   break;
              case ST_PERCENTS255:
                   param.v=from.param.v;
                   break;
              case ST_FLOAT:
                          param.v=constrain(from.param.asfloat,0.,255.);
                  break;    
              default:
                       debugSerial<<F("Wrong Assignment ")<<from.cmd.itemArgType<<F("->")<<cmd.itemArgType<<endl;
              }
             break;
        case ST_VOID:
             cmd.itemArgType=from.cmd.itemArgType;

        case ST_INT32:
        case ST_UINT32:
        case ST_TENS:
           switch (from.cmd.itemArgType)
           {
            case ST_HS:
              param.v=getPercents255();
              param.h=from.param.h;
              param.s=from.param.s; 
              cmd.itemArgType=ST_HSV255;  
           default:
              param.asInt32=from.param.asInt32;
              cmd.itemArgType=from.cmd.itemArgType;
           }
           break;
        case ST_HS:
           param.v=from.getPercents255();
           cmd.itemArgType=ST_HSV255; 
           break;     
        case ST_FLOAT_FARENHEIT:
            toFarenheit = true;
        case ST_FLOAT:
        case ST_FLOAT_CELSIUS:
           switch (from.cmd.itemArgType)
           {
             case ST_TENS:
              param.asfloat=from.param.asInt32/10.;
             break;
             case ST_PERCENTS255:             
              param.asfloat=from.param.v; 
             break;

             case ST_INT32:             
              param.asfloat=from.param.asInt32; 
             break;
             case ST_UINT32:
              param.asfloat=from.param.asUint32;
             break;

             case ST_FLOAT_FARENHEIT: 
             // F to C code should be here
             // if (!toFarenheit) convert (from.param.asfloat); cmd.itemArgType=ST_FARENHEIT
             case ST_FLOAT:
             case ST_FLOAT_CELSIUS:
              cmd.itemArgType=from.cmd.itemArgType;
              param.asfloat=from.param.asfloat; 
              break;
             case ST_HSV255:
             case ST_RGB:
             case ST_RGBW:
              cmd.itemArgType=from.cmd.itemArgType;
              param=from.param; 
              break;
              case ST_HS:
                   param.v=getPercents255();
                   param.h=from.param.h;
                   param.s=from.param.s; 
                   cmd.itemArgType=ST_HSV255;
              break;     
           default:
                     debugSerial<<F("Wrong Assignment ")<<from.cmd.itemArgType<<F("->")<<cmd.itemArgType<<endl;
           }
           
           break;

        case ST_RGBW:
             RGBW_flag=true;
        case ST_RGB:
        switch (from.cmd.itemArgType)
          {
            case ST_RGBW:
                 // RGBW_flag=true;
            case ST_RGB:
                  param.asInt32=from.param.asInt32;
                  cmd.itemArgType=from.cmd.itemArgType;
                  return *this;
            //break;
            // Those types are not possible to apply over RGB without convertion toward HSV
            case ST_FLOAT:
            case ST_HS:
            case ST_INT32:
            case ST_PERCENTS255:
            case ST_TENS:
            case ST_UINT32:
            {

             #ifndef ADAFRUIT_LED  
             // Restoring HSV from RGB
              CRGB rgb;
              rgb.r = param.r;
              rgb.g = param.g;
              rgb.b = param.b;    
              CHSV hsv = rgb2hsv_approximate(rgb);
             #endif 

              // Calculate volume
              int vol=0;
              switch (from.cmd.itemArgType)
            {  
            case ST_PERCENTS255:
              vol=from.param.v;
              break;
            case ST_INT32:
            case ST_UINT32:
              vol=from.param.asInt32;
              break; 
            case ST_TENS:
              vol=from.param.asInt32/10;
              break; 
            case ST_FLOAT:
              vol=from.param.asfloat;
              break;
            case ST_HS:
               #ifndef ADAFRUIT_LED  
               vol=hsv.v;
               #else
               vol=255;
               #endif 
            }

              #ifndef ADAFRUIT_LED  
              // Restoring HSV from RGB
              from.param.h = map(hsv.h, 0, 255, 0, 365);
              from.param.s = map(hsv.s, 0, 255, 0, 100);
              #else
              from.param.h=100;
              from.param.s=0;  
              #endif

              from.cmd.itemArgType=ST_HSV255;
              from.param.v=vol;
            }
            // Continue processing with filled from HSV 

           case ST_HSV255:
                { // HSV_XX to RGB_XX translation code
                int rgbSaturation;
                int rgbValue;

                    rgbSaturation =map(from.param.s, 0, 100, 0, 255);
                    rgbValue      = from.param.v;

                  short colorT=from.param.colorTemp-1;


                  if (RGBW_flag)
                  
                          {
                              if (colorT<0)
                              { //ColorTemperature not set
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
                                              };
                              debugSerial<<F("Converted S:")<<rgbSaturation<<F(" Converted V:")<<rgbValue<<endl;                    
                              }
                              else
                              {
                                long coldPercent = map (colorT,0,100,100,30); 
                                long hotPercent  = map (colorT,0,100,30,100);
                                int rgbvLevel;

                                if (rgbSaturation < 128) { // Using white
                                                param.w=map((127 - rgbSaturation) * rgbValue *hotPercent, 0, 127*255*100, 0, 255);
                                                rgbvLevel = map (rgbSaturation+coldPercent,0,127+100,0,255*2);
                                                rgbValue = map(rgbValue, 0, 255, 0, rgbvLevel);
                                                rgbSaturation = map(rgbSaturation, 0, 127, 0, 255);
                                                if (rgbValue>255) rgbValue = 255;
                                              }
                                  else
                                      {
                                        rgbSaturation = map(rgbSaturation, 128, 255, 100, 255);
                                        param.w=0;
                                      };
                              debugSerial<<F("Cold:")<<coldPercent<<F(" Hot:")<<hotPercent<<F(" ConvS:")<<rgbSaturation<<F(" ConvV:")<<rgbValue<<F(" vLevel:")<<rgbvLevel<<endl;             
                              } 
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

   if (prefferedStorageType) convertTo(prefferedStorageType);

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

return (cmd.itemArgType==ST_HS || cmd.itemArgType==ST_HSV255 || cmd.itemArgType==ST_RGB || cmd.itemArgType==ST_RGBW);

}


long int itemCmd::getInt()
{
  switch (cmd.itemArgType) {

    case ST_INT32:
    case ST_UINT32:
    case ST_RGB:
    case ST_RGBW:
    
      return param.aslong;
    case ST_PERCENTS255:
    case ST_HSV255:
      return param.v;

    case ST_FLOAT:
    case ST_FLOAT_CELSIUS:
    case ST_FLOAT_FARENHEIT:
      return param.asfloat;
    case ST_TENS:
      return param.aslong/10; 


 
    default:
    return 0;
  }
}


float itemCmd::getFloat()
{
  switch (cmd.itemArgType) {

    case ST_INT32:
    case ST_UINT32:
    case ST_RGB:
    case ST_RGBW:
   
      return param.aslong;
     case ST_TENS:
      return param.aslong/10;  

    case ST_PERCENTS255:
    case ST_HSV255:
      return param.v;

    case ST_FLOAT:
    case ST_FLOAT_CELSIUS:
    case ST_FLOAT_FARENHEIT:
      return param.asfloat;

 
    default:
    return 0.;
  }
}



long int itemCmd::getSingleInt()
{
  if (cmd.cmdCode) return cmd.cmdCode;
  return getInt();
}

short itemCmd::getPercents(bool inverse)
{
  switch (cmd.itemArgType) {
  
    case ST_INT32:
    case ST_UINT32:
           if (inverse) return constrain(100-param.asInt32,0,100);
            else return constrain(param.asInt32,0,100); 

    case ST_PERCENTS255:
    case ST_HSV255:
      if (inverse) return map(param.v,0,255,100,0);
          else return map(param.v,0,255,0,100);

    case ST_FLOAT:
       if (inverse) return constrain (100-param.asfloat,0,100);
            else return constrain (param.asfloat,0,100);   

    case ST_TENS:
           if (inverse) return constrain (100-param.asInt32/10,0,100);
            else return constrain(param.asInt32/10,0,100);


    default:
    return -1;
  }
}

bool itemCmd::setPercents(int percents)
{
  switch (cmd.itemArgType) {

    case ST_INT32:
    case ST_UINT32:
        param.asInt32=map(percents,0,100,0,255);
    break;       
    case ST_PERCENTS255:
    case ST_HSV255:
      param.v=map(percents,0,100,0,255);
    break;  
    case ST_FLOAT:
      param.asfloat=map(percents,0,100,0,255);;
    break;   
    case ST_TENS:
    param.asInt32 = map(percents,0,100,0,2550);;
    default:
    return false;
  }
return true;  
}

short itemCmd::getPercents255(bool inverse)
{
  switch (cmd.itemArgType) {

    case ST_INT32:
    case ST_UINT32:
        if (inverse) return 255-constrain(param.asInt32,0,255); else return constrain(param.asInt32,0,255);

    case ST_PERCENTS255:
    case ST_HSV255:
       if (inverse) return 255-param.v; else return param.v;

    case ST_FLOAT:
       if (inverse) return 255-constrain(param.asfloat,0,255); else return constrain(param.asfloat,0,255); 

    case ST_TENS:
      if (inverse) return 255-constrain(param.asInt32/10,0,255); else return constrain(param.asInt32/10,0,255); 

    default:
    return -1;
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


itemCmd itemCmd::convertTo(uint8_t type)
{ 
  if (cmd.itemArgType == type) return *this;
  debugSerial << F("Converting ") << cmd.itemArgType << F("->") << type << F(" ");
  itemCmd out(type,cmd.cmdCode);
   *this=out.assignFrom(*this);
   debugOut();
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
       case ST_HSV255:
       case ST_PERCENTS255:
       break;
       default:
       cmd.itemArgType=ST_PERCENTS255;
      }
      param.v=map(i,0,100,0,255);
      return *this;
    }

    itemCmd itemCmd::Percents255(int i)
        {
          if (i<0) i=0;
          if (i>255) i=255;

          switch (cmd.itemArgType)
          {
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

itemCmd itemCmd::Int(uint32_t i)
        {
          cmd.itemArgType=ST_UINT32;
          param.asUint32=i;
          return *this;
        }

itemCmd itemCmd::Float(float f)
        {
          cmd.itemArgType=ST_FLOAT;
          param.asfloat=f;
          return *this;
        }

itemCmd itemCmd::Tens(int32_t i)
        {
          cmd.itemArgType=ST_TENS;
          param.asInt32=i;
          return *this;
        }

itemCmd itemCmd::HSV(uint16_t h, uint8_t s, uint8_t v)
{
  cmd.itemArgType=ST_HSV255;
  param.h=h;
  param.s=s;
  param.v=map(v,0,100,0,255);

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
          //debugSerial<<F("Loaded :");
          //debugOut();
          return 1;
        }
  switch (item->itemVal->type)
    {
      case aJson_Int:
      Int((int32_t)item->itemVal->valueint);
      return true;
      
      case aJson_Float:
      Float(item->itemVal->valueint);
      return true;
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
  debugSerial<<F("Saved:");
  debugOut();
  return true;
  }
return false;
}


  int itemCmd::doMapping(aJsonObject *mappingData)
  {


  }
  int itemCmd::doReverseMapping (aJsonObject *mappingData)

  {

  }

int itemCmd::doMappingCmd(aJsonObject *mappingData)
  {


  }
  int itemCmd::doReverseMappingCmd (aJsonObject *mappingData)

  {

  }

char * itemCmd::toString(char * Buffer, int bufLen, int sendFlags, bool scale100 )
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
                          bufLen--;
                        }
       if (sendFlags & SEND_PARAMETERS)
       switch (cmd.itemArgType)
       { short colorTemp;

         case ST_PERCENTS255:
            snprintf(argPtr, bufLen, "%u", (scale100)?map (param.v,0,255,0,100):param.v);
        break;
         case ST_UINT32:
            snprintf(argPtr, bufLen, "%lu", param.asUint32);
         break;
         case ST_INT32:
            snprintf(argPtr, bufLen, "%ld", param.asInt32);
         break;
         case ST_TENS:
            snprintf(argPtr, bufLen, "%ld.%d", param.asInt32/10, abs(param.asInt32 % 10));
         break;
         case ST_HSV255:
         colorTemp=getColorTemp();

         if (colorTemp<0 || scale100) 
              snprintf(argPtr, bufLen, "%d,%d,%d", param.h, param.s, (scale100)?map (param.v,0,255,0,100):param.v);
            else   
              snprintf(argPtr, bufLen, "%d,%d,%d,%d", param.h, param.s, (scale100)?map (param.v,0,255,0,100):param.v, colorTemp);    

         break;
         case ST_HS:
         snprintf(argPtr, bufLen, "%d,%d", param.h, param.s);

         break;
         case ST_FLOAT_CELSIUS:
         case ST_FLOAT_FARENHEIT:
         case ST_FLOAT:
        { 

        float tmpVal = (param.asfloat < 0) ? -param.asfloat : param.asfloat;
        int tmpInt1 = tmpVal;                  // Get the integer 
        float tmpFrac = tmpVal - tmpInt1;      // Get fraction 
        int tmpInt2 = trunc(tmpFrac * 100);  // Turn into integer 
        // Print as parts, note that you need 0-padding for fractional bit.

        if (param.asfloat < 0)
        snprintf (argPtr, bufLen, "-%d.%02d",  tmpInt1, tmpInt2);
        else snprintf (argPtr, bufLen, "%d.%02d", tmpInt1, tmpInt2);
        }  
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

bool itemCmd::scale100()
{
  switch (cmd.itemArgType)
  {
    case ST_PERCENTS255:
    case ST_HSV255:
         param.v=constrain(map(param.v,0,100,0,255),0,255);
    return true;     
  }
return false;  
}