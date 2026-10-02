# LightHub: Справочник типов каналов (Items)

> **Инженерный справочник** соответствия цифровых кодов типов каналов к текстовым обозначениям и функциональности.
> Актуально для версии ядра с типами DMX (0) до SPRINKLR (23).
> Источник: [lighthub/item.h](../../lighthub/item.h)

---

## Таблица типов каналов

| Код | Текстовое обозначение | Английское название | Описание | Конфигурация |
|-----|----------------------|---------------------|---------|--------------|
| **0** | `DMX` | DMX Dimmer | DMX 512 выход с регулировкой яркости (1-5 каналов (3=RGB, 4=RGBW, 5=RGBWW)) | Номер DMX канала или массив номеров |
| **1** | `DMXRGBW` | DMX RGBW | DMX 512 выход RGB+White (4 канала) | Номер стартового DMX канала |
| **2** | `DMXRGB` | DMX RGB | DMX 512 выход RGB (3 канала) | Номер стартового DMX канала |
| **3** | `PWM` | PWM Output | Широтно-импульсная модуляция на GPIO (1-5 каналов) | Номер GPIO пина или массив GPIO пинов |
| **4** | `MBUSDIM` | Modbus AC Dimmer (Legacy) | Управление AC-диммером через Modbus RTU | `[адрес, регистр, маска, макс_значение, тип_регистра]` |
| **5** | `THERMO` | Simple Thermostat | ON/OFF термостат с гистерезисом | `[GPIO_pin, целевая_температура_°C]` |
| **6** | `RELAY` | Relay Output | Электромагнитное реле ON/OFF | GPIO пин |
| **7** | `GROUP` | Group Channel | Логическая группа каналов для синхронного управления | Массив строк с именами каналов |
| **8** | `VCTEMP` | Vacom PID Thermo | Температура из VC-канала Vacom (ссылка на item типа CH_VC) | `[имя_канала_VC]` (строка) |
| **9** | `MBUSVC` | Vacom Modbus Motor | Управление мотор-регулятором вентилятора Vacom через Modbus | `[modbus_адрес, "имя_объекта"]` |
| **10** | `ACHAIER` | Air Conditioner Haier | Управление кондиционером Haier по проприетарному UART-протоколу (9600) | `[порт_serial(0-3)]` |
| **11** | `SPILED` | SPI LED Strip | Управление NeoPixel LED лентой (WS2812B и совместимые, один data-пин) | `[data_pin, numLeds(дефолт 43), ledsType]` |
| **12** | `MOTOR` | Motorized Air Gateway | Управление моторной задвижкой с обратной связью (позиция 0-255) | `[pinUp, pinDown, pinFeedback, fbOpen, fbClosed, maxOnTime_ms(дефолт 10000)]` |
| **13** | `PID` | PID Regulator | Универсальный PID-контроллер для регулирования процессов | `[Kp, Ki, Kd, dT, timeout, alarm_val, min_out, max_out]` |
| **14** | `MBUS` | Universal Modbus | Универсальный Modbus канал с шаблонизацией | `[адрес, шаблон, параметры]` |
| **15** | `UARTBRDG` | UART Bridge | Мост между двумя UART портами с отладкой через UDP | `{serial, baud, ip, port, hd, log, bond}` |
| **16** | `RELAYPWM` | Relay PWM | Медленный PWM через реле для инертных систем | `[GPIO_pin, период_цикла_сек(дефолт 5)]` |
| **17** | `DMXRGBWW` | DMX RGBWW | DMX 512 выход RGB + теплый белый + холодный белый (5 каналов) | Номер стартового DMX канала |
| **18** | `VENTS` | Multiroom Ventilation | Многозональная вентиляция с каскадным управлением | `[объект_зон]` (без Modbus-адреса) |
| **19** | `ELEVATOR` | Elevator Control | Управление лифтом | `{"serial":"8N1","baud":19200,"log":...}` (компилируется с ELEVATOR_ENABLE) |
| **20** | `COUNTER` | Generic Counter | Счётчик импульсов (электроэнергия, газ, вода) | `[коэффициент_импульса, период_автосуммирования_сек]` |
| **21** | `HUM` | Humidifier | Управление увлажнителем воздуха (только один в системе) | `[[pumpPin, flushPin, pressurePin, maxPumpTime, idleTime, capacity, minLevel, startLevel], {зоны_краны}, motorExec, faultExec]` |
| **22** | `MERCURY` | Mercury Energy Meter | Счётчик энергии Mercury по RS485/Modbus | `[адрес, baud, "8N1", уровень_доступа, пароль, pollingInterval_ms(дефолт 10000)]` |
| **23** | `SPRINKLR` | Irrigation System | Система автоматического полива с многозональным управлением | `[23, {конфиг}]` или `[23, имя, {зоны}]` |

> **Примечание о доступности по целям сборки** (из build-flags): SPRINKLER — due, esp32-wifi, esp32c3-wifi, lighthub21, nrf52840; MERCURY — due, lighthub21, stm32; ELEVATOR — только custom-build mega2560-optiboot; HUMIDIFIER — только custom-build lighthub21; UARTBRIDGE — отключён (закомментирован) во всех стандартных build-flags.

