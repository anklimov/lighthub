#include "modules/in_ccs811_hdc1080.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "item.h"
#include "main.h"
#include "Wire.h"

#if defined(M5STACK)
#include <M5Stack.h>
#endif

#ifndef CSSHDC_DISABLE
static ClosedCube_HDC1080 hdc1080;
static CCS811 ccs811(CCS811_ADDR);

long ccs811Baseline;

static bool HDC1080ready = false;
static bool CCS811ready = false;


int  in_ccs811::Setup()
{
  if (CCS811ready) {errorSerial<<F("CCS811: Already initialized")<<endl; return 0;}

#ifdef WAK_PIN
  pinMode(WAK_PIN,OUTPUT);
  digitalWrite(WAK_PIN,LOW);
#endif

infoSerial.println(F("CCS811: Init"));

#if defined (TWI_SCL) && defined (TWI_SDA)
Wire.begin(TWI_SDA,TWI_SCL); //Inialize I2C Harware
#else
Wire.begin(); //Inialize I2C Harware
#endif

#ifdef I2C_CLOCK
Wire.setClock(I2C_CLOCK);
#endif

  //It is recommended to check return status on .begin(), but it is not
  //required.
  CCS811Core::status returnCode = ccs811.begin();
  //CCS811Core::CC811_Status_e returnCode = ccs811.beginWithStatus();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  //if (returnCode != CCS811Core::CCS811_Stat_SUCCESS)
  {
    errorSerial.print(F("CCS811: Init error "));
    printDriverError(returnCode);
    return 0;
  }
//ccs811.setBaseline(62000);
CCS811ready = true;

//returnCode = ccs811.setDriveMode(1);
//printDriverError(returnCode);
/*
delay(2000);Poll();
delay(2000);Poll();
delay(2000);Poll();
delay(2000); */
return 1;
}


int in_hdc1080::Setup()
{
//i2cReset();  
if (HDC1080ready)  {debugSerial<<F("HDC1080: Already initialized")<<endl; return 0;}
debugSerial.print(F("HDC1080: Init. "));
Wire.begin(); //Inialize I2C Harware
// Default settings:
// - Heater off
// - 14 bit Temperature and Humidity Measurement Resolutions
hdc1080.begin(0x40);
debugSerial.print(F("Manufacturer ID=0x"));
debugSerial.print(hdc1080.readManufacturerId(), HEX); // 0x5449 ID of Texas Instruments
debugSerial.print(F(" Device ID=0x"));
debugSerial.println(hdc1080.readDeviceId(), HEX); // 0x1050 ID of the device
printSerialNumber();
HDC1080ready = true;
return 1;
}



int in_hdc1080::Poll(short cause)
{
  float h,t;
  int reg;
if (cause!=POLLING_SLOW) return 0;
if (!HDC1080ready) {errorSerial<<F("HDC1080: Not initialized")<<endl; return 0;}
debugSerial.print(F("HDC1080: Status="));
debugSerial.print(reg=hdc1080.readRegister().rawData,HEX);
if (reg!=0xff)
{
  debugSerial.print(" T=");
  debugSerial.print(t=hdc1080.readTemperature());
  debugSerial.print(F("C, RH="));
  debugSerial.print(h=hdc1080.readHumidity());
  debugSerial.println("%");


  #ifdef M5STACK
    M5.Lcd.print(" T=");
  //Returns calculated CO2 reading
    M5.Lcd.print(t=hdc1080.readTemperature());
    M5.Lcd.print("C, RH=");
  //Returns calculated TVOC reading

    M5.Lcd.print(h=hdc1080.readHumidity());
    M5.Lcd.print("%\n");
  #endif

// New tyle unified activities
    aJsonObject *actT = aJson.getObjectItem(in->inputObj, "temp");
    aJsonObject *actH = aJson.getObjectItem(in->inputObj, "hum");
    if (!isnan(t)) executeCommand(actT,-1,itemCmd(t));
    if (!isnan(t)) executeCommand(actH,-1,itemCmd(h));
    
  publish(t,"/T");
  publish(h,"/H");
  if (CCS811ready) ccs811.setEnvironmentalData(h,t);
}
else //ESP I2C glitch
  {
    debugSerial.println();
    i2cReset();
  }
return INTERVAL_SLOW_POLLING;
}

