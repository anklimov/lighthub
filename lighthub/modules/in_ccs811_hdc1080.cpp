#include "modules/in_ccs811_hdc1080.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"
#include "item.h"
#include "main.h"

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
  if (CCS811ready) {debugSerial<<F("ccs811 is already initialized")<<endl; return 0;}

#ifdef WAK_PIN
  pinMode(WAK_PIN,OUTPUT);
  digitalWrite(WAK_PIN,LOW);
#endif

Serial.println("CCS811 Init");

Wire.begin(); //Inialize I2C Harware
Wire.setClock(4000);

  //It is recommended to check return status on .begin(), but it is not
  //required.
  CCS811Core::status returnCode = ccs811.begin();
  //CCS811Core::CC811_Status_e returnCode = ccs811.beginWithStatus();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  //if (returnCode != CCS811Core::CCS811_Stat_SUCCESS)
  {
    Serial.print("CCS811 Init error ");
    //Serial.println(ccs811.statusString(returnCode));
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
if (HDC1080ready)  {debugSerial<<F("hdc1080 is already initialized")<<endl; return 0;}
Serial.println("HDC1080 Init ");
Wire.begin(); //Inialize I2C Harware
// Default settings:
// - Heater off
// - 14 bit Temperature and Humidity Measurement Resolutions
hdc1080.begin(0x40);
Serial.print("Manufacturer ID=0x");
Serial.println(hdc1080.readManufacturerId(), HEX); // 0x5449 ID of Texas Instruments
Serial.print("Device ID=0x");
Serial.println(hdc1080.readDeviceId(), HEX); // 0x1050 ID of the device
printSerialNumber();
HDC1080ready = true;
return 1;
}

void i2cReset(){
Wire.endTransmission(true);
#if defined (SCL_RESET)
SCL_LOW();
delay(300);
SCL_HIGH();
#endif
}

int in_hdc1080::Poll(short cause)
{
  float h,t;
  int reg;
if (cause!=POLLING_SLOW) return 0;
if (!HDC1080ready) {debugSerial<<F("HDC1080 not initialized")<<endl; return 0;}
Serial.print("HDC Status=");
Serial.println(reg=hdc1080.readRegister().rawData,HEX);
if (reg!=0xff)
{
  Serial.print(" T=");
  Serial.print(t=hdc1080.readTemperature());
  Serial.print("C, RH=");
  Serial.print(h=hdc1080.readHumidity());
  Serial.println("%");


  #ifdef M5STACK
    M5.Lcd.print(" T=");
  //Returns calculated CO2 reading
    M5.Lcd.print(t=hdc1080.readTemperature());
    M5.Lcd.print("C, RH=");
  //Returns calculated TVOC reading

    M5.Lcd.print(h=hdc1080.readHumidity());
    M5.Lcd.print("%\n");
  #endif

  publish(t,"/T");
  publish(h,"/H");
  if (CCS811ready) ccs811.setEnvironmentalData(h,t);
}
else //ESP I2C glitch
  {
    Serial.println("I2C Reset");
    i2cReset();
  }
return INTERVAL_POLLING;
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
    Serial.print(" CO2[");
    //Returns calculated CO2 reading
    Serial.print(co2 = ccs811.getCO2());
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading

    Serial.print(tvoc = ccs811.getTVOC());
    Serial.print("] baseline[");
    Serial.print(ccs811Baseline = ccs811.getBaseline());

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
    publish(co2,"/CO2");
    publish(tvoc,"/TVOC");
    publish(ccs811Baseline,"/base");}
    Serial.println("]");
    printSensorError();

    #ifdef WAK_PIN
      digitalWrite(WAK_PIN,HIGH); //Relax some time
    #endif
  } else {debugSerial<<F("ccs811: data not available")<<endl; return 0;}
  return 1;
}

void in_hdc1080::printSerialNumber() {
Serial.print("Device Serial Number=");
HDC1080_SerialNumber sernum = hdc1080.readSerialNumber();
char format[16];
sprintf(format, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
Serial.println(format);
}

//printDriverError decodes the CCS811Core::status type and prints the
//type of error to the serial terminal.
//
//Save the return value of any function of type CCS811Core::status, then pass
//to this function to see what the output was.
void in_ccs811::printDriverError( CCS811Core::status errorCode )
{
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}

//printSensorError gets, clears, then prints the errors
//saved within the error register.
void in_ccs811::printSensorError()
{
  uint8_t error = ccs811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    //Serial.print("");
    if (error & 1 << 5) Serial.print("Error: HeaterSupply");
    if (error & 1 << 4) Serial.print("Error: HeaterFault");
    if (error & 1 << 3) Serial.print("Error: MaxResistance");
    if (error & 1 << 2) Serial.print("Error: MeasModeInvalid");
    if (error & 1 << 1) Serial.print("Error: ReadRegInvalid");
    if (error & 1 << 0) Serial.print("Error: MsgInvalid");
    Serial.println();
  }
}
#endif
