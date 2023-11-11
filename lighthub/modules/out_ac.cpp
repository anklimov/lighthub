#ifndef AC_DISABLE

#include "modules/out_ac.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "item.h"
#include "textconst.h"
#include "main.h"

#include "config.h"
#include "streamlog.h"
extern systemConfig sysConf;
extern bool disableCMD;

#ifndef AC_Serial
#define AC_Serial Serial3
#endif

#define INTERVAL_AC_POLLING      5000L

#define AC_FAILED  CST_FAILED
#define AC_UNKNOWN CST_UNKNOWN
#define AC_IDLE CST_INITIALIZED
#define AC_SENDING 3

//byte inCheck = 0;
byte qstn[] = {255,255,10,0,0,0,0,0,1,1,77,1,90}; // Команда опроса

byte on[]   = {255,255,10,0,0,0,0,0,1,1,77,2,91}; // Включение кондиционера
byte off[]  = {255,255,10,0,0,0,0,0,1,1,77,3,92}; // Выключение кондиционера
byte lock[] = {255,255,10,0,0,0,0,0,1,3,0,0,14};  // Блокировка пульта

//Extended subItem set
const char LOCK_P[]   PROGMEM = "lock";
const char QUIET_P[]  PROGMEM = "queit";
const char SWING_P[]  PROGMEM = "swing";
const char RAW_P[]    PROGMEM = "raw";

void out_AC::getConfig(){
 
  ACSerial=&AC_Serial;
  if (!item) return;

  if (item->getArgCount())

    switch(portNum=item->getArg(0)){
      case 0: ACSerial=&Serial;
      
      break;
      #if not defined (AVR) || defined (DMX_DISABLE)
      case 1: ACSerial=&Serial1;
      break;
      #endif
      #if defined (HAVE_HWSERIAL2) || defined (__SAM3X8E__) || defined (ESP32)
      case 2: ACSerial=&Serial2;
      break;
      #endif
      #if defined (HAVE_HWSERIAL3) || defined (__SAM3X8E__) 
      case 3: ACSerial=&Serial3;
      break;
      #endif

    }


}

void out_AC::SubmitParameters(const char * name, itemCmd value){

    if (!item || !item->itemArg) return;
    if ((item->itemArg->type == aJson_Array) && ( 1 < aJson.getArraySize(item->itemArg)))
    { 
     aJsonObject * callbackObj = aJson.getArrayItem(item->itemArg, 1);  
     if (callbackObj && callbackObj->type == aJson_Object)
        {
          aJsonObject * execObj = aJson.getObjectItem(callbackObj,name);
          if (execObj) executeCommand(execObj,-1,value);
        }
    }
}

