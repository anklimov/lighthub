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

Compiled image has been added to compiled/ folder
use 
avrdude  -v -V -patmega2560 -cwiring -b115200 -D -Uflash:w:lighthub.ino.hex:i
to flash your Mega 2560
