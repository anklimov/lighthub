export PORT=cu.usbmodem144101
echo . |  stty -f /dev/$PORT speed 1200
../tools/mac/tool-bossac/bossac  -p $PORT -i  -w -v -b firmware.bin -R 