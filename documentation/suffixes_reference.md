# LightHub: Справочник суффиксов и параметров

> **Инженерный справочник** суффиксов для обращения к различным параметрам каналов в MQTT и при программном управлении.
> Актуально для версии ядра. Источник: [lighthub/item.h](../lighthub/item.h)

---

## Таблица суффиксов

| Код | Суффикс | Назв. констаны | Описание | Применяемость | Пример |
|-----|---------|---|---------|---------|---------|
| **0** | (не используется) | `S_NOTFOUND` | Суффикс не найден или корневой параметр | - | `channel_name` |
| **1** | `/cmd` | `S_CMD` | Команда управления (ON, OFF, SET и др.) | Все типы каналов | `lamp/cmd` → ON |
| **2** | `/set` | `S_SET` | Установка значения для регулирующих каналов | Регулирующие (DMX, PWM, PID) | `dimmer/set` → 150 |
| **3** | `/val` | `S_VAL` | Текущее значение (статус канала) | Все типы каналов | `dimmer/val` → 100 |
| **4** | `/del` | `S_DELAYED` | Отложенная команда (выполнится позже) | Логирование | `lamp/del` |
| **5** | `/HSV` | `S_HSV` | Цвет в формате HSV (Hue, Saturation, Value) | RGB/RGBW/RGBWW каналы | `lamp/HSV` → 120,255,200 |
| **6** | `/RGB` | `S_RGB` | Цвет в формате RGB (Red, Green, Blue) | RGB/RGBW/RGBWW каналы | `lamp/RGB` → 255,128,0 |
| **7** | `/fan` | `S_FAN` | Скорость вентилятора | Multivent, Vacom, AC | `ac/fan` → 2 |
| **8** | `/mode` | `S_MODE` | Режим работы | AC, Multivent, регуляторы | `ac/mode` → HEAT |
| **9** | `/ctrl` | `S_CTRL` | Управление (дублирует функцию /cmd) | Специальные случаи | `channel/ctrl` |
| **10** | `/hue` | `S_HUE` | Оттенок (Hue, 0-359°) | RGB/RGBW/RGBWW каналы | `lamp/hue` → 240 |
| **11** | `/sat` | `S_SAT` | Насыщенность (Saturation, 0-100%) | RGB/RGBW/RGBWW каналы | `lamp/sat` → 80 |
| **12** | `/temp` | `S_TEMP` | Температура цвета (Color Temp, K) | RGB/RGBW/RGBWW каналы | `lamp/temp` → 6500 |
| **13** | `/raw` | `S_RAW` | Сырые данные (itemCmd в JSON формате) | Отладка, интеграция | `channel/raw` → {...} |

---

## Примеры использования по типам каналов

### Реле (CH_RELAY - 6)

```
myhome/dev/relay1/cmd       ← ON / OFF / TOGGLE
myhome/dev/relay1/val       ← 0 или 1 (текущее состояние)
myhome/dev/relay1/set       ← 1 (включить) / 0 (выключить)
```

### DMX/PWM диммер (CH_DIMMER - 0, CH_PWM - 3)

```
myhome/dev/dimmer/cmd       ← ON, OFF, SET
myhome/dev/dimmer/val       ← 0-255 (текущая яркость)
myhome/dev/dimmer/set       ← 100 (установить яркость 100/255)
myhome/dev/dimmer/set       ← 50%  (установить яркость 50%)
myhome/dev/dimmer/set       ← UP / DOWN
```

### RGB светильник (CH_RGB - 2, CH_RGBW - 1, CH_RGBWW - 17)

```
# Управление цветом (HSV формат)
myhome/dev/rgb_lamp/hue     ← 240 (0-359°, синий)
myhome/dev/rgb_lamp/sat     ← 100 (0-100%, насыщенность)
myhome/dev/rgb_lamp/val     ← 200 (0-255, яркость)

# Управление цветом (RGB формат)
myhome/dev/rgb_lamp/RGB     ← 255,0,0 (красный)
myhome/dev/rgb_lamp/RGB     ← 0,255,0 (зелёный)
myhome/dev/rgb_lamp/RGB     ← 0,0,255 (синий)

# RGBW (с белым каналом)
myhome/dev/rgbw_lamp/RGB    ← 255,128,0,100 (RGB + White)

# RGBWW (RGB + тёплый + холодный белый)
myhome/dev/rgbww_lamp/RGB   ← 255,128,0,200,50 (RGB + Warm + Cold)

# Температура цвета
myhome/dev/rgb_lamp/temp    ← 3000 (тёплый, 3000K)
myhome/dev/rgb_lamp/temp    ← 6500 (дневной, 6500K)
```

