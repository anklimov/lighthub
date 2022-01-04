@ECHO off

REM Wait X second for memory on Arduino Due is erased.
SET WAIT_ERASED=4

ECHO ------ External tool BossacArduinoDue started ------

REM number of command line arguments ok?
REM IF [%1]==[] GOTO error_args
REM IF [%2]==[] GOTO error_args

REM set command line arguments
SET BOSSACPATH=..\tools\win\tool-bossac\bossac.exe
SET BINFILE=firmware.bin

REM parse command line arguments
SET BOSSACPATH=%BOSSACPATH:"=%
SET BINFILE=%BINFILE:"=%

REM workeround for bug in Atmel Studio 6.0.1996 Service Pack 2
SET BINFILE=%BINFILE:\\=\%
SET BINFILE=%BINFILE:.cproj=%

REM bossac path exist?
IF NOT EXIST "%BOSSACPATH%" GOTO error_arg1

REM bin file exist?
IF NOT EXIST "%BINFILE%" GOTO error_binfile

REM fetch DeviceID of Arduino Due Programming Port from WMI Service
FOR /f "tokens=* skip=1" %%a IN ('wmic PATH Win32_SerialPort Where "Caption LIKE '%%USB%%'" get DeviceID') DO (
    SET COMX=%%a
    GOTO exit1
)

REM Arduino Due Programming Port not exist
GOTO error_comport

:exit1

REM remove blank
SET COMPORT=%COMX: =%

REM report in Atmel Studio 6.0 IDE output window
ECHO BossacPath=%BOSSACPATH%
ECHO BinFile=%BINFILE%
ECHO Arduino Due Programming Port is detected as %COMPORT%.

REM The bossac bootloader only runs if the memory on Arduino Due is erased.
REM The Arduino IDE does this by opening and closing the COM port at 1200 baud.
REM This causes the Due to execute a soft erase command.
ECHO Forcing reset using 1200bps open/close on port 
ECHO MODE %COMPORT%:1200,N,8,1
MODE %COMPORT%:1200,N,8,1

REM Wait X second for memory on Arduino Due is erased.
ECHO Wait for memory on Arduino Due is erased...
PING -n %WAIT_ERASED% 127.0.0.1>NUL

REM Execute bossac.exe
ECHO Execute bossac with command line:
ECHO "%BOSSACPATH%" -i -d --port=%COMPORT% -U false -e -w -v -b "%BINFILE%" -R
START /WAIT "" "%BOSSACPATH%" -i  --port=%COMPORT% -U false -e -w -v -b "%BINFILE%" -R

GOTO end

:error_args
ECHO Error: wrong number of command line arguments passed!
GOTO end

:error_arg1
ECHO Error: command line argument 1 - path to bossac.exe not exist! - "C:\Program Files (x86)\arduino-1.5.2\hardware\tools\bossac.exe"
ECHO Error: command line argument 1 - argument passed = %1
GOTO end

:error_arg2
ECHO Error: command line argument 2 - path to bin file not exist! - use $(OutputDirectory)\$(OutputFileName).bin
ECHO Error: command line argument 2 - argument passed = %1
GOTO end

:error_binfile
ECHO Error: bin file "%BINFILE%" not exist!
GOTO end

:error_comport
ECHO Error: Arduino Due Programming Port not found!

:end

ECHO ======================== Done ========================