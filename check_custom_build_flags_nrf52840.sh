#! /bin/bash
export FLAGS="$FLAGS -DWiz5500"
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_nrf52840.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
