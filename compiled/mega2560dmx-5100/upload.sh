#!/bin/bash
echo $@
killall avrdude
echo Reset..
#/usr/bin/curl "http://192.168.88.31/console/reset"
echo AvrDude..
#/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude -Pnet:192.168.88.31:23 $@
/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude -Pnet:192.168.88.2:23000 $@


#/usr/bin/curl "http://192.168.8.94/console/reset"
#echo AvrDude..
#/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude -Pnet:192.168.8.94:23 $@
