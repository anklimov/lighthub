#! /bin/bash
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_mega2560-net.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
