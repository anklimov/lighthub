
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
/// Fixed I2C address of the CCS811 sensor (alternate address).
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
/**
 * @brief CCS811 eCO2 / eTVOC air-quality sensor input (IN_CCS811 = 5).
 *
 * Reads the MOX air-quality sensor over I2C at the fixed address
 * CCS811_ADDR (0x5A). Polls the sensor when new data is available,
 * suppresses spontaneous calculation errors (CO2 >= 10000 ppm) and
 * publishes the readings on the input "in" section:
 * - /CO2   - estimated CO2 concentration (ppm);
 * - /TVOC  - estimated total volatile organic compounds (ppb);
 * - /base  - sensor baseline value.
 *
 * The humidity/temperature measured by the companion HDC1080 input are
 * forwarded here via setEnvironmentalData() to improve accuracy.
 */
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

/**
 * @brief HDC1080 temperature / humidity sensor input (IN_HDC1080 = 6).
 *
 * Reads the humidity/temperature sensor over I2C at the fixed address
 * 0x40. Polls on the slow polling cycle, resets the I2C bus on a read
 * glitch, publishes the readings on the input "in" section and routes
 * the humidity value to the "hum" key of the input object:
 * - /T - temperature (Celsius);
 * - /H - relative humidity (percent).
 *
 * The measured humidity/temperature are also forwarded to the CCS811
 * sensor (when present) via setEnvironmentalData().
 */
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
