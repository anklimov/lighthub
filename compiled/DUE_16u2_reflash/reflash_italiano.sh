#avrdude should be installed via homebrew
#avrdude -C ../tools/mac/tool-avrdude/avrdude.conf -c arduino -P /dev/cu.usbmodem144101 -b 19200 -p m16u2 -vvv -U flash:w:16u2-original.hex:i
avrdude  -c arduino -P /dev/cu.usbmodem144101 -b 19200 -p m16u2 -vvv -U flash:w:16u2-italiano.hex:i
