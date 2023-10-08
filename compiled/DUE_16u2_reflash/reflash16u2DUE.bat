REM fetch DeviceID of Arduino Port from WMI Service
FOR /f "tokens=* skip=1" %%a IN ('wmic PATH Win32_SerialPort Where "Caption LIKE '%%Arduino Uno%%'" get DeviceID') DO (
    SET COMX=%%a
    GOTO exit1
)

REM Arduino Due Programming Port not exist
GOTO error_comport

:exit1

REM remove blank
SET COMPORT=%COMX: =%


..\tools\win\tool-avrdude\avrdude -C ..\tools\win\tool-avrdude\avrdude.conf -c arduino -P %COMPORT% -b 19200 -p m16u2 -vvv -U flash:w:16u2.hex:i
