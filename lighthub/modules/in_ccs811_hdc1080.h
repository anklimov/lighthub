
#pragma once
#ifndef CSSHDC_DISABLE
#include <inputs.h>
#include <abstractin.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "SparkFunCCS811.h" //Click here to get the library: http://librarymanager/All#SparkFun_CCS811

//#ifndef I2C_CLOCK
//#define I2C_CLOCK 4000
//#endif

//#define CCS811_ADDR 0x5B //Default I2C Address
#define CCS811_ADDR 0x5A //Alternate I2C Address

#if defined (ARDUINO_ARCH_ESP8266) 

 //   #if not defined (TWI_SCL) && defined (D1)
 //   #define TWI_SCL D1
 //   #endif

  //  #if not defined (WAK_PIN) && defined (D3)
  //  #define WAK_PIN D3
  //  #endif

    #if defined (TWI_SCL)
    #define SCL_LOW()   (GPES = (1 << TWI_SCL))
    #define SCL_HIGH()  (GPEC = (1 << TWI_SCL))
    #define SCL_RESET
    #endif

#endif



#if defined (ARDUINO_ARCH_ESP32)
//#undef WAK_PIN
#undef SCL_RESET
//#ifndef WAK_PIN
//#define WAK_PIN 17
//#endif
#endif

#if defined(ARDUINO_ARCH_AVR)
//#ifndef WAK_PIN
//#define WAK_PIN 3  // for LightHub UEXT SCS Pin
//#endif
#endif


class Input;
class in_ccs811 : public abstractIn {
public:
    //CCS811 ccs811(CCS811_ADDR);
    //uint16_t ccs811Baseline;
    in_ccs811(Input * _in):abstractIn(_in){};
    int Setup() override;
    int Poll(short cause) override;

protected:
   void printDriverError( CCS811Core::status errorCode );
   void printSensorError();
};

class in_hdc1080 : public abstractIn {
public:
    //ClosedCube_HDC1080 hdc1080;
    in_hdc1080(Input * _in):abstractIn(_in){};
    int Setup() override;
    int Poll(short cause) override;

protected:
    void printSerialNumber();
};
#endif