void out_AC::InsertData(byte data[], size_t size){

    int fresh =0;

    int swing =0;
    int lock_rem =0;
    int cur_tmp = 0;
    int set_tmp = 0;
    int fan_spd = 0;


    char s_buffer[10];
    itemCmd icmd;

    set_tmp = data[B_SET_TMP]+16;
    if (set_tmp>40 || set_tmp<16) return;
    cur_tmp = data[B_CUR_TMP];
    store->mode    = data[B_MODE];
    fan_spd = data[B_FAN_SPD];
    swing   = data[B_SWING];
    store->power   = data[B_POWER];
    lock_rem = data[B_LOCK_REM];
    fresh   = data[B_FRESH];
  /////////////////////////////////
  if (fresh & 0x01)
        publishTopic(item->itemArr->name, "ON","/fresh");
  else  publishTopic(item->itemArr->name, "OFF","/fresh");

  /////////////////////////////////
  if (lock_rem == 0x80){
      publishTopic(item->itemArr->name, "ON","/lock");
  }
  if (lock_rem == 0x00){
      publishTopic(item->itemArr->name, "OFF","/lock");
  }
  /////////////////////////////////
  /*
  if (power == 0x01 || power == 0x11){
      publishTopic(item->itemArr->name,"Power", "on");
  }
  if (power == 0x00 || power == 0x10){
      publishTopic(item->itemArr->name,"Power", "off");
  }
  */

  debugSerial.print ("AC: Power=");
  debugSerial.println(store->power);

  if (store->power & 0x08)
      publishTopic(item->itemArr->name, "ON", "/quiet");
  else  publishTopic(item->itemArr->name, "OFF" , "/quiet");


  if (store->power & 0x02) //Compressor on
      publishTopic(item->itemArr->name, "ON","/compressor");
  else
      publishTopic(item->itemArr->name, "OFF","/compressor");


  //publishTopic(item->itemArr->name, (long) swing,"/swing");
  //publishTopic(item->itemArr->name, (long) fan_spd,"/fan");

  /////////////////////////////////
   s_buffer[0]='\0';
  switch (fan_spd){
    case 0x00:
     strcpy_P(s_buffer,HIGH_P);
     icmd.Cmd(CMD_HIGH);
    break;
    case 0x01:
     strcpy_P(s_buffer,MED_P);
     icmd.Cmd(CMD_MED);
    break;
    case 0x02:
     strcpy_P(s_buffer,LOW_P);
     icmd.Cmd(CMD_LOW);
    break;
    case 0x03:
    strcpy_P(s_buffer,AUTO_P);
    icmd.Cmd(CMD_AUTO);
    break;
    default:
    strcpy_P(s_buffer,ERROR_P);
    icmd.Cmd(CMD_VOID);
  }
publishTopic(item->itemArr->name, s_buffer,"/fan");
SubmitParameters("fan",icmd);

 /*      
  if (fan_spd == 0x00){
      publishTopic(item->itemArr->name, "high","/fan");
      SubmitParameters("fan","")
  }
  if (fan_spd == 0x01){
      publishTopic(item->itemArr->name, "medium","/fan");
  }

  if (fan_spd == 0x02){
      publishTopic(item->itemArr->name, "low","/fan");
  }
  if (fan_spd == 0x03){
      publishTopic(item->itemArr->name, "auto","/fan");
  }
*/

  if (swing == 0x00)
       publishTopic(item->itemArr->name, "OFF","/swing");
  else publishTopic(item->itemArr->name, "ON","/swing");
  /*
  if (swing == 0x01){
      publishTopic(item->itemArr->name, "ud","swing");
  }
  if (swing == 0x02){
      publishTopic(item->itemArr->name, "lr","swing");
  }
  if (swing == 0x03){
      publishTopic(item->itemArr->name, "all","swing");
  }*/
  /////////////////////////////////

  publishTopic(item->itemArr->name,(long)set_tmp,"/set");
  if (cur_tmp!=255) publishTopic(item->itemArr->name, (long)cur_tmp, "/temp");
  ////////////////////////////////////
  
  //itoa(set_tmp,s_buffer,10);
  //SubmitParameters("set",itemCmd().Int((int32_t)set_tmp).setSuffix(S_SET));
  SubmitParameters("set",itemCmd().Int((int32_t)set_tmp));

  //itoa(cur_tmp,s_buffer,10);
  SubmitParameters("temp",itemCmd().Int((int32_t)cur_tmp).setSuffix(S_VAL));

  s_buffer[0]='\0';

  if (store->mode == 0x00){
    strcpy_P(s_buffer,AUTO_P);icmd.Cmd(CMD_AUTO);
  }
  else if (store->mode == 0x01){
    strcpy_P(s_buffer,COOL_P);icmd.Cmd(CMD_COOL);
  }
  else if (store->mode == 0x02){
    strcpy_P(s_buffer,HEAT_P);icmd.Cmd(CMD_HEAT);
  }
  else if (store->mode == 0x03){
    strcpy_P(s_buffer,FAN_ONLY_P);icmd.Cmd(CMD_FAN);
  }
  else if (store->mode == 0x04){
    strcpy_P(s_buffer,DRY_P);icmd.Cmd(CMD_DRY);
  }
  else if (store->mode == 109){
      strcpy_P(s_buffer,ERROR_P);icmd.Cmd(CMD_VOID);
  }

  publishTopic(item->itemArr->name, (long) store->mode, "/mode");

  if (!(store->power & 0x01)) {strcpy_P(s_buffer,OFF_P);icmd.Cmd(CMD_OFF);}
  publishTopic(item->itemArr->name, s_buffer,"/cmd");
  SubmitParameters("cmd",icmd);
 /* 
  if (store->power & 0x01)
      publishTopic(item->itemArr->name, s_buffer,"/cmd");

  else publishTopic(item->itemArr->name, "OFF","/cmd");
  */

/*
  String raw_str;
  char raw[75];
  for (int i=0; i < 37; i++){
     if (data[i] < 10){
       raw_str += "0";
       raw_str += String(data[i], HEX);
     } else {
      raw_str += String(data[i], HEX);
     }
  }
  raw_str.toUpperCase();
  raw_str.toCharArray(raw,75);
  publishTopic(item->itemArr->name, raw,"/raw");
  Serial.println(raw);
*/
///////////////////////////////////
}