### Термостат (CH_THERMO - 5)

```
myhome/dev/thermo_bath/cmd  ← ON / OFF
myhome/dev/thermo_bath/val  ← 1 или 0 (статус нагрева)
myhome/dev/thermo_bath/set  ← 25 (установить целевую температуру)
```

### Группа каналов (CH_GROUP - 7)

```
myhome/dev/lights_all/cmd   ← ON (включит ВСЕ люстры в группе)
myhome/dev/lights_all/cmd   ← OFF (выключит ВСЕ люстры в группе)
myhome/dev/lights_all/cmd   ← TOGGLE (переключит ВСЕ)
```

### PID регулятор (CH_PID - 13)

```
myhome/dev/pid_heat/cmd     ← ON / OFF / SET
myhome/dev/pid_heat/val     ← текущее значение выхода (0-255)
myhome/dev/pid_heat/set     ← требуемое значение процесса
```

### Кондиционер Haier (CH_AC - 10)

```
myhome/dev/ac/cmd          ← ON / OFF
myhome/dev/ac/mode         ← COOL / HEAT / FAN / DRY / AUTO
myhome/dev/ac/fan          ← 0, 1, 2, 3 (скорость вентилятора)
myhome/dev/ac/val          ← текущая температура (в помещении)
myhome/dev/ac/set          ← 22 (установить целевую T)
```

### Многозональная вентиляция (CH_MULTIVENT - 18)

```
# Основной контроллер
myhome/dev/vents/cmd       ← ON / OFF
myhome/dev/vents/val       ← текущая T (комната)
myhome/dev/vents/mode      ← режим

# Для зоны "спальня" (через subitems)
myhome/dev/vents/bedroom/cmd     ← управление зоной
myhome/dev/vents/bedroom/val     ← T в спальне
myhome/dev/vents/bedroom/fan     ← открытие жалюзи спальни
myhome/dev/vents/bedroom/set     ← уставка T в спальне
```

### Счётчик импульсов (CH_COUNTER - 20)

```
myhome/dev/energy_meter/val     ← текущее значение (кВт·ч)
myhome/dev/gas_meter/val        ← текущее значение (м³)
```

---

## Специальные команды управления

### Основные команды (работают с большинством каналов)

| Команда | Код | Описание |
|---------|-----|---------|
| `ON` | 1 | Включить |
| `OFF` | 0 | Выключить |
| `TOGGLE` | 2 | Переключить состояние |
| `SET` | 4 | Установить значение |

### Команды изменения значения

| Команда | Описание |
|---------|---------|
| `UP` | Увеличить на 1 |
| `DOWN` | Уменьшить на 1 |
| `INCREASE` | Плавно увеличить |
| `DECREASE` | Плавно уменьшить |

### Команды расписания

| Команда | Описание |
|---------|---------|
| `FREEZE` | Заморозить текущее состояние |
| `UNFREEZE` | Разморозить |
| `HALT` | Остановить (аварийная остановка) |
| `RESTORE` | Восстановить последнее состояние |

---

## Форматирование значений в MQTT

### Числовые значения

```
myhome/dev/dimmer/set       ← 150         # абсолютное значение (0-255)
myhome/dev/dimmer/set       ← 50%         # процент
myhome/dev/dimmer/set       ← UP 10       # с модификатором
```

### Цветовые значения

```
# RGB
myhome/dev/lamp/RGB         ← 255,0,0       # красный
myhome/dev/lamp/RGB         ← #FF0000       # красный (hex)

# HSV
myhome/dev/lamp/HSV         ← 0,100,100     # красный (полная насыщенность)
myhome/dev/lamp/HSV         ← 120,100,100   # зелёный
myhome/dev/lamp/HSV         ← 240,100,100   # синий
```

### Текстовые команды

```
myhome/dev/relay/cmd        ← ON
myhome/dev/relay/cmd        ← OFF
myhome/dev/relay/cmd        ← TOGGLE
myhome/dev/ac/mode          ← HEAT
myhome/dev/ac/mode          ← COOL
```

---

## JSON формат (subitems)

Для каналов с подпараметрами используется специальный JSON формат:

```json
{
  "item": "multizone_ac/bedroom",
  "cmd": "SET",
  "val": 22,
  "suffix": "set"
}
```

Или через MQTT топик:

```
myhome/dev/multizone_ac/bedroom/set  ← 22
```

---

## Приоритет суффиксов

При обработке команды LightHub проверяет суффиксы в следующем порядке:

