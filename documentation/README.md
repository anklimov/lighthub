# Документация LightHub

Мастер-индекс документации репозитория. Язык файла указан в колонке «Я» (RU/EN).
Сверка с кодом выполнена на дату 2026-10-02 (v4.5.2+); эталоном служит исходный код,
за ним — [снапшот официальной Wiki](reference/dokuwiki-2026-10-02/README.md).

## core/ — протокол, конфигурация, команды

| Файл | Я | Описание |
|--|--|--|
| [core\_concepts.md](core/core_concepts.md) | RU | **Основные принципы**: базовые сущности — item (канал), itemCmd (унифицированная команда), execObj (объект действия) |
| [light\_hub\_полное\_инженерное\_описание\_json\_конфигурации\_v2.md](core/light_hub_полное_инженерное_описание_json_конфигурации_v2.md) | RU | Полное описание JSON-конфигурации: секции, 24 типа каналов (0-23), примеры |
| [channel\_types\_reference.md](core/channel_types_reference.md) | RU | Справочник типов каналов: коды, форматы настройки, команды, доступность по целям |
| [technical\_channel\_types\_table.md](core/technical_channel_types_table.md) | RU | Техническая таблица типов каналов (формат item.h, суффиксы, матрица) |
| [suffixes\_reference.md](core/suffixes_reference.md) | RU | Справочник MQTT-суффиксов и команд, таблицы по типам каналов |
| [mqtt\_api\_reference.md](core/mqtt_api_reference.md) | RU | Полный справочник MQTT: топики, команды, HTTP API, $command/$stats, homie |
| [mqtt\_quick\_reference.md](core/mqtt_quick_reference.md) | RU | Быстрая шпаргалка по MQTT и HTTP |
| [configuration\_examples.md](core/configuration_examples.md) | RU | Примеры JSON-конфигураций по всем типам каналов |
| [modules\_real\_config.md](core/modules_real_config.md) | RU | Реальные конфигурации по драйверам |
| [modules\_description.md](core/modules_description.md) | RU | Описание драйверов (out\_*, in\_*) с параметрами getConfig() |
| [cli\_interface.md](core/cli_interface.md) | RU | CLI-интерфейс (USB-сериал) |
| [web\_ui.md](core/web_ui.md) | RU | Web-интерфейс и HTTP API настройки |
| [config\_file\_workflow.md](core/config_file_workflow.md) | RU | Как контроллер загружает/сохраняет файл конфигурации |
| [button\_presses.md](core/button_presses.md) | RU | Двойные/тройные/длительные нажатия, несколько кнопок на одном входе |
| [rotary\_encoder.md](core/rotary_encoder.md) | RU | Rotary encoder (T:32): профили, примеры |
| [analog\_inputs\_and\_map.md](core/analog_inputs_and_map.md) | RU | Аналоговые входы, преобразования `map` |
| [command\_signature.md](core/command_signature.md) | RU | Криптоподпись команд (SHA256, protected pins) |
| [ota.md](core/ota.md) | RU | OTA-обновление прошивки по воздуху |

## integrations/ — внешние системы

| Файл | Я | Описание |
|--|--|--|
| [can\_lightspot.md](integrations/can_lightspot.md) | RU | CAN-шина, lightspot, протокол datagram, шлюз |
| [modbus\_setup.md](integrations/modbus_setup.md) | RU | Настройка Modbus (v2), режим шлюза IP-Modbus |
| [1wire\_sensors.md](integrations/1wire_sensors.md) | RU | Подключение 1-wire датчиков |
| [homeassistant.md](integrations/homeassistant.md) | RU | Интеграция с Home Assistant |
| [openhab.md](integrations/openhab.md) | RU | Интеграция с OpenHAB |
| [work\_remotely.md](integrations/work_remotely.md) | RU | Работа дистанционно (cloud-брокеры) |

## modules/ — функциональные модули

| Файл | Я | Описание |
|--|--|--|
| [multivent\_module\_description.md](modules/multivent_module_description.md) | RU | Многозональная вентиляция/кондиционирование (CH 18) |
| [Sprinkler\_module.md](modules/Sprinkler_module.md) | RU | Контроллер полива (CH 23): состояния, команды, FA |
| [Sprinkler\_HomeAssistant.md](modules/Sprinkler_HomeAssistant.md) | RU | HA-дашборд для поливального модуля |

## hardware/ — железо

| Файл | Я | Описание |
|--|--|--|
| [hardware\_platforms.md](hardware/hardware_platforms.md) | RU | LightHub на разных платах/чипах |
| [pins\_io.md](hardware/pins_io.md) | RU | Карта входов/выходов платы LightHub (rev 2.0 / 2.2) |
| [mac\_address.md](hardware/mac_address.md) | RU | MAC-адрес контроллера: генерация, замена |
| [flashing.md](hardware/flashing.md) | RU | Заливка прошивки, список целей |

## reference/ — справочные материалы

- [dokuwiki-2026-10-02/](reference/dokuwiki-2026-10-02/README.md) — замороженный снапшот официальной Wiki (источник истины уровня 2)
- [config\_samples/](reference/config_samples/) — эталонные JSON-конфигурации
- [images/](reference/images/) — иллюстрации

## Прочее

- [../CHANGELOG.md](../CHANGELOG.md) (EN) — история изменений
- [../../README.md](../README.md) (EN) — общий README проекта
