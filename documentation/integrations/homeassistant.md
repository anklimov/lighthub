# Интеграция с Home Assistant

> Источник: Wiki LightHub (lazyhome.ru), страница «интеграция\_с\_homeassistant». Снапшот от 2026-10-02.

Контроллер LightHub прекрасно интегрируется в систему Home Assistant с использованием MQTT

В настоящее время, реализованы и используются следующие интеграции:

- Реле и дискретные выключатели
- Диммируемые одноцветные светильники
- Диммируемые цветные светильники (RGB/RGBW/RGBWW)
- Термостаты теплого пола, помещений
- Кондиционер
- Увлажнитель
- Сенсоры температуры
- Сенсоры влажности
- Сенсоры CO2
- Любые устройства, управляемые по протоколу Modbus

Документация скорректирована, так как HA внес большие изменения в компонент MQTT в 2023 году
Параметры брокера теперь настраиваются только через пользовательский интерфейс в разделе Настройки/Интеграции и в файле конфигурации настройки следует убрать

Теперь предполагается, что настройки MQTT вынесены в отдельный файл mqtt.yaml
который подгружается директивой

```yaml
mqtt: !include mqtt.yaml
```

В файле configuration.yaml
Это, действительно, удобнее.

Далее, даны примеры разделов файла mqtt.yaml, которые можно скорректировать и использовать для своих нужд

## Сенсоры

В примерах ниже:
Имя контроллера: light-d2

Предположим, что к этому контроллеру подключен датчик качества воздуха CCS811 и HDC1080 и в конфиге контроллера для этого заданы такие вот настройки:

```json
"98":{"T":5,"emit":"myhome/sensor/airq"},
"99":{"T":6,"emit":"myhome/sensor/humidity"}
```

Также, на pin3 подключен сенсор температуры и влажности dht-22, расположенный в ванной, с такой вот настройкой в JSON конфиге контроллера:

```json
"3":{"T":4,"emit":"dht"},
```

ну и к шине 1-wire подключен термометр в прихожей

```json
"ow":{"28EEA76713160204":{"emit":"t_aentr"}
}
```

В этом случае, конфигурация HomeAssistant будет выглядеть вот таким образом:

```yaml
sensor:
  - name: "CO2"
    icon: "mdi:periodic-table-co2"
    expire_after: 600
    state_topic: "myhome/sensor/airq/CO2"
    unit_of_measurement: 'ppm'
    availability_topic: "myhome/light-d2/$state"
    payload_available: "ready"
    payload_not_available: "disconnected"

  - name: "TVOC"
    expire_after: 600
    state_topic: "myhome/sensor/airq/TVOC"
    unit_of_measurement: 'ppm'
    availability_topic: "myhome/light-d2/$state"
    payload_available: "ready"
    payload_not_available: "disconnected"

  - name: "Влажность"
    icon: "mdi:water-percent"
    expire_after: 600
    state_topic: "myhome/sensor/humidity/H"
    unit_of_measurement: '%'
    availability_topic: "myhome/light-d2/$state"
    payload_available: "ready"
    payload_not_available: "disconnected"


  - name: "Влажность ванная DHT"
    icon: "mdi:water-percent"
    expire_after: 600
    state_topic: "myhome/s_out/dhtH"
    unit_of_measurement: '%'

  - name: "Воздух ванная DHT"
    icon: "mdi:thermometer"
    expire_after: 600
    state_topic: "myhome/s_out/dhtT"
    unit_of_measurement: '°C'

  - name: "Воздух прихожая DS"
    icon: "mdi:thermometer"
    expire_after: 600
    state_topic: "myhome/s_out/t_aentr"
    unit_of_measurement: '°C'
```

## Светильники

