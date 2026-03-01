# LightHub: Инженерная документация (Индекс)

> **Полная инженерная документация системы LightHub**  
> Версия ядра: CH_DIMMER (0) - CH_MERCURY (22)  
> Дата актуализации: 2025-01-24

---

## 📚 Структура документации

### Основные документы

1. **[light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)** ⭐ **НАЧНИТЕ ОТСЮДА**
   - Полное описание структуры JSON конфигурации
   - Все секции: mqtt, topics, modbus, items, in
   - Инженерные принципы конфигурирования
   - Полный пример реальной конфигурации

### Справочники

**🆕 MQTT API и топики:**

2. **[mqtt_api_reference.md](mqtt_api_reference.md)** — ⭐ ПОЛНЫЙ справочник MQTT API
   - Структура MQTT топиков: `root/[id или bcst или out]/item/[subitem]/suffix`
   - Три типа топиков: широковещательные команды, индивидуальные команды, статусные
   - Таблица суффиксов с применимостью
   - HTTP API endpoints (`/item/<name>`, `/config.json`, `/command`)
   - Примеры MQTT команд и HTTP curl запросов
   - Восстановление состояния при старте контроллера
   - Диагностика MQTT подключения

3. **[suffixes_reference_v2.md](suffixes_reference_v2.md)** — ⭐ ИСПРАВЛЕННЫЙ справочник суффиксов
   - Правильная структура суффиксов согласно wiki.lazyhome.ru
   - 7 категорий: основные, цветовые, AC, Multivent, PID, ШИМ, управление состоянием
   - Таблица применимости по типам каналов (CH_DIMMER, CH_RGB, CH_AC и др.)
   - Диапазоны значений: 0-100 vs 0-255, /hue 0-365°, /sat 0-100%
   - Примеры сценариев для каждого типа канала

**Основные справочники:**

4. **[channel_types_reference.md](channel_types_reference.md)** — Справочник типов каналов (0-22)
   - Таблица всех типов каналов с кодами
   - Текстовые обозначения и английские названия
   - Синтаксис конфигурации для каждого типа
   - Визуализация иерархии типов

5. **[suffixes_reference.md](suffixes_reference.md)** — Справочник суффиксов параметров (архив)
   - Старая версия справочника (см. suffixes_reference_v2.md)

6. **[technical_channel_types_table.md](technical_channel_types_table.md)** — Подробные технические таблицы
   - Таблицы параметров для каждого типа
   - Значения по умолчанию и ограничения
   - Специфика работы каждого типа

### Модули и компоненты

7. **[modules_description.md](modules_description.md)** — Описание модулей
   - out_Multivent — многозональная вентиляция
   - out_AC — управление кондиционером
   - out_PID — PID регулятор
   - out_Motor — управление двигателем
   - И другие модули...

8. **[multivent_module_description.md](multivent_module_description.md)** — Многозональная вентиляция (подробно)

9. **[modules_real_config.md](modules_real_config.md)** — Реальные конфигурации модулей

---

## 🎯 Быстрый старт по задачам

### Задача: Управлять LED светом через MQTT

1. Откройте [mqtt_api_reference.md](mqtt_api_reference.md) — чтобы понять структуру топиков
2. Выберите тип канала: CH_RGB (10), CH_RGBW (11) или CH_RGBWW (12)
3. Найдите примеры в [configuration_examples.md](configuration_examples.md)
4. Используйте MQTT команды из [suffixes_reference_v2.md](suffixes_reference_v2.md):
   ```
   myhome/in/rgb_lamp/hue → 240      (установить синий)
   myhome/in/rgb_lamp/sat → 100      (полная насыщенность)
   myhome/in/rgb_lamp/set → 200      (яркость 200)
   ```
5. Получите ответы в статусных топиках:
   ```
   myhome/s_out/rgb_lamp/hue → 240
   myhome/s_out/rgb_lamp/sat → 100
   myhome/s_out/rgb_lamp/val → 200
   ```

### Задача: Управлять кондиционером через MQTT

1. Откройте [mqtt_api_reference.md](mqtt_api_reference.md) — раздел "Суффиксы кондиционера (AC)"
2. Найдите CH_AC (13) в [configuration_examples.md](configuration_examples.md)
3. Используйте MQTT команды из [suffixes_reference_v2.md](suffixes_reference_v2.md):
   ```
   myhome/in/ac_main/cmd → ON        (включить)
   myhome/in/ac_main/mode → HEAT     (режим нагрева)
   myhome/in/ac_main/set → 22        (температура 22°C)
   myhome/in/ac_main/fan → HIGH      (вентилятор на максимум)
   ```
