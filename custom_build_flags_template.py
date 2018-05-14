# usage:
# first make your own copy of template
# cp custom_build_flags_template.py my_custom_build_flags.py
# then edit, change or comment something

import os
print("==============================================Custom build flags are:=====================================================")
FLAGS="-DMY_CONFIG_SERVER=lazyhome.ru"
FLAGS+=" -DWATCH_DOG_TICKER_DISABLE"
FLAGS+=" -DUSE_1W_PIN=12"
FLAGS+=" -DSD_CARD_INSERTED"
FLAGS+=" -DSERIAL_BAUD=115200"
FLAGS+=" -DWiz5500"
FLAGS+=" -DDISABLE_FREERAM_PRINT"
FLAGS+=" -DCUSTOM_FIRMWARE_MAC=de:ad:be:ef:fe:00"
FLAGS+=" -DDMX_DISABLE"
FLAGS+=" -DMODBUS_DISABLE"
FLAGS+=" -DOWIRE_DISABLE"
FLAGS+=" -DARTNET_ENABLE"
FLAGS+=" -DCONTROLLINO"
FLAGS+=" -DAVR_DMXOUT_PIN=18"

print(FLAGS)
print("==============================================Custom build flags END=====================================================")
os.environ["PLATFORMIO_BUILD_FLAGS"] = FLAGS