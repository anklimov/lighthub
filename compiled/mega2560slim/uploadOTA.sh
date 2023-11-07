../tools/mac/tool-avrdude/avrdude -C ../tools/mac/tool-avrdude/avrdude.conf -P net:192.168.88.2:23000 -v -V  -patmega2560 -cwiring -b115200 -D -Uflash:w:firmware.hex:i
