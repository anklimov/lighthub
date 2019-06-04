#! /bin/bash
CUSTOM_BUILD_FLAGS_FILE=custom-build-flags/build_flags_mega2560.sh
export FLAGS="$FLAGS -DPIO_SRC_REV="$(git log --pretty=format:%h_%ad -1 --date=short)
if [ -f $CUSTOM_BUILD_FLAGS_FILE ]; then
    source $CUSTOM_BUILD_FLAGS_FILE
fi
 echo $FLAGS
