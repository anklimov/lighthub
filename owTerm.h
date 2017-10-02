//define APU_OFF

#define SW_AUX0 0x40
#define SW_AUX1 0x80
#define SW_STAT0 0x4
#define SW_STAT1 0x8
#define SW_OUT0 0x20
#define SW_OUT1 0x10
#define SW_MASK 0xF
#define SW_INMASK 0xFC

#define recheck_interval 5
#define check_circle 2000/t_count

#define SW_FIND        1 
#define SW_DOUBLECHECK 2      //Doublecheck required
#define SW_PULSE0      4      //Pulse Reset started 
#define SW_PULSE1      8      //Pulse Reset stsrted 
#define SW_PULSE_P0    0x10   //Pulse reset in process
#define SW_PULSE_P1    0x20   //Pulse reset in process
#define SW_CHANGED_P0  0x40   //Changes while pulse in progress
#define SW_CHANGED_P1  0x80   //Changes while pulse in progress
#define SW_PULSE0_R      0x100    //Pulse Reset requested 
#define SW_PULSE1_R      0x200    //Pulse Reset requested 


#define recheck_interval 5
#define check_circle 2000/t_count



#define t_max 20 //Maximum number of 1w devices
#define TEMPERATURE_PRECISION 9

#include <OneWire.h>
#include <DallasTemperature.h>
#include "aJSON.h"

extern aJsonObject *owArr;

typedef   void (*owChangedType) (int , DeviceAddress, int) ;

#define _2482 // HW driver

#ifdef _2482
#include <Wire.h>   
#else
#define ONE_WIRE_BUS A0
#endif

extern OneWire *net;

extern DallasTemperature *sensors;
extern DeviceAddress *term ;
extern int           *regs ;
extern uint16_t       *wstat;
extern int            t_count;
extern short          si;

extern owChangedType  owChanged;



int  owUpdate();
int  owSetup(owChangedType owCh);
void owLoop();
void owIdle(void) ; 
int owFind(DeviceAddress addr);
void owAdd (DeviceAddress addr);

