../tools/mac/tool-avrdude/avrdude -C ../tools/mac/tool-avrdude/avrdude.conf -v -V -P /dev/cu.usbmodem1411 -patmega2560 -cwiring -b115200 -D -Uflash:w:firmware.hex:i
