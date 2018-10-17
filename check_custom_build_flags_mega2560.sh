#! /bin/bash
CUSTOM_BUILD_FLAGS_FILE="custom-build-flags/build_flags_mega2560.sh"
#CUSTOM_BUILD_FLAGS_FILE="custom-build-flags/build_flags_$PIOENV.sh"
#echo $PIOENV > custom-build-flags/1.txt
#TODO: make one file for all envs
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
