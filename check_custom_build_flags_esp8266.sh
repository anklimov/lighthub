#! /bin/bash
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_esp8266.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
