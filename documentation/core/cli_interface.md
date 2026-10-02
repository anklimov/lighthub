# Command Line Interface

> Источник: Wiki LightHub (lazyhome.ru), страница «cli_интерфейс». Снапшот от 2026-10-02.

Иначе, интерфейс командной строки.

Служит для настройки системных параметров контроллера (при необходимости)
Команды можно подать как подключившись к контроллеру на отладочный последовательный порт, так и при помощи MQTT (см [работа с MQTT, служебные топики](https://www.lazyhome.ru/dokuwiki/doku.php?id=работа_с_mqtt#служебные_топики)) или HTTP [API](https://www.lazyhome.ru/dokuwiki/doku.php?id=api)

## Перечень команд

  * `help` - this text
  * `mac de:ad:be:ef:fe:00` set and store MAC-address in EEPROM
  * `ip [ip[,dns[,gw[,subnet]]]]` - set static IP
  * `save` - save current config in NVRAM; ON|OFF - enable/disable autosave
  * `get` [config addr]' - get config from pre-configured URL and store addr, ON|OFF - enable/disable download on startup
  * `load` - load config from NVRAM
  * `pwd` - define and store MQTT password
  * `otapwd` - define and store HTTP API password
  * `log [serial_loglevel] [udp_loglevel]` - define log level (0..7)
  * `kill` - test watchdog - в норме, приводит к перезапуску
  * `clear` - clear EEPROM
  * `search` - search devices on 1-wire bus
  * `reboot` - reboot controller

## Подключение по USB

### MAC

Подключаем контроллер USB кабелем к ноутбуку. Открываем эмулятор терминала (например для Макбука использую minicom, который устанавливается через homebrew).
Для улучшения читаемости ответов контроллера в minicom нажимаем комбинацию Esc+U

```
$ minicom -D /dev/tty.usbmodem14201
```

### Windows

Для windows можно использовать putty, терминал, входящий в среду разработки PlatformIO, Arduino IDE

Параметры подключения - 115200 бод, N-8-1

CLI commands must be send via serial port with 115200 baud rate / 8N1 params: 8 data bits, no parity bit, 1 stop bit.

### Пример исполненения команды при помощи Unix shell:

```
> stty -f /dev/cu.usbmodem146101 ispeed 115200 ospeed 115200 -parenb cs8 cstopb
> echo "ip" > /dev/cu.usbmodem146101
> cat /dev/cu.usbmodem146101
```

### также, можно использовать Arduino IDE

(serial monitor). Serial port must be configured (MacOSX config file ~/Library/Arduino15/preferences.txt)

```
serial.databits=8
serial.stopbits=1
serial.parity=N
serial.debug_rate=115200
serial.line_ending=1
```

## Подключение по MQTT

Если контроллер уже успешно подключен к MQTT брокеру, то команду CLI можно направить в топик $command.
см [работа с MQTT, служебные топики](https://www.lazyhome.ru/dokuwiki/doku.php?id=работа_с_mqtt#служебные_топики)

## Подключение по HTTP API

Если контроллер уже успешно подключен к локальной сети, то команду CLI можно направить с помощью http [API](https://www.lazyhome.ru/dokuwiki/doku.php?id=api)

Примечание (2026-10-02): список команд wiki полностью совпадает с реализацией в `lighthub/main.cpp`, функция `setupCmdArduino()` (строки 2854–2876): зарегистрированы `help`, `save`, `load`, `get`, `mac`, `ip`, `otapwd`, `kill`, `pwd`, `clear`, `reboot`, `log`, `search`. Отдельные команды регистрируются условно: `get`, `mac`, `ip`, `otapwd` — только при включенной сети (не `NOIP`), `mac` — кроме сборок `FLASH_64KB`, `search` — кроме сборок с `OWIRE_DISABLE`. В коде есть также закомментированная команда `req` (`cmdFunctionReq`), в wiki она не упомянута. В wiki опечатка в описании `get` (`'get' [config addr]'`), в коде команда принимает URL сервера конфига либо `ON`/`OFF` (`cmdFunctionGet`, main.cpp:1772).

## Сверка с кодом (2026-10-02)

- Корректно: все 13 команд из wiki присутствуют в коде (`lighthub/main.cpp:2854-2876`).
- Уточнение: часть команд (`get`, `mac`, `ip`, `otapwd`) доступна только в сборках с сетевым интерфейсом; `search` — только при включенной поддержке 1-wire.
