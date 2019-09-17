#ifndef AC_DISABLE

#include "modules/out_ac.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "item.h"

#define AC_Serial Serial3
#define INTERVAL_AC_POLLING      5000L

static int driverStatus = CST_UNKNOWN;

static int fresh =0;
static int power = 0;
static int swing =0;
static int lock_rem =0;
static int cur_tmp = 0;
static int set_tmp = 0;
static int fan_spd = 0;
static int mode = 0;

long prevPolling = 0;
byte inCheck = 0;
byte qstn[] = {255,255,10,0,0,0,0,0,1,1,77,1,90}; // Команда опроса
byte data[37] = {}; //Массив данных
byte on[]   = {255,255,10,0,0,0,0,0,1,1,77,2,91}; // Включение кондиционера
byte off[]  = {255,255,10,0,0,0,0,0,1,1,77,3,92}; // Выключение кондиционера
byte lock[] = {255,255,10,0,0,0,0,0,1,3,0,0,14};  // Блокировка пульта

//Extended subItem set
const char LOCK_P[]   PROGMEM = "lock";
const char QUIET_P[]  PROGMEM = "queit";
const char SWING_P[]  PROGMEM = "swing";
const char RAW_P[]    PROGMEM = "raw";
//const char IDLE_P[]   PROGMEM = "IDLE";
extern char HEAT_P[] PROGMEM;
extern char COOL_P[] PROGMEM;
extern char AUTO_P[] PROGMEM;
extern char FAN_ONLY_P[] PROGMEM;
extern char DRY_P[] PROGMEM;
extern char HIGH_P[] PROGMEM;
extern char MED_P[]  PROGMEM;
extern char LOW_P[]  PROGMEM;

