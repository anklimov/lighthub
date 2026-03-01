# Миграция с suffixes_reference.md на suffixes_reference_v2.md

> Руководство для пользователей старой документации по переходу на новую правильную версию

---

## 📋 Основные различия

### Старая версия (suffixes_reference.md) — ❌ Неверно

**Проблемы:**
1. ❌ Суффиксы описаны как универсальные для всех типов каналов
2. ❌ Нет разделения на device-specific суффиксы
3. ❌ Пропущены многие суффиксы (особенно для AC, Multivent, PID)
4. ❌ Неправильные диапазоны значений
5. ❌ Структура MQTT топиков упрощена

**Пример старой ошибки:**
```markdown
# suffixes_reference.md
- /cmd — команда
- /set — значение
- /val — статус
- /hue — цвет
- /sat — насыщенность

(Других суффиксов нет — ошибка!)
```

### Новая версия (suffixes_reference_v2.md) — ✅ Правильно

**Улучшения:**
1. ✅ Правильная категоризация (7 категорий суффиксов)
2. ✅ Таблица применимости для каждого типа канала
3. ✅ Device-specific суффиксы (AC, Multivent, PID)
4. ✅ Правильные диапазоны значений
5. ✅ Полная структура MQTT топиков

**Пример новой структуры:**
```markdown
# suffixes_reference_v2.md

## Основные суффиксы (для всех)
- /cmd, /set, /val, /del

## Цветовые суффиксы (RGB)
- /hue, /sat, /hsv, /rgb

## Суффиксы AC
- /mode, /fan, /lock, /swing, /quiet

## Суффиксы Multivent
- /mode, /fan

## Суффиксы PID
- /ctrl, /mode

(Все суффиксы согласно wiki.lazyhome.ru)
```

---

## 🔄 Таблица соответствия

### Как переписать старые конфигурации

| Старое | Новое | Примечание |
|--------|-------|-----------|
| `/cmd` | `/cmd` | Не изменилось |
| `/set` | `/set` | Не изменилось, но теперь 0-255 |
| `/val` | `/val` | Не изменилось, только для выхода |
| `/hue` | `/hue` | 0-365° (было неупомянуто) |
| `/sat` | `/sat` | 0-100% (было неупомянуто) |
| Отсутствовало | `/fan` | Новое для AC и Multivent |
| Отсутствовало | `/mode` | Новое для AC, Multivent, PID |
| Отсутствовало | `/lock` | Новое для AC |
| Отсутствовало | `/swing` | Новое для AC |
| Отсутствовало | `/quiet` | Новое для AC |
| Отсутствовало | `/ctrl` | Новое для управления состоянием |
| Отсутствовало | `/del` | Новое для команд с задержкой |

---

## 🎯 Примеры обновления конфигураций

### Сценарий 1: RGB свет

#### Старое (неверное)
```json
{
  "items": {
    "rgb_light": [10, 1]
  }
}

Топики:
- myhome/in/rgb_light/cmd → ON, OFF
- myhome/in/rgb_light/set → 0-100
- myhome/in/rgb_light/hue → ???
- myhome/in/rgb_light/sat → ???
```

**Проблемы:**
- ❌ `/hue` и `/sat` не описаны
- ❌ Не ясны диапазоны значений
- ❌ Нет информации о взаимодействии суффиксов

#### Новое (правильное)
```json
{
  "items": {
    "rgb_light": [10, 1]
  }
}

Топики согласно suffixes_reference_v2.md:
- myhome/in/rgb_light/cmd → ON, OFF, TOGGLE (команды)
- myhome/in/rgb_light/set → 0-255 (яркость)
- myhome/in/rgb_light/hue → 0-365 (оттенок в градусах)
- myhome/in/rgb_light/sat → 0-100 (насыщенность в %)
- myhome/in/rgb_light/hsv → hue,sat,val (полный HSV)
- myhome/in/rgb_light/rgb → r,g,b или r,g,b,w (RGB формат)

Синергия:
- ON без /hue, /sat → восстановить последний цвет
- /hue = 0 → красный
- /sat = 0 → белый (без цвета)
- /sat = 100 → полный цвет
```

### Сценарий 2: Кондиционер