```yaml
light:
# дискретный выключатель света (вкл-выкл)
  - name: "Прихожая"
    state_topic: "myhome/s_out/spots_en"
    command_topic: "myhome/in/spots_en"
    qos: 1

# Диммируемый одноцветный светильник
  - name: Кухня точки
    command_topic: "myhome/in/ktc/cmd"
    state_topic: "myhome/s_out/ktc/cmd"
#    brightness_scale: 100 - начиная с версии 3.0.0 контроллера, настройка не нужна
    brightness_command_topic: "myhome/in/ktc/set"
    brightness_state_topic: "myhome/s_out/ktc/set"

# Цветной LED светильник

  - name: Кухня LED
    command_topic: "myhome/in/kuh/cmd"
    state_topic: "myhome/s_out/kuh/cmd"
    hs_command_topic: "myhome/in/kuh/set"
    hs_state_topic: "myhome/s_out/kuh/set"
    hs_value_template: "{%set a=value.split(',')%}
                        {% if a[1] -%}
                          {{a[0]+','+a[1]}}
                        {%- endif %}"

    brightness_command_topic: "myhome/in/kuh/set"
    brightness_state_topic: "myhome/s_out/kuh/set"
    brightness_value_template: "{{ value.split(',')[2] }}"

    color_temp_value_template: "{{ value.split(',')[3] }}" # Опционально
    color_temp_command_topic: "myhome/in/kuh/temp" # Опционально
    color_temp_state_topic: "myhome/s_out/kuh/set" # Опционально


#Новая версия интеграции. Поддерживает не только HSV но и RGBW нотацию цвета.
#Полезно, если на DMX вход контроллера подключена сенсорная панель, которая выдает значения каждого из каналов в отдельности
#- то есть RGBW. В этом случае контроллер прозрачно транслирует такой цвет в статусный топик, добавляя признак RGB в начало payload
# предполагается имя контроллера light

  - name: Кухня по новому LED
    command_topic: "myhome/light/kuh/cmd"
    state_topic: "myhome/s_out/kuh/cmd"
    hs_command_topic: "myhome/light/kuh/set"
    hs_state_topic: "myhome/s_out/kuh/set"
    brightness_command_topic: "myhome/light/kuh/set"
    brightness_state_topic: "myhome/s_out/kuh/set"
    color_temp_command_topic: "myhome/light/kuh/temp"
    color_temp_state_topic: "myhome/s_out/kuh/set"
    rgbw_state_topic: "myhome/s_out/kuh/set"

    hs_value_template:  "{%if not value is match('RGB')%}
                        {%set a=value.split(',')%}
                        {%if a[1]%}
                        {{a[0]+','+a[1]}}
                        {%endif%}
                        {% endif %}"

    brightness_value_template: "{%if not value is match('RGB')%}
                                {%set a=value.split(',')%}
                                {% if a[2] %}
                                {{ a[2] }}
                                {% endif %}
                                {% endif %}"

    color_temp_value_template: "{%if not value is match('RGB')%}
                                {%set a=value.split(',')%}
                                {% if a[3] %}
                                {{ a[3] }}
                                {% endif %}
                                {% endif %}"

    rgbw_value_template: "{%if value is match('RGB')%}
                          {{value.split()[1]}}
                          {% endif %}"
```

## Выключатели

```yaml
switch:
  - name: "Air heat"
    icon: "mdi:radiator"
    state_topic: "myhome/s_out/fm_t/cmd"
    command_topic: "myhome/in/fm_t/cmd"

  - name: "Air auto"
    icon: "mdi:auto-fix"
    state_topic: "myhome/s_out/fm_auto"
    command_topic: "myhome/in/fm_auto"

  - name: "Перекрыть воду"
    icon: "mdi:water-off"
    state_topic: "myhome/s_out/alock_leak"
    command_topic: "myhome/in/alock"
    qos: 1

  - name: "Сушилки"
    icon: "mdi:radiator"
    state_topic: "myhome/s_out/h_polotenc"
    command_topic: "myhome/in/h_polotenc"
    qos: 1

  - name: "Бойлер большой"
    state_topic: "myhome/s_out/h_boil1"
    command_topic: "myhome/in/h_boil1"
    qos: 1

  - name: "Бойлер малый"
    state_topic: "myhome/s_out/h_boil2"
    command_topic: "myhome/in/h_boil2"
    qos: 1
```

## Климат и термостаты

