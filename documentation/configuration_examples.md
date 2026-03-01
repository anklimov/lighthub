# LightHub: Примеры конфигурации для всех типов каналов (0-22)

> **Практический справочник** с готовыми примерами JSON конфигурации для каждого типа канала.
> Каждый пример показывает полный синтаксис, включая MQTT топики и все параметры.

---

## Содержание

- [CH_DIMMER (0)](#ch_dimmer-0---dmx-диммер)
- [CH_RGBW (1)](#ch_rgbw-1---dmx-rgbwhite)
- [CH_RGB (2)](#ch_rgb-2---dmx-rgb)
- [CH_PWM (3)](#ch_pwm-3---gpio-pwm)
- [CH_MODBUS (4)](#ch_modbus-4---modbus-ac-dimmer-legacy)
- [CH_THERMO (5)](#ch_thermo-5---onoff-термостат)
- [CH_RELAY (6)](#ch_relay-6---gpio-реле)
- [CH_GROUP (7)](#ch_group-7---группа-каналов)
- [CH_VCTEMP (8)](#ch_vctemp-8---vacom-pid-терморегулятор)
- [CH_VC (9)](#ch_vc-9---vacom-мотор-регулятор)
- [CH_AC (10)](#ch_ac-10---кондиционер-haier)
- [CH_SPILED (11)](#ch_spiled-11---spi-led-лента)
- [CH_MOTOR (12)](#ch_motor-12---шаговый-двигатель)
- [CH_PID (13)](#ch_pid-13---pid-регулятор)
- [CH_MBUS (14)](#ch_mbus-14---universal-modbus)
- [CH_UARTBRIDGE (15)](#ch_uartbridge-15---uart-мост)
- [CH_RELAYX (16)](#ch_relayx-16---медленный-pwm-через-реле)
- [CH_RGBWW (17)](#ch_rgbww-17---dmx-rgbww)
- [CH_MULTIVENT (18)](#ch_multivent-18---многозональная-вентиляция)
- [CH_ELEVATOR (19)](#ch_elevator-19---управление-лифтом)
- [CH_COUNTER (20)](#ch_counter-20---счётчик-импульсов)
- [CH_HUMIDIFIER (21)](#ch_humidifier-21---управление-увлажнителем)
- [CH_MERCURY (22)](#ch_mercury-22---счётчик-энергии-mercury)

---

## CH_DIMMER (0) - DMX диммер

**Назначение**: Регулировка яркости одного или нескольких DMX каналов

### Синтаксис

```json
"dimmer_name": [0, дмx_канал_или_массив]
```

### Примеры

```json
{
  "items": {
    "lamp_bedroom": [0, 1],
    "lamp_kitchen": [0, 2],
    "lamp_hall": [0, [3, 4, 5]],
    "lamp_stage": [0, 10, 255, 1]
  }
}
```

### MQTT команды

```
myhome/dev/lamp_bedroom/cmd     ← ON, OFF, TOGGLE, SET
myhome/dev/lamp_bedroom/set     ← 150 (яркость 0-255)
myhome/dev/lamp_bedroom/val     ← текущая яркость
myhome/s_out/lamp_bedroom/val   → опубликованная яркость
```

---

## CH_RGBW (1) - DMX RGBW

**Назначение**: Управление RGB + White каналом через DMX (4 канала DMX)

### Синтаксис

```json
"rgb_name": [1, стартовый_dmx_канал]
```

### Пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "dmx": [512],
  "items": {
    "rgb_light": [1, 10],        // RGB+W на DMX 10-13
    "rgb_light2": [1, 20, 255, 0] // с начальной яркостью 255
  }
}
```

### MQTT команды

```
myhome/dev/rgb_light/hue        ← 240 (0-359°)
myhome/dev/rgb_light/sat        ← 100 (0-100%)
myhome/dev/rgb_light/val        ← 200 (0-255)
myhome/dev/rgb_light/temp       ← 6500 (температура цвета, K)
myhome/dev/rgb_light/RGB        ← 255,0,0 (красный)
myhome/dev/rgb_light/RGB        ← 255,0,0,100 (RGB+White)
```

---

## CH_RGB (2) - DMX RGB

**Назначение**: Управление RGB каналом через DMX (3 канала DMX)

### Пример

```json
{
  "items": {
    "mood_light": [2, 15]         // RGB на DMX 15-17
  }
}
```

### MQTT команды

```
myhome/dev/mood_light/RGB       ← 0,255,0 (зелёный)
myhome/dev/mood_light/HSV       ← 120,100,100 (зелёный в HSV)
```

---

## CH_PWM (3) - GPIO PWM

**Назначение**: PWM выход через GPIO пины (для плат Arduino, ESP)

### Синтаксис

```json
"pwm_name": [3, gpio_пин_или_массив]
```

### Примеры

```json
{
  "items": {
    "pwm_dim": [3, 9],            // PWM на GPIO D9
    "pwm_4ch": [3, [11, 12, 13, 14]],  // 4-х канальный PWM
    "led_pwm": [3, 5, 255, 1]     // с начальным значением
  }
}
```

### MQTT команды

```
myhome/dev/pwm_dim/set          ← 128
myhome/dev/pwm_dim/val          ← текущее значение
```

---

## CH_MODBUS (4) - Modbus AC Dimmer (Legacy)

**Назначение**: Управление AC-диммером через Modbus RTU (устаревший тип, использовать CH_MBUS (14))

### Синтаксис

```json
"mbus_dim": [4, [адрес, регистр, маска, макс_значение]]
```

### Пример

```json
{
  "items": {
    "ac_dimmer": [4, [96, 0, 0, 255]]
    // Адрес: 96 (0x60)
    // Регистр: 0
    // Маска: 0 (LSB)
    // Макс значение: 255
  }
}
```

⚠️ **Примечание**: Для новых проектов используйте CH_MBUS (14) вместо этого

---

## CH_THERMO (5) - ON/OFF Термостат

**Назначение**: Простой ON/OFF термостат с гистерезисом

### Синтаксис

```json
"thermo_name": [5, gpio_пин, целевая_температура°C]
```

### Примеры

```json
{
  "items": {
    "thermo_bath": [5, 24, 33],       // GPIO 24, уставка 33°C
    "thermo_bedroom": [5, 25, 22],    // GPIO 25, уставка 22°C
    "floor_heating": [5, 26, 28, 1, 1] // с начальным значением
  }
}
```

### MQTT команды

```
myhome/dev/thermo_bath/set      ← 35 (установить целевую T)
myhome/dev/thermo_bath/val      ← 1 (нагревается) или 0
myhome/dev/thermo_bath/cmd      ← ON, OFF
```

---

## CH_RELAY (6) - GPIO Реле

**Назначение**: Простое электромагнитное реле ON/OFF

### Синтаксис

```json
"relay_name": [6, gpio_пин]
"relay_name": ["RELAY", gpio_пин, [начальное_значение, [начальная_команда]]]
```

### Примеры

```json
{
  "items": {
    "relay_water": [6, 23],           // Реле на GPIO 23
    "relay_pump": [6, 24, 0, 0],      // начально OFF
    "relay_heat": ["RELAY", 28, 1, 1] // начально ON, команда ON
  }
}
```

### MQTT команды

```
myhome/dev/relay_water/cmd      ← ON, OFF, TOGGLE
myhome/dev/relay_water/val      ← 1 (включено) или 0 (выключено)
```

---

## CH_GROUP (7) - Группа каналов

**Назначение**: Логическая группа для синхронного управления несколькими каналами

### Синтаксис

```json
"group_name": [7, [канал1, канал2, канал3, ...]]
```

### Примеры

```json
{
  "items": {
    "lamps": [0, 1],
    "lamps2": [0, 2],
    "rgb1": [1, 10],
    
    "lights_all": [7, [
      "lamps",
      "lamps2", 
      "rgb1"
    ]],
    
    "lights_bedroom": [7, ["lamps"]],
    "lights_common": [7, ["lamps", "lamps2"]]
  }
}
```

### MQTT команды

```
myhome/dev/lights_all/cmd       ← ON (включит ВСЕ в группе)
myhome/dev/lights_all/cmd       ← OFF (выключит ВСЕ в группе)
myhome/dev/lights_all/cmd       ← SET 100 (установит яркость для диммеров)
```

---

## CH_VCTEMP (8) - Vacom PID Терморегулятор

**Назначение**: PID регулятор температуры для систем вентиляции Vacom

### Синтаксис

```json
"vacom_heat": [8, [modbus_адрес, экземпляр]]
```

### Пример

```json
{
  "modbus": {
    "vacom_10": {
      "baud": 9600,
      "serial": "8N1",
      "poll": {"regs": [[0, 50]], "delay": 1000}
    }
  },
  "items": {
    "fan_heat": [8, [96, 0]],        // Vacom адрес 96, экземпляр 0
    "fan_cool": [8, [96, 1]]         // Vacom адрес 96, экземпляр 1
  }
}
```

---

## CH_VC (9) - Vacom Мотор-регулятор

**Назначение**: Управление мотор-регулятором вентилятора Vacom

### Пример

```json
{
  "items": {
    "vent_motor": [9, [96, {
      "mode": {"emit": "vent/mode"},
      "speed": {"emit": "vent/speed"}
    }]]
  }
}
```

---

## CH_AC (10) - Кондиционер Haier

**Назначение**: Управление кондиционером Haier через Modbus/RS485

### Полный пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "modbus": {
    "haier_ac": {
      "baud": 9600,
      "serial": "8N1",
      "poll": {
        "regs": [[1, 30]],
        "delay": 1000
      },
      "par": {
        "power": {
          "reg": 1,
          "type": "u16",
          "map": {"cmd": [["OFF", 0], ["ON", 1]]}
        },
        "mode": {
          "reg": 2,
          "type": "u16",
          "map": {"cmd": [["COOL", 0], ["HEAT", 1], ["DRY", 2], ["FAN", 3]]}
        },
        "temperature": {
          "reg": 3,
          "type": "i16",
          "scale": 0.1
        },
        "fan_speed": {
          "reg": 4,
          "type": "u16"
        }
      }
    }
  },
  "items": {
    "ac_hall": [10, [1, {
      "power": {"emit": "ac/power"},
      "mode": {"emit": "ac/mode"},
      "temperature": {"emit": "ac/temp"},
      "fan_speed": {"emit": "ac/fan"}
    }]]
  }
}
```

### MQTT команды

```
myhome/dev/ac_hall/cmd          ← ON, OFF
myhome/dev/ac_hall/mode         ← COOL, HEAT, DRY, FAN
myhome/dev/ac_hall/set          ← 22 (температура)
myhome/dev/ac_hall/fan          ← 0, 1, 2, 3 (скорость)
```

---

## CH_SPILED (11) - SPI LED Лента

**Назначение**: Управление адресуемой SPI LED лентой (WS2812B, APA102)

### Синтаксис

```json
"led_name": [11, [clk_pin, data_pin]]
```

### Пример

```json
{
  "items": {
    "led_strip": [11, [7, 8]],       // CLK=GPIO7, DATA=GPIO8
    "led_bar": [11, [10, 11]]
  }
}
```

---

## CH_MOTOR (12) - Шаговый двигатель

**Назначение**: Управление шаговым двигателем с обратной связью (для задвижек, жалюзи)

### Синтаксис

```json
"motor_name": [12, [pwm_pin, open_pin, close_pin, feedback_off_val, feedback_on_val, max_time_ms]]
```

### Пример

```json
{
  "items": {
    "gate_motor": [12, [9, 10, 11, 0, 255, 30000]],
    // PWM pin: 9
    // Open pin: 10
    // Close pin: 11
    // Feedback off: 0
    // Feedback on: 255
    // Max time: 30 сек
    
    "blinds": [12, [5, 6, 7, 100, 900, 20000]]
  }
}
```

### MQTT команды

```
myhome/dev/gate_motor/cmd       ← ON (открыть), OFF (закрыть)
myhome/dev/gate_motor/val       ← текущая позиция обратной связи
```

---

## CH_PID (13) - PID Регулятор

**Назначение**: Универсальный PID контроллер для процессов

### Синтаксис

```json
"pid_name": [13, [
  [Kp, Ki, Kd, dT, timeout, alarm_val, min_out, max_out],
  {выходной_execObj},
  {каскадный_execObj}
]]
```

### Пример

```json
{
  "items": {
    "pid_heater": [13, [
      [1.0, 0.05, 0.02, 5.0, 3600, 50, 0, 255],
      {"emit": "heater/output"},
      {"emit": "heater/cascade"}
    ]],
    
    "pid_cooler": [13, [
      [0.8, 0.03, 0.01, 5.0, 1800, 100, 0, 200],
      {"item": "fan_speed/set"},
      null
    ]]
  }
}
```

### Параметры

| Параметр | Описание |
|--|--|
| `Kp` | Коэффициент пропорциональности |
| `Ki` | Коэффициент интеграла |
| `Kd` | Коэффициент дифференциала |
| `dT` | Интервал расчёта (секунды) |
| `timeout` | Время срабатывания аварии (сек) |
| `alarm_val` | Значение аварии |
| `min_out`, `max_out` | Диапазон выхода |

---

## CH_MBUS (14) - Universal Modbus

**Назначение**: Универсальный Modbus канал с полной поддержкой шаблонизации

### Полный пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "modbus": {
    "temperature_sensor": {
      "baud": 9600,
      "serial": "8N1",
      "poll": {
        "regs": [[0, 10]],
        "delay": 2000
      },
      "par": {
        "temperature": {
          "reg": 0,
          "type": "i16",
          "scale": 0.1
        },
        "humidity": {
          "reg": 1,
          "type": "u16",
          "scale": 0.01
        }
      }
    }
  },
  "items": {
    "sensor_outdoor": [14, ["temperature_sensor", {
      "temperature": {"emit": "sensors/outdoor/temp"},
      "humidity": {"emit": "sensors/outdoor/humidity"}
    }]],
    
    "sensor_indoor": [14, ["temperature_sensor", {
      "temperature": {"item": "thermo_room/set"},
      "humidity": {"emit": "sensors/indoor/humidity"}
    }]]
  }
}
```

---

## CH_UARTBRIDGE (15) - UART Мост

**Назначение**: Мост между двумя UART портами с отладкой через UDP

### Пример

```json
{
  "items": {
    "uart_debug": [15, {
      "port1": 1,
      "port2": 0
    }]
  }
}
```

---

## CH_RELAYX (16) - Медленный PWM через реле

**Назначение**: Медленный PWM через реле для инертных систем (тепловые системы, комплексы)

### Синтаксис

```json
"relay_pwm": [16, [gpio_pin, период_цикла_сек]]
```

### Пример

```json
{
  "items": {
    "floor_heating": [16, [22, 60]],     // GPIO 22, период 60 сек
    "radiator": [16, [23, 120]],         // GPIO 23, период 120 сек
    "underfloor": [16, [24, 300, 128]]   // с начальным PWM 128
  }
}
```

### MQTT команды

```
myhome/dev/floor_heating/set    ← 200 (PWM 0-255)
myhome/dev/floor_heating/val    ← текущий PWM
```

---

## CH_RGBWW (17) - DMX RGBWW

**Назначение**: DMX управление RGB + тёплый белый + холодный белый (6 каналов)

### Пример

```json
{
  "dmx": [512],
  "items": {
    "led_tunable_white": [17, 30]  // RGB+W+W на DMX 30-35
  }
}
```

### MQTT команды

```
myhome/dev/led_tunable_white/RGB  ← 255,128,0,200,50
// R, G, B, Warm White, Cold White
```

---

## CH_MULTIVENT (18) - Многозональная вентиляция

**Назначение**: Каскадная система вентиляции с независимым регулированием по зонам

### Полный пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "items": {
    "multivent_main": [18, [96, {
      "": {
        "val": {"emit": "vents/main/temp"},
        "mode": {"emit": "vents/main/mode"}
      },
      "bedroom": {
        "val": {"emit": "vents/bedroom/temp"},
        "fan": {"emit": "vents/bedroom/fan"},
        "set": {"emit": "vents/bedroom/setpoint"},
        "V": 40,
        "pid": [1.0, 0.05, 0.02, 5.0]
      },
      "living_room": {
        "val": {"emit": "vents/living/temp"},
        "set": 21,
        "V": 60
      },
      "kitchen": {
        "val": {"emit": "vents/kitchen/temp"},
        "set": 20,
        "V": 30
      }
    }]]
  }
}
```

### MQTT команды

```
myhome/dev/multivent_main/bedroom/set   ← 22 (уставка T)
myhome/dev/multivent_main/bedroom/fan   ← 50 (открытие жалюзи)
myhome/dev/multivent_main/bedroom/mode  ← HEAT, COOL, FAN
```

---

## CH_ELEVATOR (19) - Управление лифтом

**Назначение**: Управление лифтом (зарезервировано для будущего использования)

**Статус**: TBD (To Be Determined)

---

## CH_COUNTER (20) - Счётчик импульсов

**Назначение**: Счётчик импульсов (электроэнергия, газ, вода)

### Синтаксис

```json
"counter_name": [20, [коэффициент, масштаб]]
"counter_name": [20, коэффициент]
```

### Примеры

```json
{
  "items": {
    "energy_meter": [20, [0.02, 1.2]],   // коэфф 0.02, масштаб 1.2
    "gas_meter": [20, 0.001],            // коэфф 0.001
    "water_meter": [20, 0.01, 0, 0]      // с начальным значением 0
  }
}
```

### MQTT команды

```
myhome/dev/energy_meter/val     ← текущее значение (кВт·ч)
myhome/dev/gas_meter/val        ← текущее значение (м³)
```

---

## CH_HUMIDIFIER (21) - Управление увлажнителем

**Назначение**: Управление увлажнителем воздуха

### Пример

```json
{
  "items": {
    "humidifier_room": [21, {
      "humidity": {"emit": "humidifier/setpoint"},
      "mode": {"emit": "humidifier/mode"},
      "power": {"emit": "humidifier/power"}
    }]
  }
}
```

### MQTT команды

```
myhome/dev/humidifier_room/set      ← 60 (целевая влажность %)
myhome/dev/humidifier_room/cmd      ← ON, OFF
myhome/dev/humidifier_room/mode     ← режим
```

---

## CH_MERCURY (22) - Счётчик энергии Mercury

**Назначение**: Счётчик энергии Mercury по RS485/Modbus

### Синтаксис

```json
"mercury": [22, [адрес, baudrate, формат, сдвиг, флаги, timeout]]
```

### Пример

```json
{
  "items": {
    "energy_mercury": [22, [1, 9600, "8N1", 2, [2,2,2,2,2,2], 10000]]
    // Адрес: 1
    // Baudrate: 9600
    // Формат: 8N1
    // Сдвиг: 2
    // Флаги: [2,2,2,2,2,2]
    // Timeout: 10000 мс
  }
}
```

### MQTT команды

```
myhome/dev/energy_mercury/val   ← текущие показания энергии
```

---

## Полная реальная конфигурация (интеграция всех типов)

```json
{
  "mqtt": ["lh-smart-home", "192.168.88.2", 1883, "mqtt_user", "mqtt_pass"],
  "topics": {"root": "myhome", "out": "s_out"},
  "syslog": ["192.168.88.2", 514],
  "dmx": [512],
  
  "modbus": {
    "haier_ac": {
      "baud": 9600,
      "serial": "8N1",
      "poll": {"regs": [[1, 30]], "delay": 1000},
      "par": {
        "power": {"reg": 1, "type": "u16"},
        "mode": {"reg": 2, "type": "u16"},
        "temperature": {"reg": 3, "type": "i16", "scale": 0.1}
      }
    }
  },
  
  "items": {
    "lights_all": [7, ["lamp1", "lamp2", "rgb1", "rgb2"]],
    
    "lamp1": [0, 1],
    "lamp2": [0, 2],
    "rgb1": [1, 10],
    "rgb2": [1, 20],
    
    "relay_water": [6, 23],
    "relay_pump": [6, 24],
    "relay_heat": [6, 25],
    
    "pwm_fan": [3, 9],
    
    "thermo_bath": [5, 26, 33],
    "thermo_hall": [5, 27, 22],
    
    "ac_main": [14, ["haier_ac", {
      "power": {"emit": "ac/power"},
      "mode": {"emit": "ac/mode"},
      "temperature": {"emit": "ac/temp"}
    }]],
    
    "energy_counter": [20, [0.02, 1.2]],
    
    "multivent": [18, [96, {
      "": {"val": {"emit": "vent/main"}},
      "bedroom": {"val": {"emit": "vent/bedroom"}, "V": 40},
      "kitchen": {"val": {"emit": "vent/kitchen"}, "V": 60}
    }]]
  },
  
  "in": {
    "37": {"item": "lights_all", "scmd": "TOGGLE"},
    "38": {"item": "relay_water", "scmd": "OFF"},
    "39": {"emit": "/status/water_leak"}
  }
}
```

---

## Полезные ссылки

- [Справочник типов каналов](channel_types_reference.md) — типы 0-22
- [Справочник суффиксов](suffixes_reference.md) — /cmd, /val, /set, /hue, /sat, /temp
- [Полное описание конфигурации](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)
- [Исходный код item.h](../lighthub/item.h)

---

**Версия документа**: 1.0  
**Дата обновления**: 2025-01-24
