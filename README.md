# LightHub 
is Flexible, Arduino-Mega based SmartHome controller

It allow to connect together:

* Contact sensors (switches, buttons etc)
* 1-Wire temperature sensors (up to 20 on single bus)
* Standard nonexpensive Relay board with TTL inputs, (like this) to control AC powered lamps, floor heaters, boilers etc
* Standard LED and AC DMX-512 dimmer boards
* Modbus RTU devices (Currently, are deployed two types of Modbus devices: AC Dimmer and Ventilation set (Based on Vacon 10 controller)
* Simple DMX wall sensor panel

Where is possible both, to configure local control/mapping between inputs and outputs (light, floor heating thermostats) and remote control from Openhab or Openhab2 Smarthome software

Scalability is virtually unlimited: Setup so many controllers you needed in most convenient places of your house - MQTT broker will allow controllers communicate each other and with Openhab and propagate commands across network

Prease refer our Wiki for insructions.

Just started preparation to porting firmware to AVR based Arduino DUE and ESP8266/ESP32

Compiled image has been added to compiled/ folder
use 
avrdude  -v -V -patmega2560 -cwiring -b115200 -D -Uflash:w:lighthub.ino.hex:i
to flash your Mega 2560

# Dependences 
(quite big number of libs required. Use git clone to have your local copy in your Arduino libs folder)

For patched libraries, appropriate GitHub repo URL provided:

* Arduino-Temperature-Control-Library   https://github.com/anklimov/Arduino-Temperature-Control-Library
* DS2482_OneWire                        https://github.com/anklimov/DS2482_OneWire
* FastLED
* Wire (standard)
* Artnet
* DmxSimple                             https://github.com/anklimov/DmxSimple (for AVR) or https://github.com/anklimov/ESP-Dmx (for ESP) or https://github.com/anklimov/DmxDue (for DUE)
* HTTPClient                            https://github.com/anklimov/HTTPClient
* aJson                                 https://github.com/anklimov/aJson
* CmdArduino                            https://github.com/anklimov/CmdArduino
* EEPROM (standard for AVR) or DueFlashStorage for DUE
* ModbusMaster                          https://github.com/anklimov/ModbusMaster
* pubsubclient-2.6
* DMXSerial-master (for AVR)            https://github.com/anklimov/DMXSerial
* Ethernet                              https://github.com/anklimov/Ethernet
* SPI (standard)