#### Старое (неверное)
```json
{
  "items": {
    "ac_main": [13, {...}]
  }
}

Топики (из старого suffixes_reference.md):
- myhome/in/ac_main/cmd → ON, OFF
- myhome/in/ac_main/set → 0-100 (???)
(Никаких других суффиксов не описано!)
```

**Проблемы:**
- ❌ Отсутствуют суффиксы: /mode, /fan, /lock, /swing, /quiet
- ❌ Невозможно управлять режимом работы
- ❌ Невозможно менять скорость вентилятора

#### Новое (правильное)
```json
{
  "items": {
    "ac_main": [13, {
      "mode": {"emit": "ac/mode"},
      "temp": {"emit": "ac/temp"}
    }]
  }
}

Топики согласно suffixes_reference_v2.md:
- myhome/in/ac_main/cmd → ON, OFF, TOGGLE
- myhome/in/ac_main/mode → HEAT, COOL, AUTO, DRY, FAN_ONLY
- myhome/in/ac_main/set → 16-30 (температура °C)
- myhome/in/ac_main/fan → HIGH, MED, LOW, AUTO
- myhome/in/ac_main/lock → ON, OFF (блокировка ПДУ)
- myhome/in/ac_main/swing → ON, OFF (направление воздуха)
- myhome/in/ac_main/quiet → ON, OFF (тихий режим)

Синергия:
- ON → восстановить последний режим и температуру
- OFF → запомнить текущие параметры
- /mode → только для выбора режима
- /set → установить температуру
- /fan → скорость вентилятора
```

### Сценарий 3: Многозональная вентиляция

#### Старое (неверное)
```json
{
  "items": {
    "multivent": [17, {...}]
  }
}

Топики:
- myhome/in/multivent/set → 20-25
(Больше ничего не понятно)
```

**Проблемы:**
- ❌ Структура subitem-ов не описана
- ❌ Отсутствуют суффиксы /fan, /mode
- ❌ Непонятно, как управлять отдельными зонами

#### Новое (правильное)
```json
{
  "items": {
    "multivent": [17, {
      "bedroom": {"set": "multivent/bedroom/set"},
      "kitchen": {"set": "multivent/kitchen/set"}
    }]
  }
}

Топики согласно suffixes_reference_v2.md:
- myhome/in/multivent/cmd → ON, OFF (главное управление)
- myhome/in/multivent/mode → HEAT, AUTO, COOL (режим)
- myhome/in/multivent/fan → HIGH, MED, LOW (скорость вентилятора)

Для отдельных зон:
- myhome/in/multivent/bedroom/set → 21 (установить T в спальне)
- myhome/in/multivent/kitchen/set → 22 (установить T на кухне)

Статус:
- myhome/s_out/multivent/bedroom/val → 21.5 (текущая T)
- myhome/s_out/multivent/kitchen/val → 22.1 (текущая T)
```

### Сценарий 4: Теплые полы (PID)

#### Старое (неверное)
```json
{
  "items": {
    "floor": [15, 4]
  }
}

Топики:
- myhome/in/floor/cmd → ON, OFF
- myhome/in/floor/set → 20-30
(Никакого управления состоянием)
```

**Проблемы:**
- ❌ Отсутствует управление состоянием (FREEZE, ENABLE, DISABLE)
- ❌ Нет информации о режимах (AUTO, HEAT)
- ❌ Невозможно заблокировать канал

#### Новое (правильное)
```json
{
  "items": {
    "floor": [15, 4]
  }
}

Топики согласно suffixes_reference_v2.md:
- myhome/in/floor/cmd → ON, OFF, TOGGLE (включить/выключить)
- myhome/in/floor/set → 24 (установить температуру)
- myhome/in/floor/mode → AUTO, HEAT, OFF (режим работы)
- myhome/in/floor/ctrl → ENABLE, DISABLE, FREEZE, UNFREEZE (управление)

Синергия:
- ON + mode AUTO → включить, включить регулирование если T < установленной
- ON + mode HEAT → включить отопление
- OFF → выключить (но помнить последние параметры)
- /ctrl FREEZE → заблокировать (игнорировать команды)
- /ctrl ENABLE → разрешить управление
```

---

## 📊 Полная таблица нового документа

### Основные суффиксы (универсальные)

