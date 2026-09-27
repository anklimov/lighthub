export PORT=cu.usbmodem21301
echo . |  stty -f /dev/$PORT speed 1200
../tools/mac/tool-bossac/bossac -U false -p $PORT -i -e -w -v -b firmware.bin -R 
