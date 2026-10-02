# LightHub: Полная техническая таблица типов каналов

> **Инженерный справочник** — полная таблица всех типов каналов (0-23) с параметрами, синтаксисом и примерами кода.
> Источник: [lighthub/item.h](../../lighthub/item.h), строки 48-72

---

## Быстрая таблица типов

| Код | Тип | Англ. | Рус. описание | Конф. тип | Примеры параметров |
|-----|-----|-------|---------|-----------|-------------------|
| 0 | DMX | DMX Dimmer | DMX одного или нескольких каналов | int/array | `1` или `[1,2,3,4]` |
| 1 | DMXRGBW | DMX RGBW | DMX RGB+White (4 канала) | int | `10` (DMX 10-13) |
| 2 | DMXRGB | DMX RGB | DMX RGB (3 канала) | int | `15` (DMX 15-17) |
| 3 | PWM | PWM Output | GPIO PWM (1-5 каналов) | int/array | `9` или `[11,12,13,14]` |
| 4 | MBUSDIM | Modbus Dimmer | AC диммер Modbus (Legacy) | array | `[96, 0, 0, 255]` |
| 5 | THERMO | Thermostat | ON/OFF термостат | array | `[24, 33]` (GPIO, T°C) |
| 6 | RELAY | Relay Output | GPIO реле | int | `23` (GPIO pin) |
| 7 | GROUP | Group Channel | Логическая группа каналов | array | `["lamp1", "lamp2"]` |
| 8 | VCTEMP | Vacom Temp | Температура из VC-канала | string | `"fm"` (имя CH_VC канала) |
| 9 | MBUSVC | Vacom Motor | Vacom мотор регулятор | array | `[96, "a_ext"]` (адрес, имя) |
| 10 | ACHAIER | AC Haier | Кондиционер Haier (UART) | int | `1` (порт 0-3) |
| 11 | SPILED | SPI LED | NeoPixel LED лента | array | `[7, 8]` (data-пин 7, 8 светодиодов) |
| 12 | MOTOR | Motor | Моторная задвижка с обратной связью | array | `[9, 10, 11, 0, 255, 30000]` |
| 13 | PID | PID Regulator | PID регулятор | array | `[[1.0,0.05,...],{...}]` |
| 14 | MBUS | Modbus | Universal Modbus v2 | array | `[96, "template", {...}]` |
| 15 | UARTBRDG | UART Bridge | UART мост | object | `{"serial","baud","ip","port","hd","log","bond"}` |
| 16 | RELAYX | Relay PWM | Медленный PWM реле | array | `[22, 60]` (GPIO, период_сек) |
| 17 | DMXRGBWW | DMX RGBWW | DMX RGB+2White (5 каналов) | int | `30` (DMX 30-34) |
| 18 | VENTS | Multivent | Многозональная вентиляция | object | `{зоны}` (без Modbus-адреса) |
| 19 | ELEVATOR | Elevator | Лифт (реализован) | object | `{"serial":"8N1","baud":19200}` |
| 20 | COUNTER | Counter | Счётчик импульсов | int/array | `0` или `[0.02, 1.2]` |
| 21 | HUM | Humidifier | Увлажнитель (один в системе) | array | `[[13,14,15,120,15,50,5,25], {зоны}]` |
| 22 | MERCURY | Mercury Meter | Счётчик энергии Mercury | array | `[1, 9600, "8N1", 2, "123456"]` |
| 23 | SPRINKLR | Irrigation System | Система полива | object/array | `{конфиг}` или `[имя, {зоны}]` |

---

## Определения из item.h

```cpp
#define CH_DIMMER 0      //DMX 1-4 ch
#define CH_RGBW   1      //DMX 4 ch
#define CH_RGB    2      //DMX 3 ch
#define CH_PWM    3      //PWM output directly to PIN 1-4 CH
#define CH_MODBUS 4      //Modbus AC Dimmer
#define CH_THERMO 5      //Simple ON/OFF thermostat
#define CH_RELAY  6      //ON_OFF relay output
#define CH_GROUP   7     //Group pseudochannel
#define CH_VCTEMP  8     //Vacom PID regulator
#define CH_VC      9     //Vacom modbus motor regulator
#define CH_AC 10         //AC Haier
#define CH_SPILED 11
#define CH_MOTOR  12
#define CH_PID   13
#define CH_MBUS  14
#define CH_UARTBRIDGE  15
#define CH_RELAYX 16
#define CH_RGBWW    17
#define CH_MULTIVENT 18
#define CH_ELEVATOR 19
#define CH_COUNTER 20
#define CH_HUMIDIFIER 21
#define CH_MERCURY 22
#define CH_SPRINKLER 23
#define CH_MAX 23
```

