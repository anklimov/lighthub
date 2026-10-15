# Примеры конфигураций LightHub

Готовые JSON-конфигурации для типовых кейсов из документации. Имя файла
отражает кейс. Каждый пример — законченный конфиг: адаптируйте пины,
адреса и топики под своё оборудование и схему подключения.

Все примеры проверены на синтаксическую корректность JSON и ссылочную
целостность (члены групп и `item`-ссылки существуют).

## Свет и реле

| Файл | Кейс | Документация |
|------|------|--------------|
| [relay\_gpio\_switch.json](relay_gpio_switch.json) | Реле на GPIO: насос, бойлер, вентилятор; управление кнопкой | [Типы каналов](../documentation/core/channel_types_reference.md) |
| [dimmer\_pwm\_gpio.json](dimmer_pwm_gpio.json) | PWM-диммер света и скорости вентилятора, группа каналов | [CH\_PWM](../documentation/core/technical_channel_types_table.md) |
| [rgbw\_led\_strip.json](rgbw_led_strip.json) | RGBW-лента (DMX RGBW, 4 канала) | [CH\_RGBW](../documentation/core/technical_channel_types_table.md) |
| [led\_spiled\_neopixel.json](led_spiled_neopixel.json) | Адресуемая NeoPixel-лента, управление диапазоном пикселей (10-20) | [CH\_SPILED](../documentation/core/technical_channel_types_table.md) |
| [group\_lighting\_scene.json](group_lighting_scene.json) | Группировка светильников, пересекающиеся группы (сцены) | [CH\_GROUP](../documentation/core/technical_channel_types_table.md) |
| [lighting\_dmx\_full\_house.json](lighting_dmx_full_house.json) | Большой дом: DMX-освещение, группы, реле, PWM, термостат, входы | [Примеры конфигураций](../documentation/core/configuration_examples.md) |
| [lighting\_dmx\_rgb\_multizone.json](lighting_dmx_rgb_multizone.json) | Многозональный DMX: RGB, диммеры, legacy Modbus-диммеры, Vacom | [Примеры конфигураций](../documentation/core/configuration_examples.md) |

## Климат и отопление

| Файл | Кейс | Документация |
|------|------|--------------|
| [thermostat\_onoff\_relay.json](thermostat_onoff_relay.json) | Простой ON/OFF термостат на реле (бак, пол) | [CH\_THERMO](../documentation/core/technical_channel_types_table.md) |
| [pid\_heating\_batteries.json](pid_heating_batteries.json) | PID-регулирование батарей отопления через slow-PWM реле (RELAYX), датчики 1-Wire | [CH\_PID](../documentation/core/technical_channel_types_table.md), [1-Wire](../documentation/integrations/1wire_sensors.md) |
| [heating\_relayx\_floor.json](heating_relayx_floor.json) | Тёплый пол: PID + медленный PWM (RELAYX), защита по времени | [CH\_RELAYX](../documentation/core/technical_channel_types_table.md) |
| [heating\_thermostat\_and\_water\_leak.json](heating_thermostat_and_water_leak.json) | Отопление всего дома, термостаты по комнатам, вход протечки | [Примеры конфигураций](../documentation/core/configuration_examples.md) |
| [garden\_relay\_thermostat\_1wire.json](garden_relay_thermostat_1wire.json) | Сад/теплица: реле, термостат, 1-Wire датчики, дискретные входы | [1-Wire](../documentation/integrations/1wire_sensors.md) |

## Вентиляция и кондиционирование

| Файл | Кейс | Документация |
|------|------|--------------|
| [multivent\_ventilation.json](multivent_ventilation.json) | Многозональная вентиляция: центральная установка (MBUS), зоны с PID и PWM-задвижками | [Модуль Multivent](../documentation/modules/multivent_module_description.md) |
| [damper\_motor\_feedback.json](damper_motor_feedback.json) | Моторные задвижки с обратной связью (аналоговый датчик положения) | [CH\_MOTOR](../documentation/core/technical_channel_types_table.md) |
| [ventilation\_fan\_monitoring.json](ventilation_fan_monitoring.json) | Мониторинг вентилятора: реле скоростей, датчик температуры | [Примеры конфигураций](../documentation/core/configuration_examples.md) |
| [ac\_haier\_control.json](ac_haier_control.json) | Кондиционер Haier по UART (родной протокол) | [CH\_AC](../documentation/core/technical_channel_types_table.md) |
| [climate\_panel\_z031.json](climate_panel_z031.json) | Климатическая панель Z031-2C0 (Modbus RTU 8E1): уставки, аварии, увлажнение, тёплый пол, кондиционер | [Панель Z031](../documentation/integrations/z031_panel.md) |

