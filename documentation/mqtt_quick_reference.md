# Шпаргалка MQTT LightHub

> Быстрая справка часто используемых MQTT команд для LightHub

---

## Структура топика

```
root / [id или bcst или out] / item_name / [subitem/] suffix
└──────┴─────────────────────────┴──────────────────────────┘
  мyhome       /в/       /lamp1/   /cmd
```

**Пример для дома `myhome`, устройства `in`, лампы `lamp1`, команды включения**:
```
myhome/in/lamp1/cmd
```

---

## Три типа топиков

| Тип | Направление | Пример |
|-----|:-----------:|--------|
| **Команда (broadcast)** | вход | `myhome/in/lamp1/cmd` (выполнить на всех) |
| **Команда (индивидуальная)** | вход | `myhome/lighthub01/lamp1/cmd` (на конкретном) |
| **Статус** | выход | `myhome/s_out/lamp1/val` (ответ контроллера) |

---

## Базовые команды управления

### Включение/выключение

```bash
# Включить лампу (ON)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "ON"

# Выключить лампу (OFF)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "OFF"

# Переключить (TOGGLE)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "TOGGLE"
```

### Установка яркости

```bash
# Установить яркость 50% (0-100)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/set" -m "50"

# Установить яркость 150 (0-255)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/set" -m "150"

# Увеличить на 10 пунктов
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "INCREASE 10"

# Уменьшить на 10 пунктов
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "DECREASE 10"
```

---

## Управление RGB светом

### Цвет по HSV (Hue, Saturation, Value)

```bash
# Оттенок (0-365°):
#   0° = красный      60° = желтый    120° = зеленый
#   180° = голубой    240° = синий    300° = магента

# Установить красный цвет
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/hue" -m "0"

# Установить зеленый
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/hue" -m "120"

# Установить синий
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/hue" -m "240"

# Установить насыщенность 0% = белый, 100% = полный цвет
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/sat" -m "100"

# Установить яркость 200 (0-255)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/set" -m "200"

# Установить полный HSV одной командой (hue,sat,value)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/hsv" -m "240,100,200"
```

### Цвет по RGB

```bash
# Красный RGB(255, 0, 0)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/rgb" -m "255,0,0"

# Зеленый
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/rgb" -m "0,255,0"

# RGBW с белым
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/rgb" -m "255,0,0,100"
```

---

## Управление кондиционером

```bash
# Включить кондиционер
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/cmd" -m "ON"

# Выключить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/cmd" -m "OFF"

# Режим нагрева (HEAT, COOL, AUTO, DRY, FAN_ONLY)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/mode" -m "HEAT"

# Температура
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/set" -m "22"

# Скорость вентилятора (HIGH, MED, LOW, AUTO)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/fan" -m "HIGH"

# Направление воздуха (ON, OFF)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/swing" -m "ON"

# Тихий режим
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/quiet" -m "ON"

# Блокировка ПДУ
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/lock" -m "ON"
```

---

## Управление теплыми полами (PID)

```bash
# Включить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/cmd" -m "ON"

# Выключить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/cmd" -m "OFF"

# Установить температуру 24°C
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/set" -m "24"

# Режим AUTO (включить, если T < установленной)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/mode" -m "AUTO"

# Заморозить (игнорировать команды)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/ctrl" -m "FREEZE"

# Разморозить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/ctrl" -m "UNFREEZE"
```

---

## Многозональная вентиляция

```bash
# Установить температуру в спальне
mosquitto_pub -h 192.168.1.100 -t "myhome/in/multivent/bedroom/set" -m "21"

# Установить температуру на кухне
mosquitto_pub -h 192.168.1.100 -t "myhome/in/multivent/kitchen/set" -m "22"

# Скорость вентилятора
mosquitto_pub -h 192.168.1.100 -t "myhome/in/multivent/fan" -m "HIGH"

# Режим (HEAT, AUTO, COOL)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/multivent/mode" -m "AUTO"
```