byte getCRC(byte req[], size_t size){
  byte crc = 0;
  for (int i=2; i < size; i++){
      crc += req[i];
  }
  return crc;
}

void out_AC::SendData(byte req[], size_t size){
if (!store || !item) return;  
if (Status() == AC_SENDING)
     {
      while (store->timestamp && !isTimeOver(store->timestamp,millis(),150)) yield();
     }

  //#if defined (__SAM3X8E__)
  //if (item->getArg(0)==2) preTransmission();
  //#endif

  ACSerial->write(req, size - 1);
  ACSerial->write(getCRC(req, size-1));
  //ACSerial->flush();
  store->timestamp=millisNZ();
  debugSerial<<F("AC: ")<<portNum<<F(" <<");
  for (int i=0; i < size-1; i++)
  {
     if (req[i] < 10){
       debugSerial.print("0");
       debugSerial.print(req[i], HEX);
     }
        else
             {
             debugSerial.print(req[i], HEX);
             }
  }
debugSerial.println();
setStatus(AC_SENDING);
//  #if defined (__SAM3X8E__)
//  if (item->getArg(0)==2) postTransmission();
//  #endif
}

inline unsigned char toHex( char ch ){
   return ( ( ch >= 'A' ) ? ( ch - 'A' + 0xA ) : ( ch - '0' ) ) & 0x0F;
}



int  out_AC::Setup()
{
abstractOut::Setup();    
if (!item) return 0;  
debugSerial<<F("AC: Init: ")<<portNum<<endl;

if (!store) store= (acPersistent *)item->setPersistent(new acPersistent);
if (!store)
              { errorSerial<<F("AC: Out of memory")<<endl;
                return 0;}

memset(store->data,0,sizeof(acPersistent::data));
store->mode=0;
store->power=0;
store->inCheck=0;
store->timestamp=millisNZ();            

if (!portNum)// && (g_APinDescription[0].ulPinType == PIO_PA8A_URXD))
    {
      pinMode(0, INPUT_PULLUP);
      #if debugSerial == Serial
      infoSerial<<F("AC: Serial used by AC - disabling serial logging and cmd")<<endl;
      Serial.flush();
      //sysConf.setSerialDebuglevel(0);
      serialDebugLevel = 0;
      disableCMD=true;
      #endif
    }
ACSerial->begin(9600);
setStatus (AC_IDLE);


return 1;
}

int  out_AC::Stop()
{
if (!item) return 0;    
debugSerial<<F("AC: De-Init: ")<<portNum<<endl;
delete store;
item->setPersistent(NULL);
store = NULL;
setStatus (CST_UNKNOWN);
return 1;
}

/*
int  out_AC::Status()
{
if (!item) return 0;  
switch (item->itemArr->subtype)
{
case AC_FAILED: return CST_FAILED;
case AC_UNKNOWN: return CST_UNKNOWN;
default:
return CST_INITIALIZED;
//return item->itemArr->subtype;
}
}*/

int out_AC::isActive()
{
if (!store) return 0;  
return (store->power & 1);
}

