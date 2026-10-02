# Работаем дистанционно

> Источник: Wiki LightHub (lazyhome.ru), страница «работаем_дистанционно». Снапшот от 2026-10-02.

## ОТА прошивка

для Windows из папки C:\Program Files (x86)\Arduino\hardware\tools\avr\bin копируем arduinoOTA.exe
в папку с прошивкой (firmware.bin).

Создаем командный файл и указываем адрес вашего контроллера

```
arduinoOTA.exe -address 192.168.XXX.XXX -port 80 -username arduino -password password -sketch firmware.bin -b -upload /sketch
```

Запускаем!

для Mac

```
arduinoOTA -address 192.168.XXX.XXX -port 80 -username arduino -password password -sketch firmware.bin -b -upload /sketch
```

password - значение по-умолчанию. Его можно заменить при помощи CLI команды otapwd

## Работа с SYSLOG

Для Windows устанавливаем Visual Syslog Сервер для Windows [https://github.com/MaxBelkov/visualsyslog/blob/master/readme_rus.md](https://github.com/MaxBelkov/visualsyslog/blob/master/readme_rus.md)

В корне конфигурационного файлa Lighthub указываем:

```json
"syslog":["IP"]
```

где "IP" - адрес сервера Syslog

## Выполнение CLI команд через MQTT

Достаточно в топик myhome/имя_контроллера/$command послать нужную команду [CLI интерфейс](../core/cli_interface.md)

Также, можно воспользоваться http [API](https://www.lazyhome.ru/dokuwiki/doku.php?id=api)

## Сверка с кодом (2026-10-02)

- CLI-команда `otapwd` подтверждена: `cmdAdd("otapwd", cmdFunctionOTAPwd)` (main.cpp:2863); о подстановом пароле HTTP/OTA API см. main.cpp:562.
- Топик `$command` для отправки CLI-команд по MQTT подтвержден: `#define CMDTOPIC "$command"` (options.h:200).
- Корневой раздел `"syslog"` (массив адресов серверов) подтвержден: `udpSyslogArr = aJson.getObjectItem(root, "syslog")` (main.cpp:588).
- Отдельных cloud-брокеров (AWS IoT, MQTT over TLS и т.п.) в прошивке не выявлено — удаленный доступ обеспечивается внешним MQTT-брокером и/или VPN, как описано на странице.