---

## Детальная таблица (с командами и суффиксами)

### CH_DIMMER (0) — DMX Диммер

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление яркостью 1-5 DMX каналов (3=RGB, 4=RGBW, 5=RGBWW) |
| **Конфигурация** | int (один канал) или array (несколько) |
| **Формат конфига** | `[0, 1]` или `[0, [1,2,3,4]]` |
| **Диапазон значений** | 0-255 (яркость) |
| **Поддерживаемые команды** | ON, OFF, TOGGLE, INCREASE, DECREASE (+ REST, HALT, XON, XOFF) |
| **Суффиксы** | /cmd, /val, /set |
| **Начальное значение** | Опционально (элемент 3) |
| **MQTT примеры** | `myhome/<имя_контроллера>/lamp/cmd ← ON`, `myhome/<имя_контроллера>/lamp/set ← 150` (индивидуальный командный топик) |

### CH_RGBW (1) — DMX RGB+White

| Параметр | Значение |
|----------|----------|
| **Назначение** | RGB + White через DMX 512 (4 канала) |
| **Конфигурация** | int (стартовый DMX канал) |
| **Формат конфига** | `[1, 10]` (DMX каналы 10-13) |
| **Диапазон значений** | 0-255 (для каждого канала) |
| **Суффиксы** | /cmd, /val, /set, /hue, /sat, /temp, /RGB |
| **MQTT формат RGB** | `255,128,0` (R,G,B) |
| **MQTT формат RGBW** | `255,128,0,100` (R,G,B,W) |
| **MQTT команды** | `/HSV`, `/RGB`, `/hue`, `/sat`, `/temp` |

### CH_RGB (2) — DMX RGB

| Параметр | Значение |
|----------|----------|
| **Назначение** | RGB через DMX 512 (3 канала) |
| **Конфигурация** | int (стартовый DMX канал) |
| **Формат конфига** | `[2, 15]` (DMX каналы 15-17) |
| **Суффиксы** | /cmd, /val, /set, /hue, /sat, /temp, /RGB |

### CH_PWM (3) — GPIO PWM

| Параметр | Значение |
|----------|----------|
| **Назначение** | ШИМ на GPIO пины Arduino/ESP |
| **Конфигурация** | int (один pin) или array (несколько) |
| **Формат конфига** | `[3, 9]` или `[3, [11, 12, 13]]` |
| **GPIO диапазон** | 0-53 (зависит от платы) |
| **Диапазон PWM** | 0-255 |
| **Частота** | 4000 Гц (Mega2560, таймеры 3/4); на ESP32 PWM недоступен (stub) |
| **Суффиксы** | /cmd, /val, /set |

### CH_MODBUS (4) — AC Dimmer (Legacy)

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление AC-диммером через Modbus (устаревшее) |
| **Конфигурация** | array `[addr, reg, mask, max_val]` |
| **Параметры** | - addr: Modbus адрес (1-247)<br>- reg: регистр (обычно 0)<br>- mask: маска (0, 1, 2 (пропуск 16 бит), 3 (Modbus relay), 4 (swap байтов), -1/отсутствует — без поллинга)<br>- max_val: максимальное значение (обычно 255) |
| **Примечание** | **Использовать CH_MBUS (14) для новых проектов** |

### CH_THERMO (5) — ON/OFF Термостат

| Параметр | Значение |
|----------|----------|
| **Назначение** | Простой ON/OFF термостат с гистерезисом |
| **Конфигурация** | array `[gpio_pin, target_temp_C]` |
| **GPIO диапазон** | 0-53 (в зависимости от платы) |
| **Температура** | Уставка ограничена ≤80 °C |
| **Гистерезис** | Включение при T<уставка−1, выключение при T≥уставка |
| **MQTT команды** | `/set` (целевая T), `/val` (вход текущей температуры) |