4. Получите ответы в статусных топиках:
   ```
   myhome/s_out/ac_main/cmd → ON
   myhome/s_out/ac_main/mode → HEAT
   myhome/s_out/ac_main/set → 22
   ```

### Задача: Настроить многозональную вентиляцию

1. Прочитайте [multivent_module_description.md](multivent_module_description.md)
2. Найдите CH_MULTIVENT (18) в [configuration_examples.md](configuration_examples.md)
3. Добавьте зоны в массив `"зоны": {...}`
4. Настройте PID параметры `[Kp, Ki, Kd, dT]`
5. Привяжите MQTT топики через `"emit"`

### Задача: Создать систему с входами (кнопки, датчики)

1. Прочитайте раздел "Секция `in`" в [light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)
2. Определите GPIO пины входов (37, 38, 39, и т.д.)
3. Привяжите их к объектам через `"item": "имя_канала"`
4. Используйте команды `scmd`, `rcmd` для действий
5. Используйте `"emit"` для публикации событий в MQTT

---

## 📖 Таблица типов каналов

| Код | Тип | Применение | Ссылка на пример |
|-----|-----|-----------|------------------|
| 0 | DMX | Диммер через DMX 512 | [⬇️](configuration_examples.md#ch_dimmer-0---dmx-диммер) |
| 1 | DMXRGBW | RGB+White через DMX | [⬇️](configuration_examples.md#ch_rgbw-1---dmx-rgbwhite) |
| 2 | DMXRGB | RGB через DMX | [⬇️](configuration_examples.md#ch_rgb-2---dmx-rgb) |
| 3 | PWM | GPIO PWM | [⬇️](configuration_examples.md#ch_pwm-3---gpio-pwm) |
| 4 | MBUSDIM | Modbus AC Dimmer (Legacy) | [⬇️](configuration_examples.md#ch_modbus-4---modbus-ac-dimmer-legacy) |
| 5 | THERMO | ON/OFF Термостат | [⬇️](configuration_examples.md#ch_thermo-5---onoff-термостат) |
| 6 | RELAY | GPIO Реле | [⬇️](configuration_examples.md#ch_relay-6---gpio-реле) |
| 7 | GROUP | Группа каналов | [⬇️](configuration_examples.md#ch_group-7---группа-каналов) |
| 8 | VCTEMP | Vacom PID Терморегулятор | [⬇️](configuration_examples.md#ch_vctemp-8---vacom-pid-терморегулятор) |
| 9 | MBUSVC | Vacom Мотор | [⬇️](configuration_examples.md#ch_vc-9---vacom-мотор-регулятор) |
| 10 | ACHAIER | Кондиционер Haier | [⬇️](configuration_examples.md#ch_ac-10---кондиционер-haier) |
| 11 | SPILED | SPI LED Лента | [⬇️](configuration_examples.md#ch_spiled-11---spi-led-лента) |
| 12 | MOTOR | Шаговый двигатель | [⬇️](configuration_examples.md#ch_motor-12---шаговый-двигатель) |
| 13 | PID | PID Регулятор | [⬇️](configuration_examples.md#ch_pid-13---pid-регулятор) |
| 14 | MBUS | Universal Modbus | [⬇️](configuration_examples.md#ch_mbus-14---universal-modbus) |
| 15 | UARTBRDG | UART Мост | [⬇️](configuration_examples.md#ch_uartbridge-15---uart-мост) |
| 16 | RELAYX | Медленный PWM реле | [⬇️](configuration_examples.md#ch_relayx-16---медленный-pwm-через-реле) |
| 17 | DMXRGBWW | RGBWW через DMX | [⬇️](configuration_examples.md#ch_rgbww-17---dmx-rgbww) |
| 18 | VENTS | Многозональная вентиляция | [⬇️](configuration_examples.md#ch_multivent-18---многозональная-вентиляция) |
| 19 | ELEVATOR | Лифт | - (TBD) |
| 20 | COUNTER | Счётчик импульсов | [⬇️](configuration_examples.md#ch_counter-20---счётчик-импульсов) |
| 21 | HUM | Увлажнитель | [⬇️](configuration_examples.md#ch_humidifier-21---управление-увлажнителем) |
| 22 | MERCURY | Счётчик энергии | [⬇️](configuration_examples.md#ch_mercury-22---счётчик-энергии-mercury) |

---

## 📊 Таблица суффиксов MQTT

| Суффикс | Назначение | Применимо к |
|---------|-----------|-----------|
| `/cmd` | Команда управления | Все типы |
| `/val` | Текущее значение (статус) | Все типы |
| `/set` | Установка значения | Диммеры, регуляторы |
| `/hue` | Оттенок (0-359°) | RGB/RGBW/RGBWW |
| `/sat` | Насыщенность (0-100%) | RGB/RGBW/RGBWW |
| `/temp` | Температура цвета (K) | RGB/RGBW/RGBWW |
| `/fan` | Скорость вентилятора | AC, Multivent, Vacom |
| `/mode` | Режим работы | AC, Multivent |
| `/raw` | JSON формат (отладка) | Все типы |

---

## 🔗 Инструменты и утилиты

### JSON Валидаторы

- [JSONLint](https://www.jsonlint.com/) — проверка синтаксиса JSON
- [JSON Online Editor](https://jsoncrack.com/) — визуализация структуры

### Инженерные калькуляторы

- Масштабирование значений: `value_out = (value_in - min_in) / (max_in - min_in) * (max_out - min_out) + min_out`
- Коэффициенты PID (для запуска): `Kp = 1.0, Ki = 0.05, Kd = 0.02, dT = 5.0`

---

## 🛠️ Отладка конфигурации

### Проверка синтаксиса

```bash
# Скопируйте конфигурацию в JSONLint или используйте Python:
python3 -m json.tool config.json
```

### Логирование Modbus

Если Modbus устройство не отвечает:
1. Проверьте baudrate (по умолчанию 9600)
2. Проверьте адрес устройства
3. Проверьте регистры (должны быть доступны для чтения)
4. Включите syslog для отладки

### MQTT Отладка

```bash
# Подпишитесь на все топики
mosquitto_sub -h 192.168.88.2 -t "myhome/#" -v

# Отправьте команду
mosquitto_pub -h 192.168.88.2 -t "myhome/dev/lamp/cmd" -m "ON"
```

---

## ⚡ Инженерные правила

### Правило 1: Сначала структура

```
Modbus шаблон (в "modbus") 
    ↓
Item (в "items")
    ↓
MQTT привязка (через "emit")
    ↓
Входы (в "in", опционально)
```

### Правило 2: Минимизируй poll

- DMX: нет опроса (output only)
- RS485 Modbus: не менее 100 мс задержки
- 1-Wire: 500-1000 мс
- GPIO входы: 10-50 мс

### Правило 3: Используй GROUP для синхронизации

```json
"lights_all": [7, ["lamp1", "lamp2", "lamp3"]],
// Теперь можно управлять всеми сразу:
// myhome/dev/lights_all/cmd → ON
```

### Правило 4: Подробное имя = легче найти

```json
"lamp_bedroom_ceiling": [0, 1],      // ✓ Хорошо
"lamp1": [0, 1]                      // ✗ Плохо
```

---

## 📞 Получить помощь

- **GitHub репозиторий**: https://github.com/anklimov/lighthub
- **Официальный сайт**: https://lazyhome.ru
- **Документация Wiki**: https://www.lazyhome.ru/dokuwiki/

---

## 📝 История версий

| Версия | Дата | Изменения |
|--------|------|----------|
| 2.0 | 2025-01-24 | Полная актуализация документации для ядра CH_DIMMER (0) - CH_MERCURY (22) |
| 1.0 | 2024-12-16 | Исходная версия |

---

## ✅ Чек-лист перед запуском

- [ ] JSON синтаксис проверен (JSONLint)
- [ ] Все GPIO пины уникальны (нет конфликтов)
- [ ] Все Modbus адреса доступны
- [ ] MQTT брокер доступен и запущен
- [ ] Все типы каналов в диапазоне 0-22
- [ ] Содержатся ли необходимые секции (mqtt, items)
- [ ] Проверены все MQTT топики
- [ ] Запасная копия конфигурации сохранена

---

## 📌 Важные замечания

⚠️ **БЕЗОПАСНОСТЬ**: 
- Не сохраняйте пароли MQTT в конфигурации!
- Используйте CLI для установки пароля

⚠️ **ПРОИЗВОДИТЕЛЬНОСТЬ**:
- Максимум 1000 items рекомендуется для стабильной работы
- Не устанавливайте poll менее 50 мс для RS485

⚠️ **СОВМЕСТИМОСТЬ**:
- Эта документация актуальна для ядра с типами 0-22
- Убедитесь, что ваша версия LightHub поддерживает нужные типы

---

**Последнее обновление**: 24 января 2026 г.  
**Актуально для**: LightHub Core v2.x  
**Автор**: Документация LightHub Project  
**Лицензия**: Apache 2.0
