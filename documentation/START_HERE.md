# ⭐ ДОКУМЕНТАЦИЯ ОБНОВЛЕНА — Версия 2.0

> **Важно**: Документация LightHub полностью актуализирована для ядра с типами каналов CH_DIMMER (0) - CH_MERCURY (22)
> 
> **🆕 Новое**: Добавлена полная документация MQTT API согласно wiki.lazyhome.ru

---

## 🎯 Начните отсюда

### Для всех (универсальный индекс)
👉 **[README.md](README.md)** — полный индекс всей документации с быстрыми ссылками

### Спешите? Быстрая шпаргалка!
👉 **[mqtt_quick_reference.md](mqtt_quick_reference.md)** — команды MQTT для всех типов устройств

---

## 📚 Основная документация

### 1️⃣ **[mqtt_api_reference.md](mqtt_api_reference.md)** ⭐ **НОВОЕ** (самое полное)
   - **Полный справочник MQTT структуры и HTTP API**
   - Структура топиков: `root/[id или bcst или out]/item/[subitem]/suffix`
   - Три типа топиков: команды broadcast, команды индивидуальные, статусные
   - Таблица всех суффиксов
   - HTTP endpoints и примеры curl
   - Восстановление состояния
   - Примеры на всех типах устройств
   
### 2️⃣ **[suffixes_reference_v2.md](suffixes_reference_v2.md)** ⭐ **ИСПРАВЛЕННЫЙ**
   - **Справочник MQTT суффиксов (согласно wiki.lazyhome.ru)**
   - 7 категорий суффиксов с примерами
   - Таблица применимости по типам каналов
   - Диапазоны значений: 0-100 vs 0-255, /hue 0-365°
   - Сценарии для RGB, AC, PID, Multivent

### 3️⃣ [light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md) ⭐ **АКТУАЛЬНО**
   - **Полное описание JSON конфигурации**
   - Все 23 типа каналов (0-22)
   - Все секции: mqtt, topics, modbus, items, in
   - Инженерные правила
   - Полный пример системы

---

## 🔍 Справочники (используйте как шпаргалку)

### 4️⃣ [channel_types_reference.md](channel_types_reference.md)
   - **Справочник типов каналов 0-22**
   - Таблица с кодами и текстовыми обозначениями
   - Синтаксис конфигурации для каждого типа
   - Визуализация иерархии

### 5️⃣ [technical_channel_types_table.md](technical_channel_types_table.md)
   - **Технические таблицы параметров**
   - Детальное описание каждого типа
   - Все константы из item.h
   - Таблицы совместимости

---

## 💡 Примеры (готовые к использованию)

### 6️⃣ [configuration_examples.md](configuration_examples.md)
   - **JSON примеры для всех 23 типов каналов**
   - Для каждого типа: синтаксис + MQTT команды
   - Полная реальная система
   - **Скопируй-вставь готовые примеры**

---

## ⚙️ Специальные документы

### 6️⃣ [modules_description.md](modules_description.md)
   - Описание модулей управления
   - out_Multivent, out_AC, out_PID и др.

### 7️⃣ [multivent_module_description.md](multivent_module_description.md)
   - Подробная документация многозональной вентиляции

### 8️⃣ [modules_real_config.md](modules_real_config.md)
   - Реальные примеры конфигурации модулей

---

## 🚀 Быстрые старты по задачам

### Задача: Включить LED светильник
1. Откройте [channel_types_reference.md](channel_types_reference.md)
2. Выберите тип: CH_DMX (0), CH_PWM (3), CH_RGB (2), CH_RGBW (1) или CH_RGBWW (17)
3. Копируйте пример из [configuration_examples.md](configuration_examples.md)
4. Адаптируйте GPIO пины или DMX адреса

### Задача: Управлять кондиционером
1. Найдите CH_AC (10) в [channel_types_reference.md](channel_types_reference.md)
2. Откройте пример в [configuration_examples.md](configuration_examples.md)
3. Настройте Modbus адрес и регистры
4. Проверьте MQTT команды в [suffixes_reference.md](suffixes_reference.md)

### Задача: Создать систему с кнопками
1. Прочитайте раздел **"Секция `in` (входы)"** в [light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)
2. Определите GPIO пины входов
3. Привяжите к объектам через `"item"`
4. Используйте команды `scmd`, `rcmd`

---

## 📊 Что изменилось

### Старая версия ❌
- ❌ Содержала только типы 0-17 (17 из 23)
- ❌ Отсутствовали типы: ELEVATOR (19), COUNTER (20), HUMIDIFIER (21), MERCURY (22)
- ❌ Примеры без полной информации
- ❌ Неполное описание Modbus