### CH_RELAY (6) — GPIO Реле

| Параметр | Значение |
|----------|----------|
| **Назначение** | Электромагнитное реле ON/OFF |
| **Конфигурация** | int (GPIO pin) или array `[gpio, val, cmd]` |
| **GPIO диапазон** | 0-53 |
| **Выходное напряжение** | 5V или 3.3V (в зависимости от платы) |
| **Макс. ток** | 40 мА на pin (требуется транзистор для реле) |
| **Состояния** | 0 = OFF, 1 = ON |

### CH_GROUP (7) — Группа каналов

| Параметр | Значение |
|----------|----------|
| **Назначение** | Логическая группа для синхронного управления |
| **Конфигурация** | array с именами каналов |
| **Формат конфига** | `[7, ["lamp1", "lamp2", "rgb1"]]` |
| **Команды** | ON, OFF, TOGGLE, SET (для диммеров) |
| **Рекурсия** | Поддерживается (группы в группах) |

### CH_VCTEMP (8) — Vacom PID Терморегулятор

| Параметр | Значение |
|----------|----------|
| **Назначение** | Публикация температуры из VC-канала Vacom |
| **Конфигурация** | string — имя_канала_VC (itemArg — строка-имя item типа CH_VC (9)) |
| **Пример** | `[8, "fm"]` |

### CH_VC (9) — Vacom Мотор-регулятор

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление мотор-регулятором вентилятора |
| **Конфигурация** | array `[modbus_адрес, "имя_объекта"]` |
| **Пример** | `[9, [96, "a_ext"]]` |

### CH_AC (10) — Кондиционер Haier

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление кондиционером Haier |
| **Конфигурация** | `[10, portNum(0-3)]` |
| **Порт** | Номер serial порта (0-3) |
| **Режимы** | AUTO, FAN_ONLY, HEAT, COOL, DRY, HEAT_COOL (текст — в payload /cmd; /mode — число); fan: LOW/MEDIUM/HIGH; /set 16-40 |
| **Требует** | Проприетарный UART-протокол Haier (9600, кадры 13/37 байт) |

### CH_SPILED (11) — SPI LED Лента

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление адресуемой NeoPixel LED лентой |
| **Конфигурация** | array `[data_pin, numLeds (дефолт 43), ledsType]` — CLK-пина нет |
| **Протокол** | NeoPixel (один data-пин); по умолчанию Adafruit NeoPixel, при FASTLED — TM1809 |
| **Диапазон пинов** | 0-53 |

### CH_MOTOR (12) — Моторная задвижка с обратной связью

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление моторной задвижкой с обратной связью (позиционная, цель 0-255) |
| **Конфигурация** | array `[pinUp, pinDown, pinFeedback, fbOpen, fbClosed, maxOnTime_ms]` (PWM-пина в конфиге нет) |
| **pinUp/pinDown** | Пины движения в стороны открытия/закрытия |
| **pinFeedback** | Аналоговый датчик положения; fbOpen/fbClosed — калибровка обратной связи |
| **maxOnTime** | Максимальное время движения (мс, дефолт 10000) |
| **Команды** | ON — к заданной позиции, OFF — в позицию 0, HALT |

### CH_PID (13) — PID Регулятор

| Параметр | Значение |
|----------|----------|
| **Назначение** | Универсальный PID контроллер |
| **Конфигурация** | array `[[Kp, Ki, Kd, dT, timeout, alarm, min, max], {...exec}, {...exec}]` |
| **Kp** | Коэффициент пропорциональности |
| **Ki** | Коэффициент интеграла |
| **Kd** | Коэффициент дифференциала |
| **dT** | Интервал расчёта (сек) |
| **timeout** | Время до аварии (сек) |
| **alarm** | Значение аварии |
| **min/max** | Диапазон выхода |

### CH_MBUS (14) — Universal Modbus

| Параметр | Значение |
|----------|----------|
| **Назначение** | Универсальный Modbus канал с полной поддержкой |
| **Конфигурация** | array `[addr, "template", {params}]` |
| **Addr** | Modbus адрес устройства |
| **Template** | Имя шаблона из секции "modbus" |
| **Params** | Привязка параметров к MQTT/items |
| **Тип данных** | i16, i32, u16, u32, i8h, i8l, u8h, u8l, x10, 100 |

