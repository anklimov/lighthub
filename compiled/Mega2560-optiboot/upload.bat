..\tools\win\tool-avrdude\avrdude -C ../tools/mac/tool-avrdude/avrdude.conf  -v -V -P com8 -patmega2560 -cwiring -b115200 -D -Uflash:w:firmware.hex:i
