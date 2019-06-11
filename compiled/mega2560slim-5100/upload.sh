../tools/mac/tool-avrdude/avrdude -C ../tools/mac/tool-avrdude/avrdude.conf -P /dev/cu.wchusbserial1450 -v -V  -patmega2560 -cwiring -b115200 -D -Uflash:w:firmware.hex:i