### CH_UARTBRIDGE (15) — UART Мост

| Параметр | Значение |
|----------|----------|
| **Назначение** | Мост между двумя UART портами |
| **Конфигурация** | object `{serial, baud, ip, port, hd, log, bond}` (порты фиксированы) |
| **Отладка** | Может логировать через UDP |

### CH_RELAYX (16) — Медленный PWM через реле

| Параметр | Значение |
|----------|----------|
| **Назначение** | Медленный PWM для инертных систем |
| **Конфигурация** | array `[gpio_pin, period_seconds]` |
| **GPIO pin** | Управление реле |
| **Период** | Цикл коммутации в секундах (дефолт 5 с, ограничений нет) |
| **Применение** | Система напольного отопления, тепловые системы |

### CH_RGBWW (17) — DMX RGBWW

| Параметр | Значение |
|----------|----------|
| **Назначение** | RGB + Тёплый белый + Холодный белый |
| **Конфигурация** | int (стартовый DMX) |
| **Каналов DMX** | 5 (R, G, B, W_warm, W_cold) |
| **Формат** | DMX 30-34 (5 каналов) |

### CH_MULTIVENT (18) — Многозональная вентиляция

| Параметр | Значение |
|----------|----------|
| **Назначение** | Каскадная система вентиляции с зонами |
| **Конфигурация** | объект зон `[18, {zones}]` (Modbus-адреса нет) |
| **Зоны** | Независимое регулирование T по комнатам |
| **PID** | Для каждой зоны свои коэффициенты |
| **itemArg** | объект зон (не массив с адресом) |

### CH_ELEVATOR (19) — Лифт

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление лифтом |
| **Конфигурация** | `[19, {"serial":"8N1","baud":19200,"log":...}]` |
| **Статус** | Реализован; компилируется с ELEVATOR_ENABLE (включён только в custom-build mega2560-optiboot) |

### CH_COUNTER (20) — Счётчик импульсов

| Параметр | Значение |
|----------|----------|
| **Назначение** | Счётчик импульсов (кВт·ч, м³, л) |
| **Конфигурация** | int или array `[коэффициент_импульса, период_сек]` |
| **Coefficient** | Кол-во единиц на один импульс |
| **Период** | Период автосуммирования (сек) |
| **Вход** | Внешние импульсы (GPIO-входа в конфиге нет) |

### CH_HUMIDIFIER (21) — Увлажнитель

| Параметр | Значение |
|----------|----------|
| **Назначение** | Управление увлажнителем воздуха (только один в системе) |
| **Конфигурация** | array `[[pumpPin, flushPin, pressurePin, maxPumpTime, idleTime, capacity, minLevel, startLevel], {зоны_краны}, motorExecObj, faultExecObj]` |
| **Сборка** | Компилируется с HUMIDIFIER_ENABLE (custom lighthub21) |

### CH_MERCURY (22) — Счётчик энергии Mercury

| Параметр | Значение |
|----------|----------|
| **Назначение** | Счётчик электроэнергии Mercury по RS485 |
| **Конфигурация** | array `[addr, baud, serial, уровень_доступа, пароль, pollingInterval]` |
| **Addr** | Modbus адрес счётчика (1-247) |
| **Baud** | 9600 (стандарт) |
| **Serial** | "8N1" (стандарт) |
| **Уровень доступа** | Уровень доступа к счётчику |
| **Пароль** | Строка из 6 цифр или массив 6 цифр |
| **PollingInterval** | Интервал опроса (мс, дефолт 10000) |

### CH_SPRINKLER (23) — Система полива

| Параметр | Значение |
|----------|----------|
| **Назначение** | Многозональная система автоматического полива (строковое имя типа — SPRINKLR) |
| **Конфигурация** | `[23, {конфиг}]` или `[23, имя, {зоны}]` |
| **Корневой объект ("" )** | пины vIn, rDren, rPump, wMax, wMin, fbDren, fbPump, wCtr + onFault(execObj) |
| **Параметры зоны** | pin (+ автоматически создаваемые cmd/val/set) |
| **Команды** | ON, OFF, DRY, RESET (+ FREEZE/UNFREEZE/ENABLE/DISABLE ядра) |
| **События** | Изменение состояния, завершение полива, ошибки |
| **Интеграция** | MQTT топики для каждой зоны |