int out_AC::Poll(short cause)
{
if (!store) return -1;

switch (Status())
{  
case AC_FAILED: return -1;
case AC_UNKNOWN: return -1;
case AC_SENDING:
      {
      if (store->timestamp && isTimeOver(store->timestamp,millis(),150)) 
                                            {
                                            setStatus(AC_IDLE);
                                            store->timestamp=millisNZ();
                                            }
      }
}

if (cause!=POLLING_SLOW) return false;

  if ((Status() == AC_IDLE) && isTimeOver(store->timestamp,millis(),INTERVAL_AC_POLLING)) 
  {
    debugSerial.println(F("AC: Polling"));
    SendData(qstn, sizeof(qstn)/sizeof(byte)); //Опрос кондиционера
  }

  if(ACSerial->available() >= 37){ //was 0
    ACSerial->readBytes(store->data, 37);
    while(ACSerial->available()){
      delay(2);
      ACSerial->read();
    }

debugSerial<<F("AC: ")<<portNum<<F(" >> ");
  for (int i=0; i < 37-1; i++)
  {
     if (store->data[i] < 10){
       debugSerial.print("0");
       debugSerial.print(store->data[i], HEX);
     }
        else
             {
             debugSerial.print(store->data[i], HEX);
             }
  }


    if (store->data[36] != store->inCheck){
      store->inCheck = store->data[36];
      InsertData(store->data, 37);
      debugSerial<<F("AC: OK");
    }

 debugSerial.println();   
  }
return true;
};

