import os
Import("env")

script = env.GetProjectOption("_upload_command")
spath  = os.path.abspath(script)
#env.Replace(
#    UPLOADER="executable or path to executable",
#    UPLOADCMD=script
#)
env.AddCustomTarget(
    "ota",
    "$BUILD_DIR/${PROGNAME}.bin",
    spath)
