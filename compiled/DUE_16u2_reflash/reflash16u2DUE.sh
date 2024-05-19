avrdude -c arduino -P /dev/cu.usbmodem144101 -b 19200 -p m16u2 -vvv -U flash:w:Arduino-usbserial.hex:i
