#! /bin/bash
# usage:
# first make your own copy of template
# cp build_flags_template.sh my_build_flags.sh
# then edit, change or comment something
# nano  my_build_flags.sh
# and source it
# source my_build_flags.sh
 #export FLAGS="-DMY_CONFIG_SERVER=lighthub.elistech.ru"
 #export FLAGS="$FLAGS -DWATCH_DOG_TICKER_DISABLE"
 #export FLAGS="$FLAGS -DUSE_1W_PIN=12"
 #export FLAGS="$FLAGS -DSD_CARD_INSERTED"
 export FLAGS="$FLAGS -DSERIAL_BAUD=115200"
# export FLAGS="$FLAGS -DWiz5500"
 export FLAGS="$FLAGS -DDISABLE_FREERAM_PRINT"
 export FLAGS="$FLAGS -DCUSTOM_FIRMWARE_MAC=C2:3E:1f:03:1B:1E"
# export FLAGS="$FLAGS -DDMX_DISABLE"
# export FLAGS="$FLAGS -DMODBUS_DISABLE"
# export FLAGS="$FLAGS -DOWIRE_DISABLE"
 #export FLAGS="$FLAGS -DAVR_DMXOUT_PIN=18"
 #export FLAGS="$FLAGS -DCONTROLLINO"
# export FLAGS="$FLAGS -DRESET_PIN=8"
# export FLAGS="$FLAGS -DLAN_INIT_DELAY=2000"
# export FLAGS="$FLAGS -DESP_WIFI_AP=vent"
# export FLAGS="$FLAGS -DESP_WIFI_PWD=kk007remont"
# export FLAGS="$FLAGS -DSYSLOG_ENABLE"
 export FLAGS="$FLAGS -DDEVICE_NAME=kk007_mega2560"
export FLAGS="$FLAGS -DDHT_COUNTER_DISABLE"
#export FLAGS="$FLAGS -DWITH_DOMOTICZ"
#export FLAGS="$FLAGS -DWITH_PRINTEX_LIB"
 export FLAGS="$FLAGS -DPIO_SRC_REV="$(git log --pretty=format:%h_%ad -1 --date=short)
