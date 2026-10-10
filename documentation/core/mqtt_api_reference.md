# LightHub: Справочник MQTT API и топиков

> **Инженерный справочник** структуры MQTT топиков и HTTP API контроллера LightHub.
> Источник: [wiki.lazyhome.ru - работа с MQTT](https://www.lazyhome.ru/dokuwiki/doku.php?id=%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D0%B0_%D1%81_mqtt) и [api](https://www.lazyhome.ru/dokuwiki/doku.php?id=api)

---

## Структура MQTT топиков

### Общий формат топика

```
root/[id-устройства или bcst или out]/имя_item/[subitem/]suffix
```

### Компоненты топика

#### 1. **root** — корневой префикс
- **Назначение**: разделение систем при одном брокере
- **По умолчанию**: `myhome`
- **Пример**: `myhome`

#### 2. **Адресация** (второй уровень)

**Широковещательные (broadcast) командные топики**:
- `bcst` — команда выполняется на всех контроллерах с одинаковым `bcst` именем
- **Пример**: `myhome/in/lamp1/cmd` (где `in` — это значение `bcst`)

**Индивидуальные командные топики**:
- `id-устройства` — имя конкретного контроллера из конфигурации MQTT
- **Пример**: `myhome/lighthub01/lamp1/cmd`

**Статусные топики**:
- `out` — контроллер публикует статус в эти топики
- **Пример**: `myhome/s_out/lamp1/set` (где `s_out` — значение `out`)

#### 3. **item\_name** — имя объекта (канала)

Определяется в разделе `items` конфигурации.

```json
"items": {
  "lamp_bedroom": [0, 1],
  "ac_main": [10, {...}]
}
```

**Пример топиков для item `lamp_bedroom`**:
- Команда: `myhome/in/lamp_bedroom/cmd`
- Статус: `myhome/s_out/lamp_bedroom/set` (0-255) + общий `myhome/s_out/lamp_bedroom` (0-100)

#### 4. **subitem** (опционально) — подпараметр

Для каналов с множественными элементами (например, адресуемая LED лента, многозональная вентиляция).

**Пример для многозональной вентиляции**:
```
myhome/in/multivent/bedroom/set      (установить T в спальне)
myhome/in/multivent/kitchen/fan      (управление вентилятором кухни)
```

**Пример для LED ленты (пиксели 10-20; работает ТОЛЬКО для CH\_SPILED, тип 11 — DMX-драйвер subitem игнорирует)**:
```
myhome/in/led_strip/10-20/set        (установить значение пиксели 10-20)
```

**Пример для состояния (условное выполнение)**:

```
myhome/in/floor/AUTO/set             (применится для теплых полов в режиме AUTO)
myhome/in/floor/OFF/set              (применится для теплых полов в режиме OFF)
```

#### 5. **suffix** (суффикс) — параметр объекта

Определяет **какое** свойство объекта меняется.

---

## Таблица суффиксов

| Суффикс | Тип | Назначение | Применимость |
|---------|-----|-----------|--------------|
| **`/cmd`** | Command, Status | Основная команда управления (ON, OFF, TOGGLE и др.) | Все типы |
| **`/set`** | Command, Status | Установка параметра (яркость, температура и др.) | Диммеры, регуляторы, термостаты |
| **`/val`** | Command | Вход сенсора: значение регулируемого параметра направляется с сенсора сюда | THERMO, PID, MULTIVENT, SPRINKLER|
| **`/hue`** | Command | Оттенок (0-365°, HSV формат) | RGB/RGBW/RGBWW |
| **`/sat`** | Command | Насыщенность (0-100%, HSV формат) | RGB/RGBW/RGBWW |
| **`/HSV`** | Command | Полный цвет (hue,saturation,volume). Суффикс ЗАГЛАВНЫЙ, строчный `/hsv` игнорируется | RGB/RGBW/RGBWW |
| **`/rgb`** | Command | **Не обрабатывается** в текущей сборке (S\_RGB) | — |
| **`/temp`** | Command | Цветовая температура 153-500 mireds (RGBWW); у AC — текущая T | RGBWW, AC |
| **`/raw`** | Command | «Сырые» данные, передаваемые драйверу без интерпретации | AC, ELEVATOR |
| **`/fan`** | Command, Status | Скорость вентилятора 0-255 или (HIGH, MEDIUM, LOW, OFF) | AC, Multivent |
| **`/mode`** | Command, Status | Режим работы — число 0-4 (AC). Команды FAN нет — есть FAN\_ONLY. Текстовые режимы (HEAT, COOL, DRY, AUTO) — payload в /cmd | AC, Multivent |
| **`/lock`** | Command, Status | Блокировка (ON, OFF) | AC |
| **`/swing`** | Command, Status | Направление воздушного потока (ON, OFF) | AC |
| **`/quiet`** | Command, Status | Тихий режим (ON, OFF) | AC |
| **`/ctrl`** | Command, Status | Управление состоянием (FREEZE, UNFREEZE, ENABLE, DISABLE) | Специальное |
| **`/del`** | Command | Команда с задержкой — только команды ON…FAN\_ONLY (код ≤ 0x0F); HIGH/MEDIUM/LOW/HEAT\_COOL/HEAT/COOL/DRY запланировать нельзя | ON…FAN\_ONLY |

---

## Три типа топиков

### 1. Командные топики (INPUT) — контроллер получает

#### Широковещательные (broadcast)
```
root/bcst/item[/subitem]/suffix
```

**Примеры** (при bcst="in"):
```
myhome/in/lamp1/cmd          ← ON
myhome/in/lamp1/set          ← 150
myhome/in/ac_main/cmd        ← HEAT
myhome/in/multivent/bedroom/set  ← 22
```

**Использование**: команда выполняется на **всех контроллерах**, у которых одинаковое имя контроллера (bcst).

#### Индивидуальные
```
root/id-устройства/item[/subitem]/suffix
```

**Примеры** (при id-устройства="lighthub01"):
```
myhome/lighthub01/lamp1/cmd  ← ON
myhome/lighthub01/lamp1/set  ← 150
```

**Использование**: команда выполняется на **конкретном контроллере**.

### 2. Статусные топики (OUTPUT) — контроллер публикует

```
root/out/item[/subitem]/suffix
```

**Примеры** (при out="s\_out"):
```
myhome/s_out/lamp1/set          → 255 (текущая яркость, 0-255; общий топик s_out/lamp1 — 0-100)
myhome/s_out/lamp1/cmd          → ON (последняя команда)
myhome/s_out/ac_main/mode       → <0-4> (число; текстовый режим «HEAT» публикуется в s_out/ac_main/cmd)
myhome/s_out/ac_main/set        → 22 (установленная T)
myhome/s_out/multivent/bedroom/set  → 21.0 (статус зоны — в суффикс ввода: /set, /fan, /cmd; /val нет)
```

**Свойства**:
- Публикуются с флагом **PERSISTENT** (помощь при отключении-подключении)
- При старте контроллер **восстанавливает состояние** из этих топиков
- При локальном изменении (через входы) тоже обновляются

### 3. Служебные топики (SERVICE)

**Формат**: `root/id-устройства/$command` и `root/id-устройства/$stats`

#### `$command`
```
myhome/lighthub01/$command
```
В payload записываются **CLI команды** (как если бы через последовательный порт):
- `reboot`
- `save`
- `get`
- `load` и др.

#### `$stats` (два retained-топика, публикуются каждые 30 сек)
```
myhome/lighthub01/$stats/uptime    → 12345     (uptime, сек)
myhome/lighthub01/$stats/freeheap  → 2500      (свободная RAM, байт)
```
При подключении в `myhome/lighthub01/$stats` публикуется список доступных метрик: `uptime,freeheap`.

#### `$state` (публикуется контроллером)
```
myhome/lighthub01/$state  → "ready" или "disconnected"
```

#### Homie-топики

Контроллер публикует также (retained) homie-топики: `$homie`, `$name`, `$state`, `$nodes`, `$fw/*` и `<item>/cmd/$datatype` для каждого item — для совместимости с Homie-клиентами.

---

## Примеры конфигурации MQTT

### Базовая конфигурация

```json
{
  "mqtt": ["LHexample03", "test.mosquitto.org", 1883, "user", "password"],
  "topics": {"root": "myhome", "bcst": "in", "out": "s_out"}
}
```

### Результирующие топики

**Командные широковещательные**:
```
myhome/in/<item>
myhome/in/<item>/set
myhome/in/<item>/cmd
myhome/in/<item>/<суффикс>
```

**Командные индивидуальные**:
```
myhome/LHexample03/<item>
myhome/LHexample03/<item>/set
myhome/LHexample03/<item>/cmd
myhome/LHexample03/<item>/<суффикс>
```

**Статусные**:
```
myhome/s_out/<item>
myhome/s_out/<item>/set
myhome/s_out/<item>/cmd
myhome/s_out/<item>/<суффикс>
```

---

## Команды и инструкции (Payload)

### Базовые команды (совместимы с OpenHab)

| Команда | Описание | Пример |
|---------|---------|--------|
| `ON` | Включить (восстанавливает последнее значение) | `myhome/in/lamp1/cmd → ON` |
| `OFF` | Выключить | `myhome/in/lamp1/cmd → OFF` |
| `TOGGLE` | Переключить | `myhome/in/lamp1/cmd → TOGGLE` |
| `0..255` | Установить яркость/значение. `/set` всегда 0-255; шкала 0-100 — только в общем топике без суффикса | `myhome/in/lamp1/set → 50` |

### Числовые команды

| Формат | Описание | Пример |
|--------|---------|--------|
| `<0..100>` | Яркость (OpenHab стиль) | `myhome/in/lamp → 50` |
| `<0..255>` | Яркость/PWM/температура/значение счетчика/... (новый стиль) | `myhome/in/lamp/set → 128` |
| `<H>,<S>,<V>` | HSV формат (суффикс ЗАГЛАВНЫЙ `/HSV`) | `myhome/in/led/HSV → 240,100,200` |
| `<H>,<S>` | HSV без изменения яркости | `myhome/in/led/HSV → 240,100` |
| `#RRGGBB` | RGB hex (Home Remote) — **не работает** в текущей сборке (hex не парсится) | — |
| `<H>,<S>,<V>[,<CT>]` | Строка в общем топике (без суффикса) трактуется как **HSV(+colorTemp)**, а не RGB/RGBW | `myhome/in/led → 255,0,0,100` = HSV+CT |

### Расширенные команды

| Команда | Описание | Применимость |
|---------|---------|--------------|
| `HALT` | Выключить | Все |
| `REST` | Включить (если был выключен HALT) | Все |
| `XON` | Включить (если не DISABLE) | Все |
| `XOFF` | Выключить (если был включен XON) | Все |
| `INCREASE N` или `%+N` | Увеличить на N пунктов | Диммеры, PWM |
| `DECREASE N` или `%-N` | Уменьшить на N пунктов | Диммеры, PWM |
| `ENABLE` | Разрешить управление (XON, DISABLE) | PID, все |
| `DISABLE` | Запретить управление (XON, DISABLE) | PID, все |
| `FREEZE` | Заблокировать канал (игнорирует команды) | Все |
| `UNFREEZE` | Разблокировать канал | Все |

### Команды AC и Thermostat

| Команда | Описание | Применимость |
|---------|---------|--------------|
| `AUTO` | Автоматический режим | AC, Thermostat, Multivent |
| `HEAT` | Нагрев | AC, Thermostat, Multivent  |
| `HEAT_COOL` | Нагрев/охлаждение (автоматически по температуре) | AC, Multivent |
| `COOL` | Охлаждение | AC, Multivent  |
| `DRY` | Осушение | AC, Multivent, Sprinkler  |
| `FAN_ONLY` | Только вентилятор (команды FAN не существует) | AC, Multivent  |
| `HIGH`, `MEDIUM`, `LOW` | Скорость вентилятора (MED — неверно) | AC, MULTIVENT |
| `RESET` | Сброс параметров канала | Все |

### Команды с задержкой

**Синтаксис**: `КОМАНДА ВРЕМЯ_МС` (в топик с суффиксом `/del`)

```
myhome/in/lamp/del → "ON 5000"        (включить через 5 сек)
myhome/in/lamp/del → "TOGGLE 2000"    (переключить через 2 сек)
myhome/in/lamp/del → "OFF 10000"      (выключить через 10 сек)
```

### Команды, включаемые на время

**Синтаксис**: `КОМАНДА ВРЕМЯ_МС` (в топик с суффиксом `/cmd`)

Команда выполняется, а через указанное время выполняется **обратная команда**.

**Пример**:
```
myhome/in/light/cmd → "ON 3000"
  # Включит свет на 3 секунды, затем выключит
```

**Взаимно-обратные команды**:

- `ON` ↔ `OFF`
- `TOGGLE` ↔ `TOGGLE`
- `REST` ↔ `HALT`
- `XON` ↔ `XOFF`
- `ENABLE` ↔ `DISABLE`
- `FREEZE` ↔ `UNFREEZE`
- `INCREASE` ↔ `DECREASE`

---

## Важные моменты

### Восстановление состояния при старте

1. Контроллер подписывается на **статусные топики** на **8 секунд** после старта (TIMEOUT\_RETAIN = 8000 мс)
2. Брокер отправляет **последний актуальный статус**
3. Контроллер восстанавливает все значения: яркость, цвет, температуру и т.д.

### Интерпретация команд

Контроллер **интерпретирует** полученные команды:

**Пример**:
- Получена команда: `ON` на RGB канал
- Контроллер восстанавливает последний цвет (HSV из памяти)
- Публикует в `s_out/<item>/set` строку `240,100,255` («h,s,v[,ct]») и в `s_out/<item>/cmd` → `ON`

### Различие нового подхода (/set и /cmd) и старого (общий топик, совместимо с шиной OpenHab)
| Что | новый | openHab |
|-----|:----:|:----:|
| Команда управления | в /cmd| в общем топике |
| Команда установки значения | в /set| в общем топике |
| Диапазон значений | 0-255 | 0-100 |


**Практика**:
- `/set` используется для установки **значения** (например, яркость 0-255)
- `/cmd` используется для **команд** (ON, OFF, TOGGLE ...)

---

## HTTP API

### Базовый URL

```
http://<controller_ip>/item/<item_name>[/subitem][/suffix]
```

### Методы

| Метод | Действие |
|-------|----------|
| `GET` | Прочитать статус item |
| `POST` | Выполнить команду или установить значение |

### Примеры HTTP запросов

```bash
# Включить лампу
curl  --user arduino:password -X POST http://192.168.1.10/item/lamp1/cmd -d "ON" 

# Установить яркость
curl  --user arduino:password -X POST http://192.168.1.10/item/lamp1/set -d "150"

# Установить цвет (HSV; /rgb не поддерживается)
curl  --user arduino:password -X POST http://192.168.1.10/item/rgb_light/HSV -d "0,100,255"

# Получить текущий статус
curl  --user arduino:password -X GET http://192.168.1.10/item/lamp1

# Многозональная вентиляция - установить T в спальне
curl  --user arduino:password -X POST http://192.168.1.10/item/multivent/bedroom/set -d "22"

# Кондиционер - установить режим
curl  --user arduino:password -X POST http://192.168.1.10/item/ac_main/mode -d "HEAT"
```

### Дополнительные endpoints

| Endpoint | Метод | Описание |
|----------|-------|---------|
| `/config.json` | GET, POST | Загрузить/сохранить конфигурацию |
| `/config.bin` | GET, POST | Системная конфигурация |
| `/sketch` | POST | Загрузить прошивку (OTA) |
| `/command/<cmd>` | POST | Выполнить CLI команду |
| `/ram/[item]` | GET | Дамп item-ов из RAM (JSON; без имени — все) |
| `/` | GET | Переадресация на PWA приложение |

---

## MQTT подписки контроллера

Контроллер автоматически подписывается на:

1. **Командные широковещательные**: `root/bcst/#`
2. **Командные индивидуальные**: `root/id-устройства/#`
3. **Статусные** (только при старте, на 8 сек, TIMEOUT\_RETAIN = 8000 мс): `root/out/+/+/#`
4. **Служебные**: `$command` ловится общей подпиской `root/id-устройства/#` (отдельной подписки нет)

---

## Пример полной интеграции

### Конфигурация

```json
{
  "mqtt": ["lighthub01", "192.168.1.100", 1883],
  "topics": {"root": "myhome", "bcst": "in", "out": "s_out"},
  
  "items": {
    "lamp_bedroom": [0, 1],
    "rgb_light": [1, 10],
    "ac_main": [10, [1, {
      "mode": {"emit": "ac/mode"},
      "temp": {"emit": "ac/temp"}
    }]]
  }
}
```

### MQTT команды и ответы

```
# Включить лампу
→ myhome/in/lamp_bedroom/cmd: ON
← myhome/s_out/lamp_bedroom/set: 255 (восстановленная яркость, 0-255)
← myhome/s_out/lamp_bedroom/cmd: ON

# Установить яркость RGB
→ myhome/in/rgb_light/set: 200
← myhome/s_out/rgb_light/set: 0,0,200,500 (в статусный топик запишется полный набор параметров канала)

# Установить цвет
→ myhome/in/rgb_light/hue: 240
← myhome/s_out/rgb_light/set:  240,100,200,500 (в статусный топик запишется полный набор параметров канала)

# Установить насыщенность 
→ myhome/in/rgb_light/sat: 50
← myhome/s_out/rgb_light/set:  240,50,200,500 (в статусный топик запишется полный набор параметров канала)

# Включить кондиционер в режим HEAT
→ myhome/in/ac_main/cmd: HEAT
← myhome/s_out/ac_main/cmd: HEAT (запишется в статусный топик)

# Установить температуру
→ myhome/in/ac_main/set: 22
← myhome/s_out/ac_main/set: 22 (запишется в статусный топик)
```

---

## Диагностика MQTT

### Проверка связи

```bash
# Подписаться на все топики контроллера
mosquitto_sub -h 192.168.1.100 -t "myhome/#" -v

# Отправить команду
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "ON"
```

### Проверка статусных топиков

```bash
# Только статусные топики
mosquitto_sub -h 192.168.1.100 -t "myhome/s_out/#" -v
```

---

**Версия документа**: 2.1 (актуально для работа\_с\_mqtt из wiki.lazyhome.ru)  
**Источники**: [MQTT wiki](https://www.lazyhome.ru/dokuwiki/doku.php?id=%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D0%B0_%D1%81_mqtt), [API wiki](https://www.lazyhome.ru/dokuwiki/doku.php?id=api)
