# from time import time
#
# from SCons.Script import DefaultEnvironment
#
# print("==============================================Custom build flags are:=====================================================")
# #FLAGS="-MY_CONFIG_SERVER=192.168.10.110"
# #FLAGS+=" -WATCH_DOG_TICKER_DISABLE"
# #FLAGS+=" -USE_1W_PIN=12"
# #FLAGS+=" -SD_CARD_INSERTED"
# #FLAGS+=" -SERIAL_BAUD=115200"
# #FLAGS+=" -Wiz5500"
# #FLAGS+=" -DISABLE_FREERAM_PRINT"
# #FLAGS+=" -CUSTOM_FIRMWARE_MAC=C4:3E:1f:03:1B:1B"
# #FLAGS+=" -DMX_DISABLE"
# FLAGS="MODBUS_DISABLE"
# #FLAGS+=" -OWIRE_DISABLE"
# #FLAGS+=" -ARTNET_ENABLE"
# #FLAGS+=" -CONTROLLINO"
# #FLAGS+=" -AVR_DMXOUT_PIN=18"
#
# print(FLAGS)
# print("==============================================Custom build flags END=====================================================")
#
# env = DefaultEnvironment()
# env.Append(CPPDEFINES=['MODBUS_DISABLE=1'])

from time import time

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()
env.Append(CPPDEFINES=['BUILD_TIMESTAMP=%d' % time()])
