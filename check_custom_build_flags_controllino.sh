#! /bin/bash
export FLAGS="$FLAGS -DCONTROLLINO"
export FLAGS="$FLAGS -DCUSTOM_FIRMWARE_MAC=de:ad:be:ef:fe:07"
export FLAGS="$FLAGS -DPIO_SRC_REV="$(git log --pretty=format:%h_%ad -1 --date=short)
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_controllino.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