int in_ccs811::Poll(short cause)
{
  if (!CCS811ready) {debugSerial<<F("ccs811 not initialized")<<endl; return 0;}
  #ifdef WAK_PIN
    digitalWrite(WAK_PIN,LOW);
  #endif
  delay(1);
//Check to see if data is ready with .dataAvailable()
  if (ccs811.dataAvailable())
//if (1)
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
    CCS811Core::status returnCode = ccs811.readAlgorithmResults();
    printDriverError(returnCode);
    float co2,tvoc;
    debugSerial.print(F(" CO2["));
    //Returns calculated CO2 reading
    debugSerial.print(co2 = ccs811.getCO2());
    debugSerial.print(F("] tVOC["));
    //Returns calculated TVOC reading

    debugSerial.print(tvoc = ccs811.getTVOC());
    debugSerial.print(F("] baseline["));
    debugSerial.print(ccs811Baseline = ccs811.getBaseline());

    #ifdef M5STACK
      M5.Lcd.print(" CO2[");
    //Returns calculated CO2 reading
      M5.Lcd.print(co2 = ccs811.getCO2());
      M5.Lcd.print("] tVOC[");
    //Returns calculated TVOC reading

      M5.Lcd.print(tvoc = ccs811.getTVOC());
      M5.Lcd.print("]\n");
    #endif


    if (co2<10000.) //Spontaneous calculation error suppress
    {

    // New tyle unified activities
    aJsonObject *actCO2 = aJson.getObjectItem(in->inputObj, "co2");
    aJsonObject *actTVOC = aJson.getObjectItem(in->inputObj, "tvoc");
    executeCommand(actCO2,-1,itemCmd(co2));
    executeCommand(actTVOC,-1,itemCmd(tvoc));

    publish(co2,"/CO2");
    publish(tvoc,"/TVOC");
    publish(ccs811Baseline,"/base");}
    debugSerial.println("]");
    printSensorError();

    #ifdef WAK_PIN
      digitalWrite(WAK_PIN,HIGH); //Relax some time
    #endif
  } else {debugSerial<<F("ccs811: data not available")<<endl; return 0;}
  return 1;
}

void in_hdc1080::printSerialNumber() {
infoSerial.print(F("Device Serial Number="));
HDC1080_SerialNumber sernum = hdc1080.readSerialNumber();
char format[16];
sprintf(format, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
infoSerial.println(format);
}

//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void in_ccs811::printDriverError( CCS811Core::status errorCode )
{
  debugSerial.print(F("CCS811: "));
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      debugSerial.print(F("SUCCESS"));
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      debugSerial.print(F("ID_ERROR"));
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      debugSerial.print(F("I2C_ERROR"));
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      debugSerial.print(F("INTERNAL_ERROR"));
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      debugSerial.print(F("GENERIC_ERROR"));
      break;
    default:
      debugSerial.print(F("Unspecified error."));
  }
}

//printSensorError gets, clears, then prints the errors
//saved within the error register.
void in_ccs811::printSensorError()
{
  uint8_t error = ccs811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    errorSerial.println(F("CCS811: Failed to get ERROR_ID register."));
  }
  else
  {
    if (error) errorSerial.print(F("CCS811: Error "));
    if (error & 1 << 5) errorSerial.print(F("HeaterSupply"));
    if (error & 1 << 4) errorSerial.print(F("HeaterFault"));
    if (error & 1 << 3) errorSerial.print(F("MaxResistance"));
    if (error & 1 << 2) errorSerial.print(F("MeasModeInvalid"));
    if (error & 1 << 1) errorSerial.print(F("ReadRegInvalid"));
    if (error & 1 << 0) errorSerial.print(F("MsgInvalid"));
    if (error) errorSerial.println();
  }
}
#endif
