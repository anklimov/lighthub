# LightHub: Примеры конфигурации для всех типов каналов (0-23)

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
- [CH_SPRINKLER (23)](#ch_sprinkler-23---система-полива)

---

## CH_DIMMER (0) - DMX диммер

**Назначение**: Регулировка яркости одного или нескольких DMX каналов

### Синтаксис

```json
"dimmer_name": [0, дмx_канал_или_массив]
```

Число элементов в arg задаёт число DMX каналов: 1 → одно-канальный диммер, 3 → RGB, 4 → RGBW, 5 → RGBWW.

### Примеры

```json
{
  "items": {
    "lamp_bedroom": [0, 1],           // 1 канал: DMX 1
    "lamp_kitchen": [0, 2],           // 1 канал: DMX 2
    "lamp_hall": [0, [3, 4, 5]],      // 3 канала: RGB (DMX 3-5)
    "lamp_stage": [0, 10, 255, 1]     // с начальными значениями
  }
}
```

### MQTT команды

```
myhome/dev/lamp_bedroom/cmd     ← ON, OFF, TOGGLE, SET
myhome/dev/lamp_bedroom/set     ← 150 (яркость 0-255)
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
myhome/dev/rgb_light/temp       ← 6500 (температура цвета, Mirids)
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
    "pwm_dim": [3, 9],                     // PWM на GPIO D9
    "pwm_4ch": [3, [11, 12, 13, 14]],      // RGBW: 4 канала PWM (GPIO 11-14)
    "led_pwm": [3, 5, 255, 1]              // с начальным значением
  }
}
```

### MQTT команды

```
myhome/dev/pwm_dim/set          ← 128
myhome/dev/pwm_dim/cmd          ← ON
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
myhome/dev/thermo_bath/val      ← передать измерянную температуру с удаленного сенсора
myhome/dev/thermo_bath/cmd      ← ON, OFF
```

---

## CH_RELAY (6) - GPIO Реле

**Назначение**: Простое электромагнитное реле ON/OFF или иное дискретное устройство

### Синтаксис

```json
"relay_name": [6, gpio_пин]
"relay_name": [6, gpio_пин, начальное_значение, начальная_команда]
```

### Примеры

```json
{
  "items": {
    "relay_water": [6, 23],           // Реле на GPIO 23
    "relay_pump": [6, 24, 0, 0],      // начально OFF
    "relay_heat": [6, 28, 1, 1]       // начально ON, команда ON
  }
}
```

### MQTT команды

```
myhome/dev/relay_water/cmd      ← ON, OFF, TOGGLE
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
"vacom_heat": [8, "имя_item_VC"]
```

Аргумент VCTEMP (8) — **строка**: имя item типа VC (9), к которому привязывается термостат.

### Пример

```json
{
  "items": {
    "fm": [9, [10, "a_ext"]],      // VC: modbus-адрес 10, объект "a_ext"
    "fan_heat": [8, "fm"],         // VCTEMP, привязан к item "fm"
    "fan_cool": [8, "fm"]          // второй термостат на том же VC
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
    "vent_motor": [9, [96, "имя_объекта"]]
  }
}
```

---

## CH_AC (10) - Кондиционер Haier

**Назначение**: Управление кондиционером Haier по UART (проприетарный протокол Haier, 9600 baud, не Modbus)

### Синтаксис

```json
"ac_name": [10, номер_порта]
"ac_name": [10, [номер_порта, {объект_колбэков}]]
```

Номер порта: 0-3 (0 = Serial, 1 = Serial1, 2 = Serial2, 3 = Serial3).
Ключи объекта колбэков: `fan`, `set`, `temp`, `cmd`.

### Полный пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "items": {
    "ac_hall": [10, [1, {
      "cmd": {"emit": "ac/cmd"},
      "set": {"emit": "ac/set"},
      "temp": {"emit": "ac/temp"},
      "fan": {"emit": "ac/fan"}
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
"led_name": [11, [data_pin, число_светодиодов, тип_лент]]
```

`число_светодиодов` (по умолчанию 43) и `тип_лент` — опциональны. Отдельного CLK-пина нет.

### Пример

```json
{
  "items": {
    "led_strip": [11, [7, 8]],       // пин 7, 8 светодиодов
    "led_bar": [11, [10, 11]]        // пин 10, 11 светодиодов
  }
}
```

---

## CH_MOTOR (12) - Шаговый двигатель

**Назначение**: Управление шаговым двигателем с обратной связью (для задвижек, жалюзи)

### Синтаксис

```json
"motor_name": [12, [pinUp, pinDown, pinFeedback, fbOpen, fbClosed, maxOnTime_ms]]
```

### Пример

```json
{
  "items": {
    "gate_motor": [12, [9, 10, 11, 0, 255, 30000]],
    // pinUp: 9
    // pinDown: 10
    // pinFeedback: 11
    // fbOpen: 0
    // fbClosed: 255
    // maxOnTime: 30000 мс
    
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
          "type": "x10"
        },
        "humidity": {
          "reg": 1,
          "type": "100"
        }
      }
    }
  },
  "items": {
    "sensor_outdoor": [14, [96, "temperature_sensor", {
      "temperature": {"emit": "sensors/outdoor/temp"},
      "humidity": {"emit": "sensors/outdoor/humidity"}
    }]],
    
    "sensor_indoor": [14, [97, "temperature_sensor", {
      "temperature": {"item": "thermo_room/set"},
      "humidity": {"emit": "sensors/indoor/humidity"}
    }]]
  }
}
```

> Формат item: `[14, [адрес, "имя_шаблона", {параметры}]]` — modbus-адрес обязателен.
> Масштаб значения задаётся типом регистра: `"x10"` (×0.1), `"100"` (×0.01); ключа `scale` нет.

---

## CH_UARTBRIDGE (15) - UART Мост

**Назначение**: Мост между двумя UART портами с отладкой через UDP

### Пример

```json
{
  "items": {
    "uart_debug": [15, {
      "serial": "8N1",
      "baud": 9600,
      "ip": "192.168.88.2",
      "port": 12345,
      "hd": 1,
      "log": 1,
      "bond": ","
    }]
  }
}
```

`serial` (кадр, по умолчанию 8N1), `baud` (по умолчанию 9600), `ip`/`port` (UDP-дамп трафика для отладки), `hd` (half-duplex, по умолчанию 1), `log` (уровень логирования), `bond` (границы PDU).

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
    "floor_heating": [16, [22, 60]],      // GPIO 22, период цикла 60 сек
    "radiator": [16, [23, 120]],         // GPIO 23, период цикла 120 сек
    "underfloor": [16, [24, 300], 128]   // GPIO 24, период 300 сек, начальный PWM 128
  }
}
```

Период цикла задаётся в **секундах**. Начальное значение PWM — третий элемент item (не внутри массива arg).

### MQTT команды

```
myhome/dev/floor_heating/set    ← 200 (PWM 0-255)
myhome/dev/floor_heating/val    ← текущий PWM
```

---

## CH_RGBWW (17) - DMX RGBWW

**Назначение**: DMX управление RGB + тёплый белый + холодный белый (5 каналов)

### Пример

```json
{
  "dmx": [512],
  "items": {
    "led_tunable_white": [17, 30]  // RGB+WW+CW на DMX 30-34
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

**Синтаксис**: itemArg — **объект** с зонами, формат `[[18, нач_значение], {зоны}]`.
Зона `""` — AC-узел (ключи `fan`/`set`/`cmd`/`temp`/`V`/`turnbyfan`/`boost`/`supress`/`overridecmd`,
привязка к AC item — ключ `item`), остальные зоны: `set` (число, уставка), `fan`, `cmd`, `V`,
`pid` ([kP, kI, kD, dT]), `AlarmTime` (деф 120), `AlarmValue` (деф 0).

### Полный пример

```json
{
  "mqtt": ["lighthub", "192.168.88.2"],
  "items": {
    "ac_hall": [10, 0],
    "multivent_main": [[18, 5], {
      "": {
        "item": "ac_hall",
        "fan": {"emit": "vents/main/fan"},
        "set": {"emit": "vents/main/set"},
        "cmd": {"emit": "vents/main/cmd"},
        "temp": {"emit": "vents/main/temp"}
      },
      "bedroom": {
        "fan": {"emit": "vents/bedroom/fan"},
        "set": 21,
        "V": 40,
        "pid": [1.0, 0.05, 0.02, 5.0]
      },
      "living_room": {
        "fan": {"emit": "vents/living/fan"},
        "set": 21,
        "V": 60
      },
      "kitchen": {
        "fan": {"emit": "vents/kitchen/fan"},
        "set": 20,
        "V": 30
      }
    }]
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
"counter_name": [20, [коэффициент, период_автосуммирования_сек]]
"counter_name": [20, коэффициент]
```

### Примеры

```json
{
  "items": {
    "energy_meter": [20, [0.02, 1.2]],   // коэфф 0.02, автосуммирование раз в 1.2 сек
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

**Назначение**: Управление форсуночным увлажнителем воздуха

**Синтаксис**: itemArg — **массив** (не объект):
`[21, [[pumpPin, flushPin, pressurePin, maxPumpTime, idleTime, capacity, minLevel, startLevel], {зоны}, motorExecObj, faultExecObj]]`

### Пример

```json
{
  "items": {
    "humidifier_room": [21, [
      [22, 23, 54, 255, 2500, 4, 10, 20],
      {
        "bedroom": {"N": 1, "pin": 24},
        "living": {"N": 2, "pin": 25}
      },
      {"item": "pumpctr"}
    ]]
  }
}
```

### MQTT команды

```
myhome/dev/humidifier_room/set      ← 60 (целевая влажность %)
myhome/dev/humidifier_room/cmd      ← ON, OFF
```

---

## CH_MERCURY (22) - Счётчик энергии Mercury

**Назначение**: Счётчик энергии Mercury по RS485/Modbus

### Синтаксис

```json
"mercury": [22, [адрес, baudrate, формат, уровень_доступа, пароль, pollingInterval_ms]]
```

### Пример

```json
{
  "items": {
    "energy_mercury": [22, [1, 9600, "8N1", 2, [2,2,2,2,2,2], 10000]]
    // Адрес: 1
    // Baudrate: 9600
    // Формат: 8N1
    // Уровень доступа: 2
    // Пароль: [2,2,2,2,2,2] (6 цифр)
    // Интервал опроса: 10000 мс (по умолчанию)
  }
}
```

### MQTT команды

```
myhome/dev/energy_mercury/val   ← текущие показания энергии
```

---

## CH_SPRINKLER (23) - Система полива

**Назначение**: Многозональная система автоматического полива с контролем воды и насосов

### Синтаксис

```json
"sprinkler_name": [23, {конфиг}]
```

Где `{конфиг}` — объект: в корне (зона `""`) пин системы (`vIn`, `rDren`, `rPump`, `wMax`, `wMin`, `fbDren`, `fbPump`, `wCtr`), далее — зоны полива с `pin`.

### Полный пример конфигурации

```json
{
  "items": {
    "sprinkler": [23, {
      "": {
        "vIn": 3,
        "wMax": 44,
        "wMin": 46,
        "rDren": 23,
        "fbDren": 63,
        "rPump": 24,
        "fbPump": 62,
        "wCtr": 49
      },
      "nord": {
        "pin": 6,
        "set": 60,
        "cmd": 1
      },
      "south": {
        "pin": 7,
        "set": 100,
        "cmd": 1
      },
      "trees": {
        "pin": 10,
        "set": 60,
        "cmd": 2
      },
      "outlets": {}
    }]
  }
}
```

### MQTT Команды управления

#### Запуск/остановка цикла полива
```
root/name/sprinkler/cmd        ← ON     # Запустить цикл полива всех зон
root/name/sprinkler/cmd        ← OFF    # Остановить полив
root/name/sprinkler/cmd        ← RESET  # Сбросить счётчики - инициирует старт полива

root/name/sprinkler/cmd        ← FREEZE    # Полная блокировка на зиму
root/name/sprinkler/cmd        ← UNFREEZE  # Разблокировка
root/name/sprinkler/cmd        ← DRY       # Режим осушения системы перед зимой
root/name/sprinkler/cmd        ← ENABLE    # Разрешить полив
root/name/sprinkler/cmd        ← DISABLE   # Запретить полив (например, после дождя)
```

#### Управление конкретной зоной
```
root/name/sprinkler/nord/cmd   ← ON        # Включить полив северной зоны
root/name/sprinkler/nord/cmd   ← OFF       # Выключить полив
root/name/sprinkler/nord/set   ← 60        # Требуемый объём полива
root/name/sprinkler/nord/set   ← 0         # Без лимита (для розеток)

root/name/sprinkler/south/cmd  ← ON        # Включить южную зону
root/name/sprinkler/south/set  ← 100       # Требуемый объём

root/name/sprinkler/trees/cmd  ← ON        # Включить капельное орошение
root/name/sprinkler/trees/set  ← 60        # Требуемый объём
```

#### Специальные команды
```
root/name/sprinkler/val        ← -1        # Температура < 0 → автоматический FREEZE
root/name/sprinkler/ctrl       ← FREEZE    # Установить режим FREEZE
root/name/sprinkler/ctrl       ← UNFREEZE  # Снять режим FREEZE
```

### MQTT Статусные топики (выходные)

#### Состояние системы
```
root/s_out/sprinkler/$state      → SP_UNKNOWN, SP_OFF, SP_DREN_ON, SP_DREN_OPERATE, 
                                    SP_DREN_EMPTY, SP_VIN, SP_FULL, SP_FAULT_VIN, SP_FAULT_DREN
root/s_out/sprinkler/$fault      → код ошибки (если есть)
root/s_out/sprinkler/$wMax       → ON/OFF - бак полный (поплавок макс)
root/s_out/sprinkler/$wMin       → ON/OFF - бак пуст (поплавок мин)
```

#### Состояние насосов и клапанов
```
root/s_out/sprinkler/$rPump      → ON/OFF - реле основного насоса включено
root/s_out/sprinkler/$fbPump     → ON/OFF - основной насос работает (датчик)
root/s_out/sprinkler/$rDren      → ON/OFF - реле дренажного насоса включено
root/s_out/sprinkler/$fbDren     → ON/OFF - дренажный насос работает (датчик)
root/s_out/sprinkler/$rVIN       → ON/OFF - клапан налива из водопровода открыт
root/s_out/sprinkler/set         → текущее значение счётчика воды
```

#### Состояние зон полива
```
root/s_out/sprinkler/nord/$state → ON/OFF - зона в данный момент поливается
root/s_out/sprinkler/nord/cmd    → ON/OFF - состояние команды включения
root/s_out/sprinkler/nord/set    → требуемый объём полива
root/s_out/sprinkler/nord/val    → выполненный объём полива (накапливается)

root/s_out/sprinkler/south/$state → ON/OFF - южная зона поливается
root/s_out/sprinkler/south/cmd    → ON/OFF
root/s_out/sprinkler/south/set    → требуемый объём
root/s_out/sprinkler/south/val    → выполненный объём

root/s_out/sprinkler/trees/$state → ON/OFF - капельная зона поливается
root/s_out/sprinkler/trees/cmd    → ON/OFF
root/s_out/sprinkler/trees/set    → требуемый объём
root/s_out/sprinkler/trees/val    → выполненный объём

root/s_out/sprinkler/outlets/$state → ON/OFF - розетка активна
```

### Примеры использования

#### Запуск полива с установкой объёмов
```bash
# Установить объёмы для каждой зоны
mosquitto_pub -t root/name/sprinkler/nord/set -m 60
mosquitto_pub -t root/name/sprinkler/south/set -m 100
mosquitto_pub -t root/name/sprinkler/trees/set -m 60

# Запустить цикл полива
mosquitto_pub -t root/name/sprinkler/cmd -m ON
```

#### Запрет полива после дождя и восстановление
```bash
# Запретить полив
mosquitto_pub -t root/name/sprinkler/cmd -m DISABLE

# Разрешить полив
mosquitto_pub -t root/name/sprinkler/cmd -m ENABLE
```

#### Подготовка к зиме (блокировка + осушение)
```bash
# 1. Перейти в режим DRY для осушения
mosquitto_pub -t root/name/sprinkler/cmd -m DRY

# 2. Открыть розетки для слива
mosquitto_pub -t root/name/sprinkler/outlets/cmd -m ON

# 3. После полного слива - заморозить систему
mosquitto_pub -t root/name/sprinkler/cmd -m FREEZE
```

#### Сброс счётчиков (инициирует новый цикл полива)
```bash
mosquitto_pub -t root/name/sprinkler/cmd -m RESET
```

### Интеграция с Home Assistant

```yaml
sensor:
  - name: "Полив бак Макс"
    state_topic:  "root/s_out/sprinkler/$wMax"
    
  - name: "Полив бак Мин"
    state_topic:  "root/s_out/sprinkler/$wMin"    
    
  - name: "Полив состояние"
    state_topic:  "root/s_out/sprinkler/$state"
    
  - name: "Полив ошибка"
    state_topic:  "root/s_out/sprinkler/$fault"
    
  - name: "Полив север выполнено"
    state_topic:  "root/s_out/sprinkler/nord/val"   

  - name: "Полив юг выполнено"
    state_topic:  "root/s_out/sprinkler/south/val"   

  - name: "Полив капельный выполнено"
    state_topic:  "root/s_out/sprinkler/trees/val"   

switch:
  - name: "Полив"
    state_topic:  "root/s_out/sprinkler/cmd"
    command_topic: "root/air/sprinkler/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 

  - name: "Полив север"
    state_topic:  "root/s_out/sprinkler/nord/cmd"
    command_topic: "root/air/sprinkler/nord/cmd"

  - name: "Полив юг"
    state_topic:  "root/s_out/sprinkler/south/cmd"
    command_topic: "root/air/sprinkler/south/cmd"

  - name: "Полив капельный"
    state_topic:  "root/s_out/sprinkler/trees/cmd"
    command_topic: "root/air/sprinkler/trees/cmd"

button:
  - name: "Полив сброс"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "RESET"
    
  - name: "Полив блокировка (FREEZE)"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "FREEZE"

  - name: "Полив разблокировка"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "UNFREEZE"   
    
  - name: "Полив запретить (дождь)"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "DISABLE"

  - name: "Полив разрешить"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "ENABLE"

  - name: "Полив осушение (DRY)"
    command_topic: "root/air/sprinkler/cmd"
    payload_press: "DRY"

number:
  - name: "Полив север объём"
    state_topic:  "root/s_out/sprinkler/nord/set"   
    command_topic: "root/air/sprinkler/nord/set"   
    min: 0
    max: 6000
    
  - name: "Полив юг объём"
    state_topic:  "root/s_out/sprinkler/south/set"   
    command_topic: "root/air/sprinkler/south/set"   
    min: 0
    max: 6000   
    
  - name: "Полив капельный объём"
    state_topic:  "root/s_out/sprinkler/trees/set"   
    command_topic: "root/air/sprinkler/trees/set"   
    min: 0
    max: 6000
```

### Особенности и советы

| Особенность | Описание |
|-----------|---------|
| **Водяная розетка** | Настройте зону с `set: 0` — она будет работать неограниченно и всегда под давлением |
| **Счётчик воды** | Если `wCtr` не установлен, система отсчитывает время в секундах вместо объёма |
| **Приоритет дренажа** | Система сначала наполняет бак из дренажного колодца, потом из водопровода |
| **Автозащита от мороза** | Отправьте `/val ← -1` или `FREEZE` при угрозе мороза |
| **DISABLE vs FREEZE** | DISABLE блокирует только запуск, FREEZE полностью замораживает систему |
| **Режим DRY** | Используйте перед зимой: включает насос без наполнения, зоны можно открыть вручную |
| **RESET** | Сбрасывает счётчики всех зон в 0 и инициирует новый цикл полива |

## Полная реальная конфигурация (интеграция всех типов)

```json
{
  "mqtt": ["lh-smart-home", "192.168.88.2", 1883, "mqtt_user", "mqtt_pass"],
  "topics": {"root": "myhome", "out": "s_out"},
  "syslog": ["192.168.88.2", 514],
  "dmx": [512],
  
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
    
    "ac_main": [10, [0, {
      "cmd": {"emit": "ac/cmd"},
      "set": {"emit": "ac/set"},
      "temp": {"emit": "ac/temp"},
      "fan": {"emit": "ac/fan"}
    }]],
    
    "energy_counter": [20, [0.02, 1.2]],
    
    "multivent": [[18, 5], {
      "": {"item": "ac_main", "fan": {"emit": "vent/main/fan"}},
      "bedroom": {"fan": {"emit": "vent/bedroom/fan"}, "set": 21, "V": 40},
      "kitchen": {"fan": {"emit": "vent/kitchen/fan"}, "set": 20, "V": 60}
    }]
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

- [Справочник типов каналов](channel_types_reference.md) — типы 0-23
- [Справочник суффиксов](suffixes_reference.md) — /cmd, /val, /set, /hue, /sat, /temp
- [Полное описание конфигурации](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)
- [Исходный код item.h](../../lighthub/item.h)

---

**Версия документа**: 1.2  
**Дата обновления**: 2025-06-17