int out_AC::Ctrl(itemCmd cmd,  char* subItem , bool toExecute, bool authorized)
{
  //char s_mode[10];
  char s_speed[10];

  int suffixCode = cmd.getSuffix();
 // Some additional Subitems
       if (strcmp_P(subItem, LOCK_P) == 0) suffixCode = S_LOCK;
  else if (strcmp_P(subItem, SWING_P) == 0) suffixCode = S_SWING;
  else if (strcmp_P(subItem, QUIET_P) == 0) suffixCode = S_QUIET;
  else if (strcmp_P(subItem, RAW_P) == 0) suffixCode = S_RAW;

  if (cmd.isCommand() && !suffixCode && !subItem) suffixCode=S_CMD; //if some known command find, but w/o correct suffix - got it

 //data[B_POWER] = power;
  //    debugSerial<<F(".");
      switch(suffixCode)
      {
      case S_SET:
          {
          byte set_tmp = cmd.getInt();
          if (set_tmp >= 16 && set_tmp <= 40)
          {
            //if (set_tmp>40 || set_tmp<16) set_temp=21;
            store->data[B_SET_TMP] = set_tmp -16;
            publishTopic(item->itemArr->name,(long) set_tmp,"/set");
            }
          else return -1;
          }  
      break;

      case S_CMD:
     // s_mode[0]='\0';
            switch (cmd.getCmd())
                {
                  case CMD_ON:
                  case CMD_XON:
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                      SendData(on, sizeof(on)/sizeof(byte));
                   //   publishTopic(item->itemArr->name,"ON","/cmd");
                      return 1;
                  break;
                  case CMD_OFF:
                  case CMD_HALT:
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] &= ~1;
                      SendData(off, sizeof(off)/sizeof(byte));
                   //   publishTopic(item->itemArr->name,"OFF","/cmd");
                      return 1;
                  break;
                  case CMD_AUTO:
                      store->data[B_MODE] = 0;
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                    //  strcpy_P(s_mode,AUTO_P);
                  break;
                  case CMD_COOL:
                      store->data[B_MODE] = 1;
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                    //  strcpy_P(s_mode,COOL_P);
                  break;
                  case CMD_HEAT:
                      store->data[B_MODE] = 2;
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                    //  strcpy_P(s_mode,HEAT_P);
                  break;
                  case CMD_DRY:
                      store->data[B_MODE] = 4;
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                   //   strcpy_P(s_mode,DRY_P);
                  break;
                  case CMD_FAN:
                      store->data[B_MODE] = 3;
                    //  debugSerial<<"fan\n";
                      store->data[B_POWER] = store->power;
                      store->data[B_POWER] |= 1;
                      if (store->data[B_FAN_SPD] == 3)
                         {
                           store->data[B_FAN_SPD] = 2; //Auto - fan speed in Ventilation mode not working
                         }
                    //  strcpy_P(s_mode,FAN_ONLY_P);
                  break;
                  case CMD_UNKNOWN:
                  return -1;
                  break;
                }
              //  debugSerial<<F("Mode:")<<s_mode<<endl;
              //  publishTopic(item->itemArr->name,s_mode,"/cmd");
      break;

      case S_FAN:
      s_speed[0]='\0';
      switch (cmd.getCmd())
      {
      case CMD_AUTO:
      store->data[B_FAN_SPD] = 3;
      strcpy_P(s_speed,AUTO_P);
      break;
      case CMD_HIGH:
      store->data[B_FAN_SPD] = 0;
      strcpy_P(s_speed,HIGH_P);
      break;
      case CMD_MED:
      store->data[B_FAN_SPD] = 1;
      strcpy_P(s_speed,MED_P);
      break;
      case CMD_LOW:
      store->data[B_FAN_SPD] = 2;
      strcpy_P(s_speed,LOW_P);
      break;
      default:
      //if (n) data[B_FAN_SPD] = Parameters[0];
      store->data[B_FAN_SPD] = cmd.getInt();
      //TODO - mapping digits to speed
      }
      publishTopic(item->itemArr->name,s_speed,"/fan");
      break;

      case S_MODE:
      //data[B_MODE]    = Parameters[0];
      store->data[B_MODE]    = cmd.getInt();
      break;

      case S_LOCK:
      switch (cmd.getCmd())
          {
            case CMD_ON:
            store->data[B_LOCK_REM] = 80;
            break;
            case CMD_OFF:
            store->data[B_LOCK_REM] = 0;
            break;
          }
      break;

      case S_SWING:
      switch (cmd.getCmd())
          {
            case CMD_ON:
            store->data[B_SWING] = 3;
            publishTopic(item->itemArr->name,"ON","/swing"); 
            break;
            case CMD_OFF:
            store->data[B_SWING] = 0;
            publishTopic(item->itemArr->name,"OFF","/swing"); 
            break;
            default:
            //if (n) data[B_SWING] = Parameters[0];
            store->data[B_SWING] = cmd.getInt();
          }
         
      break;

      case S_QUIET:
      switch (cmd.getCmd())
          {
            case CMD_ON:
            store->data[B_POWER] |= 8;
            break;
            case CMD_OFF:
            store->data[B_POWER] &= ~8;
            break;
          }


      break;

      case S_RAW:
        /*
      {
        char buf[75];
        char hexbyte[3] = {0};
        strPayload.toCharArray(buf, 75);
        int octets[sizeof(buf) / 2] ;
        for (int i=0; i < 76; i += 2){
          hexbyte[0] = buf[i] ;
          hexbyte[1] = buf[i+1] ;
          data[i/2] = (toHex(hexbyte[0]) << 4) | toHex(hexbyte[1]);
        ACSerial->write(data, 37);
        ACSerial->flush();
        publishTopic("RAW", buf);
      }
      */

      break;

      case S_NOTFOUND:
      return -1;

   }

/*
    //////////

    if (strTopic == "/myhome/in/Conditioner/Swing"){
       if (strPayload == "off"){
        data[B_SWING] = 0;
      }
      if (strPayload == "ud"){
          data[B_SWING] = 1;
      }
      if (strPayload == "lr"){
          data[B_SWING] = 2;
      }
      if (strPayload == "all"){
          data[B_SWING] = 3;
      }
    }
    ////////

    */

    //if (strTopic == "/myhome/in/Conditioner/RAW")

    store->data[B_CMD] = 0;
    store->data[9] = 1;
    store->data[10] = 77;
    store->data[11] = 95;
    
    ///ACSerial->flush();

    //uint32_t ts=item->getExt();
    //while (ts && !isTimeOver(ts,millis(),100)) yield();
    SendData(store->data, sizeof(store->data)/sizeof(byte));
    //InsertData(data, sizeof(data)/sizeof(byte));
    //ACSerial->flush();
    //item->setExt(millisNZ());
    return 1;
}

int out_AC::getChanType() {return CH_THERMO;} 

#endif