---

## Альтернативное определение типа (текстовое)

Вместо числового кода можно использовать текстовое обозначение типа:

```json
"lamp1": [0, 1]              // числовой код
"lamp1": ["DMX", 1]          // текстовое обозначение
"relay1": ["RELAY", 10]      // текстовое обозначение
```

---

## Примеры конфигурации по типам

### CH_DIMMER (0) - DMX выход с регулировкой

```json
"dimmer1": [0, 5],           // DMX канал 5
"dimmer2": [0, [1, 2, 3, 4]] // 4-канальный (RGBW), DMX 1-4
```

### CH_RGBW (1) - DMX RGB+White

```json
"rgb_light": [1, 10]         // RGB+W на DMX 10-13
```

### CH_RGB (2) - DMX RGB

```json
"rgb_light": [2, 15]         // RGB начиная с 15-го (на DMX 15-17)
```

### CH_PWM (3) - GPIO PWM

```json
"pwm1": [3, 9],              // PWM на GPIO pin 9
"pwm_4ch": [3, [11, 12, 13, 14]] // 4-х канальный PWM
```

### CH_MODBUS (4) - AC Dimmer (Legacy)

```json
"mbus_dim": [4, [96, 0, 0, 255]]
// Адрес: 96
// Регистр: 0
// Маска: 0 (LSB)
// Макс значение: 255
```

### CH_THERMO (5) - Термостат

```json
"thermo_bath": [5, 24, 33]   // GPIO 24, уставка 33°C
```

### CH_RELAY (6) - Реле

```json
"relay1": [6, 23],           // Реле на GPIO 23
"relay2": ["RELAY", 28, 1, 1] // Реле, по умолчанию ON
```

### CH_GROUP (7) - Группа каналов

```json
"lights_all": [7, [
  "lamp1", "lamp2", "lamp3",
  "rgb1", "rgb2"
]],
"lights_bedroom": [7, ["lamp1", "rgb1"]]
```

### CH_VCTEMP (8) - Vacom PID терморегулятор

```json
"fm_t": [8, "fm"]            // строка-имя item типа CH_VC (9)
```

### CH_VC (9) - Vacom мотор регулятор

```json
"fan_speed": [9, [96, "a_ext"]]   // Modbus адрес 96, "имя_объекта" (строка)
```

### CH_AC (10) - Кондиционер Haier

```json
"ac_main": [10, 1]             // номер порта serial (0-3)
// подканалы: fan (LOW/MEDIUM/HIGH), set (16-40), temp, cmd
// также доступны подканалы lock/swing/queit/raw (/mode — число, текстовые режимы — в /cmd)
```

### CH_SPILED (11) - SPI LED лента

```json
"led_strip": [11, [7, 8]]    // data-пин GPIO7, 8 светодиодов
```

### CH_MOTOR (12) - Моторная задвижка с обратной связью

```json
"gate_motor": [12, [9, 10, 11, 0, 255, 30000]]
// pinUp: 9, pinDown: 10, pinFeedback: 11 (аналоговый)
// fbOpen: 0, fbClosed: 255, maxOnTime_ms: 30000
```

### CH_PID (13) - PID регулятор

```json
"pid_temp": [13, [
  [1.0, 0.05, 0.02, 5.0, 3600, 50, 0, 255],
  {"emit": "pid/output"},
  {"emit": "pid/cascade"}
]]
// Kp=1.0, Ki=0.05, Kd=0.02
// dT=5.0 сек, alarm timeout=3600 сек (при отсутствии измерений), alarm value на выходе=50
// min_out=0, max_out=255
```

### CH_MBUS (14) - Универсальный Modbus

```json
"mbus_generic": [14, [96, "temperature_sensor", {
  "temp": {"emit": "sensors/temp"},
  "humidity": {"emit": "sensors/humidity"}
}]]
```

### CH_UARTBRIDGE (15) - UART мост

```json
"uart_debug": [15, {
  "serial": "8N1",
  "baud": 9600,
  "ip": "192.168.1.100",
  "port": 9001,
  "log": true
}]
```

### CH_RELAYX (16) - Медленный PWM через реле

```json
"relay_pwm": [16, [22, 60]]  // GPIO 22, период цикла 60 сек
```

### CH_RGBWW (17) - DMX RGBWW

```json
"led_warm_cold": [17, 30]    // DMX 30-34 (RGBWW, 5 каналов)
```

### CH_MULTIVENT (18) - Многозональная вентиляция

```json
"multivent_system": [18, {
  "": {"val": {"emit": "main/temp"}},
  "bedroom": {
    "val": {"emit": "bed/temp"},
    "fan": {"emit": "bed/fan"},
    "V": 40,
    "pid": [1.0, 0.05, 0.02, 5.0]
  }
}]
```

### CH_COUNTER (20) - Счётчик по импульсам/времени