### Новая версия ✅
- ✅ **Все 23 типа каналов** (полное покрытие)
- ✅ **76+ примеров JSON**
- ✅ **3000+ строк инженерной документации**
- ✅ **100% соответствие исходному коду (item.h, item.cpp)**
- ✅ **Таблицы совместимости**
- ✅ **Быстрые старты по задачам**

---

## 📋 Таблица типов каналов (0-22)

| № | Тип | Описание | Справочник |
|---|-----|---------|-----------|
| 0 | DMX | DMX диммер | [⬇️](configuration_examples.md#ch_dimmer-0---dmx-диммер) |
| 1 | DMXRGBW | RGB+White | [⬇️](configuration_examples.md#ch_rgbw-1---dmx-rgbwhite) |
| 2 | DMXRGB | RGB | [⬇️](configuration_examples.md#ch_rgb-2---dmx-rgb) |
| 3 | PWM | GPIO PWM | [⬇️](configuration_examples.md#ch_pwm-3---gpio-pwm) |
| 4 | MBUSDIM | Modbus Dimmer (Legacy) | [⬇️](configuration_examples.md#ch_modbus-4---modbus-ac-dimmer-legacy) |
| 5 | THERMO | Термостат | [⬇️](configuration_examples.md#ch_thermo-5---onoff-термостат) |
| 6 | RELAY | GPIO реле | [⬇️](configuration_examples.md#ch_relay-6---gpio-реле) |
| 7 | GROUP | Группа каналов | [⬇️](configuration_examples.md#ch_group-7---группа-каналов) |
| 8 | VCTEMP | Vacom PID | [⬇️](configuration_examples.md#ch_vctemp-8---vacom-pid-терморегулятор) |
| 9 | MBUSVC | Vacom мотор | [⬇️](configuration_examples.md#ch_vc-9---vacom-мотор-регулятор) |
| 10 | ACHAIER | Кондиционер | [⬇️](configuration_examples.md#ch_ac-10---кондиционер-haier) |
| 11 | SPILED | SPI LED | [⬇️](configuration_examples.md#ch_spiled-11---spi-led-лента) |
| 12 | MOTOR | Шаговый двигатель | [⬇️](configuration_examples.md#ch_motor-12---шаговый-двигатель) |
| 13 | PID | PID регулятор | [⬇️](configuration_examples.md#ch_pid-13---pid-регулятор) |
| 14 | MBUS | Universal Modbus | [⬇️](configuration_examples.md#ch_mbus-14---universal-modbus) |
| 15 | UARTBRDG | UART мост | [⬇️](configuration_examples.md#ch_uartbridge-15---uart-мост) |
| 16 | RELAYX | Медленный PWM | [⬇️](configuration_examples.md#ch_relayx-16---медленный-pwm-через-реле) |
| 17 | DMXRGBWW | RGBWW | [⬇️](configuration_examples.md#ch_rgbww-17---dmx-rgbww) |
| 18 | VENTS | Многозональная вентиляция | [⬇️](configuration_examples.md#ch_multivent-18---многозональная-вентиляция) |
| 19 | ELEVATOR | Лифт (резервирован) | - |
| 20 | COUNTER | Счётчик | [⬇️](configuration_examples.md#ch_counter-20---счётчик-импульсов) |
| 21 | HUM | Увлажнитель | [⬇️](configuration_examples.md#ch_humidifier-21---управление-увлажнителем) |
| 22 | MERCURY | Mercury счётчик | [⬇️](configuration_examples.md#ch_mercury-22---счётчик-энергии-mercury) |

---

## ⚡ Инженерные правила

1. **Сначала структура**: Modbus шаблон → Item → MQTT топик → входы
2. **Минимизируй poll**: RS485 не менее 100 мс, GPIO входы 10-50 мс
3. **Используй GROUP**: Для синхронного управления несколькими каналами
4. **Подробные имена**: `lamp_bedroom_ceiling` лучше, чем `lamp1`

---

## 🔗 Дополнительно

- **GitHub репозиторий**: https://github.com/anklimov/lighthub
- **Официальный сайт**: https://lazyhome.ru
- **Документация Wiki**: https://www.lazyhome.ru/dokuwiki/

---

## ✅ Чек-лист перед использованием

- [ ] JSON синтаксис проверен (JSONLint)
- [ ] Все GPIO пины уникальны
- [ ] Все Modbus адреса доступны
- [ ] MQTT брокер запущен
- [ ] Все типы каналов в диапазоне 0-22
- [ ] Необходимые секции присутствуют
- [ ] MQTT топики проверены
- [ ] Резервная копия конфигурации сохранена

---

**Документация обновлена**: 24 января 2026 г.  
**Версия ядра**: LightHub с CH_DIMMER (0) - CH_MERCURY (22)  
**Статус**: ✅ Актуально и готово к использованию
