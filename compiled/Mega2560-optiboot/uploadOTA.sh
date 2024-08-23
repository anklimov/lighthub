#!/bin/sh
../tools/mac/arduinoOTA -address 192.168.11.10 -port 80 -username arduino -password password -sketch firmware.bin -b -upload /sketch