void out_AC::InsertData(byte data[], size_t size){

    char s_mode[10];
    set_tmp = data[B_SET_TMP]+16;
    cur_tmp = data[B_CUR_TMP];
    mode    = data[B_MODE];
    fan_spd = data[B_FAN_SPD];
    swing   = data[B_SWING];
    power   = data[B_POWER];
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

  Serial.print ("Power=");
  Serial.println(power);

  if (power & 0x08)
      publishTopic(item->itemArr->name, "ON", "/quiet");
  else  publishTopic(item->itemArr->name, "OFF" , "/quiet");


  if (power & 0x02) //Compressor on
      publishTopic(item->itemArr->name, "ON","/compressor");
  else
      publishTopic(item->itemArr->name, "OFF","/compressor");


  publishTopic(item->itemArr->name, (long) swing,"/swing");
  //publishTopic(item->itemArr->name, (long) fan_spd,"/fan");

  /////////////////////////////////
  if (fan_spd == 0x00){
      publishTopic(item->itemArr->name, "high","/fan");
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
  publishTopic(item->itemArr->name, (long)cur_tmp, "/temp");
  ////////////////////////////////////
  s_mode[0]='\0';

  if (mode == 0x00){
    strcpy_P(s_mode,AUTO_P);
  }
  else if (mode == 0x01){
    strcpy_P(s_mode,COOL_P);
  }
  else if (mode == 0x02){
    strcpy_P(s_mode,HEAT_P);
  }
  else if (mode == 0x03){
    strcpy_P(s_mode,FAN_ONLY_P);
  }
  else if (mode == 0x04){
    strcpy_P(s_mode,DRY_P);
  }

  publishTopic(item->itemArr->name, (long) mode, "/mode");

  if (power & 0x01)
      publishTopic(item->itemArr->name, s_mode,"/cmd");

  else publishTopic(item->itemArr->name, "OFF","/cmd");

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

///////////////////////////////////
}

byte getCRC(byte req[], size_t size){
  byte crc = 0;
  for (int i=2; i < size; i++){
      crc += req[i];
  }
  return crc;
}

void SendData(byte req[], size_t size){
  AC_Serial.write(req, size - 1);
  AC_Serial.write(getCRC(req, size-1));
  AC_Serial.flush();
}

inline unsigned char toHex( char ch ){
   return ( ( ch >= 'A' ) ? ( ch - 'A' + 0xA ) : ( ch - '0' ) ) & 0x0F;
}



int  out_AC::Setup()
{
Serial.println("AC Init");
AC_Serial.begin(9600);
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_AC::Stop()
{
Serial.println("AC De-Init");

driverStatus = CST_UNKNOWN;
return 1;
}

int  out_AC::Status()
{
return driverStatus;
}

int out_AC::isActive()
{
return (power & 1);
}

int out_AC::Poll()
{
long now = millis();
  if (now - prevPolling > INTERVAL_AC_POLLING) {
    prevPolling = now;
    Serial.println ("Polling");
    SendData(qstn, sizeof(qstn)/sizeof(byte)); //Опрос кондиционера
  }
delay(100);
  if(AC_Serial.available() > 0){
    AC_Serial.readBytes(data, 37);
    while(AC_Serial.available()){
      delay(2);
      AC_Serial.read();
    }
    if (data[36] != inCheck){
      inCheck = data[36];
      InsertData(data, 37);
    }
  }
return 1;
};

int out_AC::Ctrl(short cmd, short n, int * Parameters, boolean send, int suffixCode, char* subItem)
{char s_mode[10];
 // Some additional Subitems
       if (strcmp_P(subItem, LOCK_P) == 0) suffixCode = S_LOCK;
  else if (strcmp_P(subItem, SWING_P) == 0) suffixCode = S_SWING;
  else if (strcmp_P(subItem, QUIET_P) == 0) suffixCode = S_QUIET;
  else if (strcmp_P(subItem, RAW_P) == 0) suffixCode = S_RAW;

 data[B_POWER] = power;
  //    debugSerial<<F(".");
      switch(suffixCode)
      {
      case S_SET:
          set_tmp = Parameters[0]-16;
          if (set_tmp >= 0 && set_tmp <= 30)
          {
            data[B_SET_TMP] = set_tmp;
            if (send) publishTopic(item->itemArr->name,(long)Parameters[0],"/set");
            }
      break;

      case S_CMD:
      s_mode[0]='\0';
            switch (cmd)
                {
                  case CMD_ON:
                      data[B_POWER] |= 1;
                      SendData(on, sizeof(on)/sizeof(byte));
                      if (send) publishTopic(item->itemArr->name,"ON","/cmd");
                      return 1;
                  break;
                  case CMD_OFF:
                  case CMD_HALT:
                      data[B_POWER] &= ~1;
                      SendData(off, sizeof(off)/sizeof(byte));
                      if (send) publishTopic(item->itemArr->name,"OFF","/cmd");
                      return 1;
                  break;
                  case CMD_AUTO:
                      data[B_MODE] = 0;
                      data[B_POWER] |= 1;
                      strcpy_P(s_mode,AUTO_P);
                  break;
                  case CMD_COOL:
                      data[B_MODE] = 1;
                      data[B_POWER] |= 1;
                      strcpy_P(s_mode,COOL_P);
                  break;
                  case CMD_HEAT:
                      data[B_MODE] = 2;
                      data[B_POWER] |= 1;
                      strcpy_P(s_mode,HEAT_P);
                  break;
                  case CMD_DRY:
                      data[B_MODE] = 4;
                      data[B_POWER] |= 1;
                      strcpy_P(s_mode,DRY_P);
                  break;
                  case CMD_FAN:
                      data[B_MODE] = 3;
                      data[B_POWER] |= 1;
                      strcpy_P(s_mode,FAN_ONLY_P);
                  break;
                  case CMD_UNKNOWN:
                  return -1;
                  break;
                }
                if (send) publishTopic(item->itemArr->name,s_mode,"/cmd");
      break;

      case S_FAN:
      s_mode[0]='\0';
      switch (cmd)
      {
      case CMD_AUTO:
      data[B_FAN_SPD] = 3;
      strcpy_P(s_mode,AUTO_P);
      break;
      case CMD_HIGH:
      data[B_FAN_SPD] = 0;
      strcpy_P(s_mode,HIGH_P);
      break;
      case CMD_MED:
      data[B_FAN_SPD] = 1;
      strcpy_P(s_mode,MED_P);
      break;
      case CMD_LOW:
      data[B_FAN_SPD] = 2;
      strcpy_P(s_mode,LOW_P);
      break;
      default:
      if (n) data[B_FAN_SPD] = Parameters[0];
      //TODO - mapping digits to speed
      }
      if (send) publishTopic(item->itemArr->name,s_mode,"/fan");
      break;

      case S_MODE:
      data[B_MODE]    = Parameters[0];
      break;

      case S_LOCK:
      switch (cmd)
          {
            case CMD_ON:
            data[B_LOCK_REM] = 80;
            break;
            case CMD_OFF:
            data[B_LOCK_REM] = 0;
            break;
          }
      break;

      case S_SWING:
      switch (cmd)
          {
            case CMD_ON:
            data[B_LOCK_REM] = 3;
            break;
            case CMD_OFF:
            data[B_LOCK_REM] = 0;
            break;
            default:
            if (n) data[B_SWING] = Parameters[0];
          }
      break;

      case S_QUIET:
      switch (cmd)
          {
            case CMD_ON:
            data[B_POWER] |= 8;
            break;
            case CMD_OFF:
            data[B_POWER] &= ~8;
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
        AC_Serial.write(data, 37);
        AC_Serial.flush();
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

    data[B_CMD] = 0;
    data[9] = 1;
    data[10] = 77;
    data[11] = 95;
    SendData(data, sizeof(data)/sizeof(byte));
    //InsertData(data, sizeof(data)/sizeof(byte));

    return 1;
}



#endif
