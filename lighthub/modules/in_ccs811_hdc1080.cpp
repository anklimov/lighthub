#include "modules/in_ccs811_hdc1080.h"
#include "Arduino.h"

#ifndef CSSHDC_DISABLE

CCS811 ccs811(CCS811_ADDR);
ClosedCube_HDC1080 hdc1080;
long ccs811Baseline;

bool CCS811ready = false;
bool HDC1080ready = false;

int  in_ccs811::Setup(int addr)
{
#ifdef WAK_PIN
  pinMode(WAK_PIN,OUTPUT);
  digitalWrite(WAK_PIN,LOW);
#endif

Serial.println("CCS811 Init");
if (CCS811ready) return 0;
Wire.begin(); //Inialize I2C Harware

  //It is recommended to check return status on .begin(), but it is not
  //required.
  CCS811Core::status returnCode = ccs811.begin();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  {
    Serial.println("CCS811 Init error");
    printDriverError(returnCode);
    return 0;
  }
ccs811.setBaseline(62000);
CCS811ready = true;
return 1;
}

int in_hdc1080::Setup(int addr)
{
if (HDC1080ready) return 0;
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
#if defined (ARDUINO_ARCH_ESP8266)
SCL_LOW();
delay(300);
SCL_HIGH();
#endif
}

int in_hdc1080::Poll()
{
  float h,t;
  int reg;
if (!HDC1080ready) return 0;
Serial.print("HDC Status=");
Serial.println(reg=hdc1080.readRegister().rawData,HEX);
if (reg!=0xff)
{
  Serial.print("T=");
  Serial.print(t=hdc1080.readTemperature());
  Serial.print("C, RH=");
  Serial.print(h=hdc1080.readHumidity());
  Serial.print("%");
  publish(t,"/T");
  publish(h,"/H");
  ccs811.setEnvironmentalData(h,t);
}
else //ESP I2C glitch
  {
    Serial.println("I2C Reset");
    i2cReset();
  }
return 1;
}

int in_ccs811::Poll()
{
  if (!CCS811ready) return 0;
  #ifdef WAK_PIN
    digitalWrite(WAK_PIN,LOW);
  #endif
//Check to see if data is ready with .dataAvailable()
  if (ccs811.dataAvailable())
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

    publish(co2,"/CO2");
    publish(tvoc,"/TVOC");
    publish(ccs811Baseline,"/base");

    Serial.print("] millis[");
    //Simply the time since program start
    Serial.print(millis());
    Serial.print("]");
    Serial.println();
    printSensorError();

    #ifdef WAK_PIN
      digitalWrite(WAK_PIN,HIGH); //Relax some time
    #endif
  }
  return 1;
}

void in_hdc1080::printSerialNumber() {
Serial.print("Device Serial Number=");
HDC1080_SerialNumber sernum = hdc1080.readSerialNumber();
char format[12];
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