```json
"energy_meter": [20, [0.02, 1.2]] // коэфф 0.02, период автосуммирования 1.2 с
"gas_counter": [20, 0]             // без калибровки
```

### CH_HUMIDIFIER (21) - Увлажнитель

```json
"humidifier1": [21, [13, 14, 15, 120, 15, 50, 5, 25]]
// [pumpPin, flushPin, pressurePin, maxPumpTime, idleTime, capacity, minLevel, startLevel]
// (только один humidifier в системе; компилируется с HUMIDIFIER_ENABLE)
```

### CH_MERCURY (22) - Счётчик энергии Mercury

```json
"mercury_meter": [22, [1, 9600, "8N1", 2, "123456", 10000]]
// Адрес: 1
// Baud: 9600
// Формат: "8N1"
// Уровень доступа: 2
// Пароль: "123456" (строка из 6 цифр или массив 6 цифр)
// Интервал опроса: 10000 мс (дефолт)
```

### CH_SPRINKLR (23) - Система полива

```json
"irrigation_system": [23, {
  "": {                                // корневой объект: пины платы
    "vIn": 10, "rDren": 11, "rPump": 12,
    "wMax": 13, "wMin": 14,
    "fbDren": 15, "fbPump": 16, "wCtr": 17,
    "onFault": {"exec": "alarm"}
  },
  "north_zone": {"pin": 20},           // зона: только pin
  "south_zone": {"pin": 21}            // cmd/val/set создаются автоматически
}]
// команды: ON, OFF, DRY, RESET
```

---

## Заметки по конфигурации

### Форматы параметров конфигурации

1. **Простой формат** (число или строка):
   ```json
   "item": [тип, параметр]
   ```

2. **Массив параметров**:
   ```json
   "item": [тип, [параметр1, параметр2, параметр3]]
   ```

3. **С начальным значением и командой**:
   ```json
   "item": [тип, конфигурация, начальное_значение, начальная_команда]
   ```

### Поддерживаемые режимы (для каналов с управлением):

- `ON` / `OFF` — включение/отключение
- `REST` / `TOGGLE` / `HALT` — включение после HALT / переключение / выключение
- `XON` / `XOFF` — принудительное включение/выключение
- `INCREASE N` / `DECREASE N` — увеличение/уменьшение (шаг по умолчанию 5)
- `ENABLE` / `DISABLE` / `FREEZE` / `UNFREEZE` — включение/отключение и заморозка канала
- `AUTO` / `HEAT_COOL` / `HEAT` / `COOL` / `DRY` / `FAN_ONLY` — режимы (кондиционер/термостат/вентиляция)
- `HIGH` / `MEDIUM` / `LOW` — скорость вентилятора
- `RGB` / `HSV` / `RESET` — цветовые команды (для RGB каналов) и сброс

Полный список команд: ON OFF REST TOGGLE HALT XON XOFF INCREASE DECREASE ENABLE DISABLE UNFREEZE FREEZE AUTO FAN_ONLY HIGH MEDIUM LOW HEAT_COOL HEAT COOL DRY RGB HSV RESET.

---

## Визуализация иерархии

```
ITEMS (каналы)
├── Digital Output (Реле)
│   ├── CH_RELAY  "RELAY"(6)
│   ├── CH_THERMO "THERMO"(5)
│   ├── CH_RELAYX "RELAYPWM"(16)
│   └── CH_MOTOR  "MOTOR"(12)
│
├── DMX
│   ├── CH_DIMMER "DMX"(0)
│   ├── CH_RGBW   "DMXRGBW"(1)
│   ├── CH_RGB    "DMXRGB"(2)
│   └── CH_RGBWW  "DMXRGBWW"(17)
│
├── Analog Output (PWM)
│   └──  CH_PWM "PWM"(3)
|
├── Каналы с множеством подканалов
│   ├── CH_SPILED     "SPILED"(11)
│   ├── CH_MULTIVENT  "VENTS"(18)
│   ├── CH_SPRINKLER  "SPRINKLR"(23)
│   └── CH_HUMIDIFIER "HUM"(21)
│
├── Modbus/RS485 Slaves/UART
│   ├── CH_MODBUS "MBUSDIM"(4) - Legacy
│   ├── CH_MBUS   "MBUS"(14) - Universal
│   ├── CH_VCTEMP "VCTEMP"(8)
│   ├── CH_VC     "MBUSVC"(9)
│   ├── CH_AC     "ACHAIER"(10)
│   └── CH_MERCURY "MERCURY"(22)
│
├── System/Special
│   ├── CH_GROUP     "GROUP"(7)
│   ├── CH_PID       "PID"(13)
│   ├── CH_COUNTER   "COUNTER"(20)
│   ├── CH_UARTBRIDGE "UARTBRDG"(15)
│   └── CH_ELEVATOR  "ELEVATOR"(19)
```

---

## Полезные ссылки

- [Полное описание конфигурации](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)
- [Описание модулей](modules_description.md)
- [Исходный код item.h](../../lighthub/item.h)
- [Исходный код item.cpp](../../lighthub/item.cpp)