1. Явный суффикс в команде (`/cmd`, `/val`, `/set`, etc.)
2. Суффикс по умолчанию для типа канала
3. Корневой параметр (если суффиксов нет)

**Пример**:
```
# Команда с явным суффиксом имеет приоритет
myhome/dev/lamp/cmd  ← ON          # выполнится как команда ON

# Без суффикса используется суффикс по умолчанию
myhome/dev/lamp     ← ON           # может интерпретироваться как SET
```

---

## Статус канала (ответ контроллера)

Контроллер публикует текущий статус в топик:

```
myhome/s_out/<item_name>/<suffix>
```

Пример:
```
myhome/s_out/lamp1/val              # яркость лампы 1
myhome/s_out/lamp1/cmd              # последняя выполненная команда
myhome/s_out/rgb_lamp/hue           # текущий оттенок
myhome/s_out/rgb_lamp/sat           # текущая насыщенность
myhome/s_out/ac/mode                # текущий режим кондиционера
```

---

## Особенности отдельных типов

### RGB каналы с коррекцией белого (RGBWW)

**RGBWW формат**: `R,G,B,WarmWhite,ColdWhite`

```
# Пример: розовый свет (красный + тёплый белый)
myhome/dev/led/RGB  ← 255,0,0,100,0

# Нейтральный белый (комбинация тёплого и холодного)
myhome/dev/led/RGB  ← 0,0,0,128,128

# Холодный белый
myhome/dev/led/RGB  ← 0,0,0,0,255
```

### Multivent суффиксы для отдельных зон

Для зон используются дополнительные параметры:

```
myhome/dev/multivent/bedroom/fan    ← 50     # открытие жалюзи
myhome/dev/multivent/bedroom/mode   ← HEAT   # режим нагрева
myhome/dev/multivent/bedroom/set    ← 23     # уставка температуры
myhome/dev/multivent/bedroom/val    ← текущая T в спальне
```

---

## Отладка через /raw суффикс

Для отладки и развёрнутого логирования можно использовать суффикс `/raw`:

```
myhome/dev/lamp/raw  ← получить полное описание состояния в JSON
```

Ответ:
```json
{
  "cmd": 1,
  "val": 150,
  "type": 0,
  "arg": [1],
  "suffix": 0
}
```

---

## Таблица соответствия суффиксов и типов каналов

| Тип | /cmd | /val | /set | /hue | /sat | /temp | /fan | /mode |
|-----|:----:|:----:|:----:|:----:|:----:|:-----:|:----:|:-----:|
| RELAY (6) | ✓ | ✓ | - | - | - | - | - | - |
| DMX (0) | ✓ | ✓ | ✓ | - | - | - | - | - |
| PWM (3) | ✓ | ✓ | ✓ | - | - | - | - | - |
| RGB (2) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - |
| RGBW (1) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - |
| RGBWW (17) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | - | - |
| THERMO (5) | ✓ | ✓ | ✓ | - | - | - | - | - |
| PID (13) | ✓ | ✓ | ✓ | - | - | - | - | - |
| AC (10) | ✓ | ✓ | ✓ | - | - | - | ✓ | ✓ |
| MULTIVENT (18) | ✓ | ✓ | ✓ | - | - | - | ✓ | ✓ |
| GROUP (7) | ✓ | ✓ | - | - | - | - | - | - |
| COUNTER (20) | - | ✓ | - | - | - | - | - | - |
| MOTOR (12) | ✓ | ✓ | ✓ | - | - | - | - | - |

---

## Примеры реальных MQTT команд

### Включение всех светильников в комнате

```
myhome/dev/lights_hall/cmd  ON
```

### Установка яркости на 50%

```
myhome/dev/dimmer1/set  128
```

### Установка цвета RGB лампы на синий с 80% яркостью

```
myhome/dev/lamp_rgb/hue  240
myhome/dev/lamp_rgb/sat  100
myhome/dev/lamp_rgb/val  200
```

### Включение кондиционера в режим охлаждения при 22°C

```
myhome/dev/ac/cmd   ON
myhome/dev/ac/mode  COOL
myhome/dev/ac/set   22
```

### Нагрев в спальне многозональной системы до 23°C

```
myhome/dev/multivent/bedroom/set  23
myhome/dev/multivent/bedroom/mode HEAT
```

---

## Полезные ссылки

- [Справочник типов каналов](channel_types_reference.md)
- [Полное описание конфигурации](light_hub_полное_инженерное_описание_json_конфигурации.md)
- [Описание модулей](modules_description.md)
- [Исходный код item.h](../lighthub/item.h)