| Суффикс | Применимость | Диапазон | Пример |
|---------|:----:|:-----:|---------|
| `/cmd` | Все | Текст | ON, OFF, TOGGLE |
| `/set` | Все | 0-255 | 150 |
| `/val` | Все | 0-255 | 100 (только выход) |
| `/del` | Все | Текст + время | "ON 5000" |

### Цветовые суффиксы (RGB, RGBW, RGBWW)

| Суффикс | Применимость | Диапазон | Пример |
|---------|:----:|:-----:|---------|
| `/hue` | RGB только | 0-365° | 240 (синий) |
| `/sat` | RGB только | 0-100% | 100 (полный) |
| `/hsv` | RGB только | H,S,V | 240,100,200 |
| `/rgb` | RGB только | R,G,B или R,G,B,W | 255,0,0 или 255,0,0,100 |

### AC суффиксы

| Суффикс | Применимость | Значения | Пример |
|---------|:----:|:-----:|---------|
| `/cmd` | AC | ON, OFF, TOGGLE | ON |
| `/mode` | AC | HEAT, COOL, AUTO, DRY, FAN_ONLY | HEAT |
| `/set` | AC | 16-30°C | 22 |
| `/fan` | AC | HIGH, MED, LOW, AUTO | HIGH |
| `/lock` | AC | ON, OFF | ON |
| `/swing` | AC | ON, OFF | ON |
| `/quiet` | AC | ON, OFF | ON |

### Multivent суффиксы

| Суффикс | Применимость | Значения | Пример |
|---------|:----:|:-----:|---------|
| `/cmd` | Multivent | ON, OFF | ON |
| `/set` | Multivent | 0-100 или температура | 21 |
| `/mode` | Multivent | HEAT, AUTO, COOL | AUTO |
| `/fan` | Multivent | HIGH, MED, LOW | HIGH |

### PID суффиксы

| Суффикс | Применимость | Значения | Пример |
|---------|:----:|:-----:|---------|
| `/cmd` | PID | ON, OFF, TOGGLE | ON |
| `/set` | PID | Температура | 24 |
| `/mode` | PID | HEAT, AUTO, OFF | AUTO |
| `/ctrl` | PID | ENABLE, DISABLE, FREEZE, UNFREEZE | FREEZE |

---

## 🚀 Как перейти

### Шаг 1: Поняться, что изменилось
Прочитайте [CHANGELOG_v2.md](CHANGELOG_v2.md)

### Шаг 2: Изучить новую структуру
Изучите [suffixes_reference_v2.md](suffixes_reference_v2.md)

### Шаг 3: Обновить конфигурацию
Найдите ваш тип канала в таблице и добавьте новые суффиксы

### Шаг 4: Обновить MQTT подписки
Добавьте новые топики в вашу систему (Home Assistant, Node-Red, etc.)

### Шаг 5: Тестирование
Проверьте все топики согласно [mqtt_quick_reference.md](mqtt_quick_reference.md)

---

## ❓ FAQ

### Q: Нужно ли мне обновлять существующие конфигурации?
**A:** Да, если вы используете:
- AC (кондиционер) — добавьте /mode, /fan, /lock, /swing, /quiet
- RGB свет — убедитесь, что используете /hue, /sat, /hsv, /rgb правильно
- Multivent — добавьте /fan, /mode
- PID (теплые полы) — добавьте /ctrl для управления состоянием

### Q: Будет ли работать старая конфигурация?
**A:** Да, но вы потеряете функциональность. Рекомендуется обновить.

### Q: Что если я использую только включение/выключение?
**A:** Для простых ON/OFF конфигурация не изменится.但 рекомендуется обновить для совместимости.

### Q: Где найти примеры обновления?
**A:** В [configuration_examples.md](configuration_examples.md) есть готовые примеры для всех типов.

---

## 📞 Помощь

- Вопросы о суффиксах: [suffixes_reference_v2.md](suffixes_reference_v2.md)
- Структура MQTT: [mqtt_api_reference.md](mqtt_api_reference.md)
- Быстрая справка: [mqtt_quick_reference.md](mqtt_quick_reference.md)
- Примеры: [configuration_examples.md](configuration_examples.md)

---

**Версия**: 2.0  
**Дата**: 2025-01-24  
**Статус**: ✅ Актуально согласно wiki.lazyhome.ru
