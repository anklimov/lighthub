#! /bin/bash
export FLAGS="$FLAGS -DWiz5500"
export FLAGS="$FLAGS -DARTNET_ENABLE"
export FLAGS="$FLAGS -DPIO_SRC_REV="$(git log --pretty=format:%h_%ad -1 --date=short)
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_due-5500.sh
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
