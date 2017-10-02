//define APU_OFF
#include <OneWire.h>
#include <DallasTemperature.h>



int  owRead2408(uint8_t* addr);
int  ow2408out(DeviceAddress addr,uint8_t cur);
//int  read1W(int i);
int  cntrl2408(uint8_t* addr, int subchan, int val=-1);
int cntrl2413(uint8_t* addr, int subchan, int val=-1);
int  cntrl2890(uint8_t* addr, int val);
