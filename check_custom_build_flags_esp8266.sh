#! /bin/bash
export FLAGS="$FLAGS -DMODBUS_DISABLE"
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_esp8266.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
export FLAGS="$FLAGS -DDMX_DISABLE"
export FLAGS="$FLAGS -DMODBUS_DISABLE"
 echo $FLAGS
