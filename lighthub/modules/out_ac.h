
#pragma once
#ifndef AC_DISABLE
#include <abstractout.h>

#define LEN_B   37
#define B_CUR_TMP   13  //Текущая температура
#define B_CMD       17  // 00-команда 7F-ответ ???
#define B_MODE      23  //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
#define B_FAN_SPD   25  //Скорость 02 - min, 01 - mid, 00 - max, 03 - auto
#define B_SWING     27  //01 - верхний и нижний предел вкл. 00 - выкл. 02 - левый/правый вкл. 03 - оба вкл
#define B_LOCK_REM  28  //80 блокировка вкл. 00 -  выкл
#define B_POWER     29  //on/off 01 - on, 00 - off (10, 11)-Компрессор??? 09 - QUIET
#define B_FRESH     31  //fresh 00 - off, 01 - on
#define B_SET_TMP   35  //Установленная температура

#define S_LOCK S_ADDITIONAL+1
#define S_QUIET S_ADDITIONAL+2
#define S_SWING S_ADDITIONAL+3
#define S_RAW S_ADDITIONAL+4

extern void modbusIdle(void) ;
class out_AC : public abstractOut {
public:

    out_AC(Item * _item):abstractOut(_item){};
    int Setup() override;
    int Poll() override;
    int Stop() override;
    int Status() override;
    int isActive() override;
    int Ctrl(short cmd, short n=0, int * Parameters=NULL, boolean send=true, int suffixCode=0, char* subItem=NULL) override;

protected:
    void InsertData(byte data[], size_t size);
};
#endif
