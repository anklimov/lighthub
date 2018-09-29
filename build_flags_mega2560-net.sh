#! /bin/bash
# usage:
# first make your own copy of template
# cp build_flags_template.sh build_flags_ENVNAME.sh
# then edit, change or comment something
 export FLAGS="-DMY_CONFIG_SERVER=lazyhome.ru"
 #export FLAGS="$FLAGS -DWATCH_DOG_TICKER_DISABLE"
 #export FLAGS="$FLAGS -DUSE_1W_PIN=12"
 #export FLAGS="$FLAGS -DSD_CARD_INSERTED"
 export FLAGS="$FLAGS -DSERIAL_BAUD=115200"
 #export FLAGS="$FLAGS -DWiz5500"
 #export FLAGS="$FLAGS -DDISABLE_FREERAM_PRINT"
 export FLAGS="$FLAGS -DCUSTOM_FIRMWARE_MAC=de:ad:be:ef:fe:fe"
# export FLAGS="$FLAGS -DDMX_DISABLE"
# export FLAGS="$FLAGS -DARTNET_ENABLE"
# export FLAGS="$FLAGS -DMODBUS_DISABLE"
# export FLAGS="$FLAGS -DOWIRE_DISABLE"
# export FLAGS="$FLAGS -DAVR_DMXOUT_PIN=18"
# export FLAGS="$FLAGS -DLAN_INIT_DELAY=2000"
# export FLAGS="$FLAGS -DCONTROLLINO"
# export FLAGS="$FLAGS -DESP_WIFI_AP=MYAP"
# export FLAGS="$FLAGS -DESP_WIFI_PWD=MYPWD"
# export FLAGS="$FLAGS -DWIFI_MANAGER_DISABLE"
 export FLAGS="$FLAGS -DDHT_DISABLE"
# export FLAGS="$FLAGS -DRESET_PIN=5"
# export FLAGS="$FLAGS -DDHCP_RETRY_INTERVAL=60000"
# export FLAGS="$FLAGS -DRESTART_LAN_ON_MQTT_ERRORS"
# export FLAGS="$FLAGS -DW5500_CS_PIN=53"
 export FLAGS="$FLAGS -DPIO_SRC_REV="$(git log --pretty=format:%h_%ad -1 --date=short)
 echo $FLAGS
