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
| **Статус** | выход | `myhome/s_out/lamp1/set` (0-255) + общий `myhome/s_out/lamp1` (0-100) |

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
# Установить яркость 50 из 255
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

# Установить полный HSV одной командой (hue,sat,value), суффикс ЗАГЛАВНЫЙ
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/HSV" -m "240,100,200"
```

### Цвет по RGB (не поддерживается)

> Суффикс `/rgb` **не поддерживается** в текущей сборке (S_RGB не обрабатывается).
> Цвет задаётся через `/HSV` или строкой «h,s,v[,ct]» в общем топике без суффикса.

```bash
# Красный (HSV: hue=0, sat=100, val=255)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/HSV" -m "0,100,255"

# Зеленый (hue=120)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb/HSV" -m "120,100,255"

# Общий топик: строка «h,s,v[,ct]» трактуется как HSV(+colorTemp), не RGBW
mosquitto_pub -h 192.168.1.100 -t "myhome/in/rgb" -m "240,100,200"
```

---

## Управление кондиционером

```bash
# Включить кондиционер
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/cmd" -m "ON"

# Выключить
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/cmd" -m "OFF"

# Режим нагрева (текстовые режимы HEAT, COOL, AUTO, DRY, FAN_ONLY — в /cmd; /mode — число 0-4)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/cmd" -m "HEAT"

# Температура
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/set" -m "22"

# Скорость вентилятора (LOW, MEDIUM, HIGH, OFF или число)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/fan" -m "HIGH"

# Направление воздуха (ON, OFF)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/swing" -m "ON"

# Тихий режим (входной subitem «queit» — опечатка в прошивке; статус — /quiet)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/ac_main/queit" -m "ON"

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

# Режим AUTO (включить, если T < установленной) — режим: команда в /cmd
mosquitto_pub -h 192.168.1.100 -t "myhome/in/floor/cmd" -m "AUTO"

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

# Режим зоны (HEAT, AUTO, COOL) — команда в zone/cmd (zone/mode не обрабатывается)
mosquitto_pub -h 192.168.1.100 -t "myhome/in/multivent/bedroom/cmd" -m "AUTO"
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
# Включить лампу: /item/<item>[/subitem][/suffix] (GET/POST)
curl -X POST http://192.168.1.10/item/lamp1/cmd -d "ON"

# Установить яркость
curl -X POST http://192.168.1.10/item/lamp1/set -d "150"

# Прочитать статус item (GET)
curl -X GET http://192.168.1.10/item/lamp1

# Передать вход сенсора (текущую температуру)
curl -X POST http://192.168.1.10/item/thermostat/val -d "21.5"

# AC: установить режим (командой) и температуру
curl -X POST http://192.168.1.10/item/ac_main/cmd -d "HEAT"
curl -X POST http://192.168.1.10/item/ac_main/set -d "22"

# CLI-команда: /command/<cmd>
curl -X POST http://192.168.1.10/command/reboot

# Конфигурация (GET/POST) и дамп RAM
curl http://192.168.1.10/config.json
curl http://192.168.1.10/ram/lamp1
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
| `/val` | Вход сенсора (THERMO/PID/VENT/SPRINKLER) | Число (T/значение) |
| `/hue` | Оттенок RGB | 0-365° |
| `/sat` | Насыщенность RGB | 0-100% |
| `/HSV` | Полный HSV (ЗАГЛАВНЫЙ) | hue,sat,value |
| `/rgb` | **Не поддерживается** (S_RGB не обрабатывается) | — |
| `/fan` | Вентилятор | LOW, MEDIUM, HIGH, OFF или число|
| `/mode` | Режим работы — число 0-4 (AC); текстовые режимы — через /cmd | 0-4 |
| `/ctrl` | Управление | ENABLE, DISABLE, FREEZE, UNFREEZE |
| `/del` | Задержка | "ON 5000" |

---

**Дополнительно**:
- Полный справочник: [mqtt_api_reference.md](mqtt_api_reference.md)
- Справочник суффиксов: [suffixes_reference.md](suffixes_reference.md)
- Примеры конфигураций: [configuration_examples.md](configuration_examples.md)