```yaml
climate:

  - name: Кондиционер
    modes:
      - "off"
      - "cool"
      - "fan_only"
      - "heat"
      - "auto"
    fan_modes:
      - "auto"
      - "high"
      - "medium"
      - "low"
    swing_modes:
      - "on"
      - "off"
    swing_mode_state_topic: "myhome/s_out/ac/swing"
    swing_mode_command_topic: "myhome/in/ac/swing"
    swing_mode_state_template: "{{value|lower()}}"
    mode_command_topic: "myhome/in/ac/cmd"
    mode_state_topic: "myhome/s_out/ac/cmd"
    mode_state_template: "{{value|lower()}}"
    temperature_command_topic: "myhome/in/ac/set"
    temperature_state_topic: "myhome/s_out/ac/set"
    fan_mode_command_topic: "myhome/in/ac/fan"
    fan_mode_state_topic: "myhome/s_out/ac/fan"
    fan_mode_state_template: "{{value|lower()}}"
    current_temperature_topic: "myhome/s_out/ac/temp"

  - name: "Теплый пол душ"
    modes:
      - "off"
      - "heat"
      - "auto"
    mode_command_topic: "myhome/in/h_bath1/cmd"
    temperature_command_topic: "myhome/in/h_bath1/set"
    mode_state_topic: "myhome/s_out/h_bath1/cmd"
    mode_state_template: "{{value|lower()}}"
    temperature_state_topic: "myhome/s_out/h_bath1/set"
    current_temperature_topic: "myhome/s_out/t_bath1"


  - name: "Климат спальня"
    modes:
      - "off"
      - "heat"
      - "auto"
      - "cool"
      - "fan_only"
    mode_command_topic: "myhome/in/c_bedr/cmd"
    temperature_command_topic: "myhome/in/c_bedr/set"
    mode_state_topic: "myhome/s_out/c_bedr/cmd"
    mode_state_template: "{{value|lower()}}"
    temperature_state_topic: "myhome/s_out/c_bedr/set"
    current_temperature_topic: "myhome/s_out/t_bedr"
```

## Увлажнитель

```yaml
humidifier:
  - name: "Увлажнитель пар"
    command_topic: "myhome/plug00/reghum/cmd"
    state_topic: "myhome/s_out/reghum/cmd"
    target_humidity_command_topic: "myhome/plug00/reghum/set"
    target_humidity_state_topic: "myhome/s_out/reghum/set"
    current_humidity_topic: "myhome/s_out/plughum"
    max_humidity: 60
    min_humidity: 20
```

## Приточка

```yaml
climate:

  - name: Вентиляция
    modes:
      - "off"
      - "fan_only"
      - "heat"
      - "auto"
    mode_command_topic: "myhome/in/fm/cmd"
    mode_state_topic: "myhome/s_out/fm/cmd"
    mode_state_template: "{{value|lower()}}"
    temperature_command_topic: "myhome/in/fm_t/set"
    temperature_state_topic: "myhome/s_out/fm_t/set"
    current_temperature_topic: "myhome/s_out/fm_stat"
    current_temperature_template: "{{ value_json.t }}"

sensor:

  - name: "Heater power"
    state_topic: "myhome/s_out/fm_stat"
    icon: "mdi:radiator"
    unit_of_measurement: '%'
    value_template: "{{ value_json.pwr }}"

  - name: "Fan RPM"
    icon: "mdi:fan"
    state_topic: "myhome/s_out/fm_stat"
    unit_of_measurement: 'RPM'
    value_template: "{{ value_json.RPM }}"

  - name: "Fan Current"
    icon: "mdi:current-ac"
    state_topic: "myhome/s_out/fm_stat"
    unit_of_measurement: 'A'
    value_template: "{{ value_json.I }}"

  - name: "Fan Fault"
    icon: "mdi:alert"
    state_topic: "myhome/s_out/fm_stat"
    value_template: "{{ value_json.flt }}"
#  - platform: mqtt
#    name: "Fan Temp"
#    state_topic: "myhome/s_out/fm_stat"
#    unit_of_measurement: '°C'
#    value_template: "{{ value_json.t }}"
#  - platform: mqtt
#    name: "Fan Temp set"
#    state_topic: "myhome/s_out/fm_stat"
#    unit_of_measurement: '°C'
#    value_template: "{{ value_json.set }}"
```

## Сверка с кодом (2026-10-02)

- Страница содержит только примеры конфигурации HA (yaml) и стандартные фрагменты конфига LightHub; специфических для HA ключей контроллера нет.
- Форматы топиков `myhome/in/...` и `myhome/s_out/...`, а также служебный топик `myhome/<имя_контроллера>/$state` соответствуют префиксам по умолчанию контроллера.
- Настройки датчиков в примерах — стандартные записи входов (`"98":{"T":5,...}`, `"3":{"T":4,...}`) и раздел `ow` (см. [Подключение 1-Wire датчиков](1wire_sensors.md)).
- Контент, относящийся к модулю sprinkler, на странице отсутствует (отдельный документ — `documentation/Sprinkler_HomeAssistant.md`).