---



Из [item.h](../../lighthub/item.h):

```cpp
#define S_NOTFOUND  0
#define S_CMD  1      // /cmd
#define S_SET  2      // /set
#define S_VAL  3      // /val
#define S_DELAYED 4   // /del
#define S_HSV  5      // /HSV
#define S_RGB  6      // /RGB
#define S_FAN  7      // /fan
#define S_MODE 8      // /mode
#define S_CTRL 9      // /ctrl
#define S_HUE  10     // /hue
#define S_SAT  11     // /sat
#define S_TEMP 12     // /temp
#define S_RAW 13      // /raw
```

---

## Таблица совместимости MQTT суффиксов и типов

| Тип | /cmd | /val | /set | /hue | /sat | /temp | /fan | /mode | /RGB |
|-----|:----:|:----:|:----:|:----:|:----:|:-----:|:----:|:-----:|:----:|
| 0 (DMX) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 1 (RGBW) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - | ✓ |
| 2 (RGB) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - | ✓ |
| 3 (PWM) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 4 (MBUSDIM) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 5 (THERMO) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 6 (RELAY) | ✓ | ✓ | - | - | - | - | - | - | - |
| 7 (GROUP) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 10 (AC) | ✓ | ✓ | ✓ | - | - | - | ✓ | ✓ | - |
| 13 (PID) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 14 (MBUS) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 16 (RELAYX) | ✓ | - | ✓ | - | - | - | - | - | - |
| 23 (SPRINKLR) | ✓ | ✓ | ✓ | - | - | - | - | - | - |
| 17 (RGBWW) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - | ✓ |
| 18 (MULTIVENT) | ✓ | ✓ | ✓ | - | - | - | ✓ | ✓ | - |
| 20 (COUNTER) | ✓ | ✓ | ✓ | - | - | - | - | - | - |

---

## Константы и флаги

### Флаги управления (CTRL Execution flags)

```cpp
#define CTRL_DISABLE_RECURSION 1
#define CTRL_DISABLE_NON_GRP  2
#define CTRL_SCHEDULED_CALL_RECURSION (CTRL_DISABLE_RECURSION | CTRL_DISABLE_NON_GRP)
```

### Флаги режима опроса

```cpp
#define POLLING_SLOW 1
#define POLLING_FAST 2
#define POLLING_INT  3
#define POLLING_1S   4
```

### Индексы массива Item

```cpp
#define I_TYPE 0       // Type of item
#define I_ARG  1       // Channel-type depended argument
#define I_VAL  2       // Latest preset (int or array)
#define I_CMD  3       // Latest CMD received
#define I_EXT  4       // Channel-depended extension
#define I_TIMESTAMP 5
```

---

## Примеры JSON конфигурации

```json
{
  "items": {
    "relay1": [6, 23],
    "dimmer1": [0, 1],
    "rgb_light": [1, 10],
    "pwm1": [3, 9],
    "thermo": [5, 24, 33],
    "group_lights": [7, ["relay1", "dimmer1", "rgb_light"]],
    "ac_main": [10, 1],
    "counter_energy": [20, [0.02, 1.2]],
    "multivent": [18, {"bedroom": {"set": 22}}]
  }
}
```

---

---

## Доступность типов по целям сборки

| Тип | Цели сборки (build-flags) |
|-----|---------------------------|
| SPRINKLER (23) | due, esp32-wifi, esp32c3-wifi, lighthub21, nrf52840 |
| MERCURY (22) | due, lighthub21, stm32 |
| ELEVATOR (19) | только custom-build mega2560-optiboot (ELEVATOR_ENABLE) |
| HUMIDIFIER (21) | только custom-build lighthub21 (HUMIDIFIER_ENABLE) |
| UARTBRIDGE (15) | закомментирован во всех стандартных build-flags (UARTBRIDGE_ENABLE) |

---

**Версия**: 2.0  
**Дата**: 2025-01-24  
**Источник**: [item.h](../../lighthub/item.h) (строки 48-72)
