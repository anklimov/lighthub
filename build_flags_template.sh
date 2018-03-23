#! /bin/bash
# usage:
# first make your own copy of template
# cp build_flags_template.sh my_build_flags.sh
# then edit, change or comment something
# nano  my_build_flags.sh
# and source it
# source my_build_flags.sh
 export FLAGS="-DMY_CONFIG_SERVER=lazyhome.ru"
 export FLAGS="$FLAGS -DWATCH_DOG_TICKER_DISABLE"
 export FLAGS="$FLAGS -DUSE_1W_PIN=12"
 export FLAGS="$FLAGS -DSD_CARD_INSERTED"
 export FLAGS="$FLAGS -DSERIAL_BAUD=115200"
 export FLAGS="$FLAGS -DWiz5500"
 export FLAGS="$FLAGS -DDISABLE_FREERAM_PRINT"
 export FLAGS="$FLAGS -DCUSTOM_FIRMWARE_MAC=de:ad:be:ef:fe:00"
 export PLATFORMIO_BUILD_FLAGS="$FLAGS"
 unset FLAGS