---

## Команды с задержкой

```bash
# Включить на 3 секунды, потом выключить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "ON 3000"

# Включить через 5 сек 
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/del" -m "ON 5000"

# Импульс гаража на 3 сек
mosquitto_pub -h 192.168.1.100 -t "myhome/in/gate/cmd" -m "ON 3000"
```

---

## Расширенные команды

```bash
# Перевести в режим HALT (запомнить состояние и выключить)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "HALT"

# Восстановить с HALT
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "REST"

# Включить (если не DISABLE)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "XON"

# Выключить (если был включен XON)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/cmd" -m "XOFF"

# Разрешить управление (если было DISABLE)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/ctrl" -m "ENABLE"

# Запретить управление
mosquitto_pub -h 192.168.1.100 -t "myhome/in/lamp1/ctrl" -m "DISABLE"
```

---

## Подписка на статусные топики

```bash
# Подписаться на все топики контроллера
mosquitto_sub -h 192.168.1.100 -t "myhome/#" -v

# Только статусные топики (ответы)
mosquitto_sub -h 192.168.1.100 -t "myhome/s_out/#" -v

# Статус конкретной лампы
mosquitto_sub -h 192.168.1.100 -t "myhome/s_out/lamp1/#" -v

# Служебная статистика
mosquitto_sub -h 192.168.1.100 -t "myhome/lighthub01/\$stats" -v
```

---

## HTTP API (альтернатива MQTT)

```bash
# Включить лампу через HTTP
curl -X POST http://192.168.1.10/lamp1/cmd -d "ON"

# Установить яркость
curl -X POST http://192.168.1.10/lamp1/set -d "150"

# Установить RGB цвет
curl -X POST http://192.168.1.10/rgb/set -d "255,0,0"

# Передать текущую температуру
curl -X GET http://192.168.1.10/thermostat/val -d "150"

# AC: установить режим и температуру
curl -X POST http://192.168.1.10/ac_main/mode -d "HEAT"
curl -X POST http://192.168.1.10/ac_main/set -d "22"
```

---

## Типичные ошибки

| Ошибка | Причина | Решение |
|--------|---------|--------|
| Команда не выполняется | Неправильный топик | Проверьте структуру: `root/bcst/item/suffix` |
| Статус не приходит | Неправильный `out` параметр | Использовать `myhome/s_out/...` |
| Яркость не меняется | Используется `/cmd` вместо `/set` | Используйте `/set` для значений (0-255) |
| RGB не меняет цвет | Насыщенность 0% (белый) | Установите `/sat` на 100 для цвета |
| Нет ответа от MQTT | Контроллер не подключен | Проверьте MQTT конфигурацию в `config.json` |

---

## Таблица суффиксов (краткая)

| Суффикс | Использование | Примеры |
|---------|--------------|---------|
| `/cmd` | Команды | ON, OFF, TOGGLE, HALT, REST |
| `/set` | Значения | 0-255, температура |
| `/val` |  | Текущее значение |
| `/hue` | Оттенок RGB | 0-365° |
| `/sat` | Насыщенность RGB | 0-100% |
| `/hsv` | Полный HSV | hue,sat,value |
| `/rgb` | RGB цвет | r,g,b или r,g,b,w |
| `/fan` | Вентилятор | HIGH, MED, LOW, AUTO, OFF или число|
| `/mode` | Режим работы | HEAT, COOL, AUTO, DRY |
| `/ctrl` | Управление | ENABLE, DISABLE, FREEZE, UNFREEZE |
| `/del` | Задержка | "ON 5000" |

---

**Дополнительно**:
- Полный справочник: [mqtt_api_reference.md](mqtt_api_reference.md)
- Справочник суффиксов: [suffixes_reference_v2.md](suffixes_reference_v2.md)
- Примеры конфигураций: [configuration_examples.md](configuration_examples.md)