## Датчики и счётчики

| Файл | Кейс | Документация |
|------|------|--------------|
| [sensors\_modbus\_co2.json](sensors_modbus_co2.json) | CO2-датчик Sensair S8 и термометр по Modbus; PID по CO2 выдаёт запрос воздуха | [Настройка Modbus](../documentation/integrations/modbus_setup.md) |
| [counter\_energy\_monitoring.json](counter_energy_monitoring.json) | Виртуальный счётчик-сумматор (тарифные импульсы) | [CH\_COUNTER](../documentation/core/technical_channel_types_table.md) |
| [energy\_meter\_mercury.json](energy_meter_mercury.json) | Трёхфазный счётчик электроэнергии «Меркурий» по RS485 | [CH\_MERCURY](../documentation/core/technical_channel_types_table.md) |
| [modbus\_panel\_lh22\_test.json](modbus_panel_lh22_test.json) | Стенд LH22: Z031-панель, S8 CO2, термометр, DHT, датчики влажности, PWM, DMX | [Настройка Modbus](../documentation/integrations/modbus_setup.md) |

## Полив и вода

| Файл | Кейс | Документация |
|------|------|--------------|
| [sprinkler\_irrigation.json](sprinkler_irrigation.json) | Автополив: бак с поплавками, насосы, дренаж, клапаны зон, водосчётчик | [Модуль Sprinkler](../documentation/modules/Sprinkler_module.md) |

## Увлажнение

| Файл | Кейс | Документация |
|------|------|--------------|
| [humidifier\_control.json](humidifier_control.json) | Паровой увлажнитель: насос, промывочный клапан, датчик давления, зоны | [CH\_HUMIDIFIER](../documentation/core/technical_channel_types_table.md) |
| [sonoff\_s26\_humidity\_controller\_example.json](sonoff_s26_humidity_controller_example.json) | Sonoff S26 (ESP8266): PID влажности, реле, DHT22 | [Примеры конфигураций](../documentation/core/configuration_examples.md) |

## Кнопки, энкодер, CAN

| Файл | Кейс | Документация |
|------|------|--------------|
| [buttons\_and\_rotary.json](buttons_and_rotary.json) | Кнопки: одиночные/двойные/тройные/длительные нажатия, repeat; rotary-энкодер (яркость, цвет) | [Нажатия кнопок](../documentation/core/button_presses.md), [Энкодер](../documentation/core/rotary_encoder.md) |
| [can\_two\_controllers.json](can_two_controllers.json) | Два контроллера по CAN: панель Z031 на одном, вентиляция/увлажнение на другом; маршрутизация `can:[addr, item, suffix]` | [CAN и lightspot](../documentation/integrations/can_lightspot.md) |

## Legacy

| Файл | Кейс | Примечание |
|------|------|------------|
| [modbus\_ac\_dimmer\_legacy.json](modbus_ac_dimmer_legacy.json) | AC-диммер по Modbus (старый драйвер CH\_MODBUS) | Для новых проектов используйте CH\_MBUS (14) |

## Как адаптировать пример

1. **MQTT**: укажите имя контроллера и адрес брокера; при необходимости —
   `topics.root` (корень топиков) и `syslog`.
2. **Пины**: сверьте номера GPIO с распиновкой вашей платы
   ([pins\_io](../documentation/hardware/pins_io.md)); отрицательный пин
   инвертирует выход.
3. **Аппаратные адреса**: Modbus-адреса устройств, адрес CAN-контроллера.
4. **Item-имена**: переименуйте под вашу схему; имена топиков MQTT
   строятся из `root` + имя контроллера/`s_out` + имя item.
5. Конфигурация загружается контроллером как описано в
   [работе с файлом конфигурации](../documentation/core/config_file_workflow.md).
