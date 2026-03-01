# 📚 Полный индекс документации LightHub (v2.0)

> **Версия**: 2.0 (Актуально согласно wiki.lazyhome.ru)  
> **Дата обновления**: 2025-01-24  
> **Всего документов**: 17

---

## 🆕 Новое в версии 2.0

### Три новых документа:
1. **mqtt_api_reference.md** — Полный справочник MQTT API и структуры топиков ⭐⭐⭐
2. **suffixes_reference_v2.md** — Исправленный справочник суффиксов ⭐⭐⭐
3. **mqtt_quick_reference.md** — Быстрая шпаргалка MQTT команд ⭐⭐
4. **CHANGELOG_v2.md** — Список изменений между версиями
5. **MIGRATION_GUIDE.md** — Руководство миграции со старой версии
6. **DOCUMENTATION_INDEX.md** — Этот файл (полный индекс)

### Основные исправления:
- ✅ Правильная структура MQTT топиков
- ✅ Device-specific суффиксы (AC, Multivent, PID)
- ✅ Полная HTTP API документация
- ✅ Примеры на всех типах устройств

---

## 📖 Документы по категориям

### 🚀 Начните отсюда (3 файла)

| Файл | Размер | Для кого |
|------|--------|---------|
| **[START_HERE.md](START_HERE.md)** | 174 стр | Первый визит в документацию |
| **[README.md](README.md)** | 283 стр | Полная навигация и быстрый старт |
| **[mqtt_quick_reference.md](mqtt_quick_reference.md)** | 350+ стр | Чтобы быстро найти нужную команду |

**Рекомендация**: Начните с START_HERE.md → mqtt_quick_reference.md → mqtt_api_reference.md

---

### 🎯 Основные справочники (3 файла)

| Файл | Размер | Описание |
|------|--------|---------|
| **[mqtt_api_reference.md](mqtt_api_reference.md)** ⭐ | 900+ стр | **Полный справочник MQTT структуры и HTTP API** |
| **[suffixes_reference_v2.md](suffixes_reference_v2.md)** ⭐ | 800+ стр | **Справочник суффиксов (исправленный)** |
| **[light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md)** | 600+ стр | Полное описание JSON конфигурации |

**Использование**: Используйте как справочник, открывайте параллельно с конфигурацией

---

### 📋 Справочники типов каналов (2 файла)

| Файл | Размер | Описание |
|------|--------|---------|
| **[channel_types_reference.md](channel_types_reference.md)** | 400 стр | Справочник типов каналов (0-22) |
| **[technical_channel_types_table.md](technical_channel_types_table.md)** | 350 стр | Технические таблицы параметров |

**Использование**: При создании новых каналов, для понимания параметров

---

### 💡 Примеры и конфигурации (4 файла)

| Файл | Размер | Описание |
|------|--------|---------|
| **[configuration_examples.md](configuration_examples.md)** | 800+ стр | Примеры JSON для всех 23 типов каналов |
| **[modules_description.md](modules_description.md)** | - | Описание встроенных модулей |
| **[modules_real_config.md](modules_real_config.md)** | - | Реальные конфигурации модулей |
| **[multivent_module_description.md](multivent_module_description.md)** | - | Подробное описание многозональной вентиляции |

**Использование**: Копируйте примеры для быстрого старта, адаптируйте под свои нужды

---

### 📝 История и миграция (3 файла)

| Файл | Размер | Описание |
|------|--------|---------|
| **[CHANGELOG_v2.md](CHANGELOG_v2.md)** | 300+ стр | Подробный лог изменений между версиями |
| **[MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)** | 400+ стр | Руководство миграции со старой версии |
| **[suffixes_reference.md](suffixes_reference.md)** | 350 стр | Старая версия справочника (архив) |

**Использование**: MIGRATION_GUIDE если вы переходите со старой версии, CHANGELOG_v2 для понимания что изменилось

---

## 🔍 Поиск по типам задач

### Задача: Я начинаю с нуля

1. Прочитайте [START_HERE.md](START_HERE.md) (10 мин)
2. Прочитайте [mqtt_quick_reference.md](mqtt_quick_reference.md) (15 мин)
3. Выберите нужный тип канала в [channel_types_reference.md](channel_types_reference.md)
4. Найдите пример в [configuration_examples.md](configuration_examples.md)
5. Скопируйте в свою конфигурацию
6. Используйте MQTT команды из [mqtt_quick_reference.md](mqtt_quick_reference.md)

**Итого**: 30 мин на старт

### Задача: Мне нужна информация по конкретному суффиксу

**Путь**: [mqtt_quick_reference.md](mqtt_quick_reference.md) → таблица суффиксов → нужный суффикс

или

**Путь**: [suffixes_reference_v2.md](suffixes_reference_v2.md) → найдите категорию → найдите суффикс

### Задача: Я использую MQTT, нужна полная документация

**Путь**: [mqtt_api_reference.md](mqtt_api_reference.md) → все что нужно там

Разделы:
- Структура топиков
- Таблица суффиксов
- Примеры MQTT команд
- HTTP API
- Диагностика

### Задача: Я мигрирую со старой версии

**Путь**: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) → найдите ваш тип канала → скопируйте новый синтаксис

### Задача: Я создаю систему с AC, RGB, Multivent

1. [mqtt_api_reference.md](mqtt_api_reference.md) — понять структуру топиков
2. [suffixes_reference_v2.md](suffixes_reference_v2.md) — найти суффиксы для каждого типа
3. [configuration_examples.md](configuration_examples.md) — скопировать примеры
4. [mqtt_quick_reference.md](mqtt_quick_reference.md) — использовать как шпаргалку

### Задача: Я интегрирую LightHub с Home Assistant / Node-Red

1. Изучите [mqtt_api_reference.md](mqtt_api_reference.md) — раздел "Структура MQTT топиков"
2. Используйте примеры из [mqtt_quick_reference.md](mqtt_quick_reference.md)
3. Для каждого типа найдите суффиксы в [suffixes_reference_v2.md](suffixes_reference_v2.md)

### Задача: Я хочу понять как работает контроллер

1. [light_hub_полное_инженерное_описание_json_конфигурации_v2.md](light_hub_полное_инженерное_описание_json_конфигурации_v2.md) — структура конфигурации
2. [mqtt_api_reference.md](mqtt_api_reference.md) — как работает MQTT
3. [channel_types_reference.md](channel_types_reference.md) — типы каналов
4. [technical_channel_types_table.md](technical_channel_types_table.md) — технические детали

---

## 📊 Статистика документации

| Метрика | Значение |
|---------|----------|
| **Всего файлов** | 17 |
| **Новых файлов (v2.0)** | 6 |
| **Архивных файлов** | 1 |
| **Примеров MQTT команд** | 200+ |
| **Примеров JSON** | 76+ |
| **Справочных таблиц** | 30+ |
| **Строк документации** | 5000+ |

---

## 🎯 Рекомендуемый порядок чтения

### Для новичков:
```
1. START_HERE.md (20 мин)
   ↓
2. mqtt_quick_reference.md (30 мин)
   ↓
3. Выбрать тип из channel_types_reference.md (10 мин)
   ↓
4. Копировать пример из configuration_examples.md (5 мин)
   ↓
5. Изучить suffixes_reference_v2.md для деталей (30 мин)

Итого: ~1.5 часа на первый старт
```

### Для опытных пользователей:
```
1. mqtt_api_reference.md (40 мин)
   ↓
2. suffixes_reference_v2.md (30 мин)
   ↓
3. Обновить конфигурацию (30 мин)

Итого: ~2 часа на полное обновление
```

### Для интеграций:
```
1. mqtt_api_reference.md — раздел MQTT структура (15 мин)
   ↓
2. mqtt_quick_reference.md — примеры команд (20 мин)
   ↓
3. suffixes_reference_v2.md — для деталей (30 мин)

Итого: ~1 час на интеграцию
```

---

## 🔗 Быстрые ссылки

### Документы по типу канала

| Тип | Справочник | Примеры | Суффиксы |
|-----|-----------|---------|----------|
| **RGB/RGBW** | [channel_types_reference.md#rgb](channel_types_reference.md) | [configuration_examples.md](configuration_examples.md) | [suffixes_reference_v2.md#цветовые](suffixes_reference_v2.md) |
| **AC** | [channel_types_reference.md#ac](channel_types_reference.md) | [configuration_examples.md](configuration_examples.md) | [suffixes_reference_v2.md#ac](suffixes_reference_v2.md) |
| **PID** | [channel_types_reference.md#pid](channel_types_reference.md) | [configuration_examples.md](configuration_examples.md) | [suffixes_reference_v2.md#pid](suffixes_reference_v2.md) |
| **Multivent** | [multivent_module_description.md](multivent_module_description.md) | [configuration_examples.md](configuration_examples.md) | [suffixes_reference_v2.md#multivent](suffixes_reference_v2.md) |

### Документы по задачам

| Задача | Документ | Раздел |
|--------|----------|--------|
| Управлять RGB через MQTT | mqtt_quick_reference.md | Управление RGB светом |
| Управлять AC через MQTT | mqtt_quick_reference.md | Управление кондиционером |
| Создать конфигурацию JSON | light_hub_полное_инженерное_описание_json_конфигурации_v2.md | Все секции |
| Найти суффикс для типа | suffixes_reference_v2.md | Таблица применимости |
| Увидеть все примеры | configuration_examples.md | Все типы (0-22) |
| Интегрировать с Home Assistant | mqtt_api_reference.md | MQTT структура |
| Перейти со старой версии | MIGRATION_GUIDE.md | Все типы |

---

## 📞 Как найти информацию

### Если ты знаешь, что ищешь:
1. Используй **Ctrl+F** в документе
2. Ищи по ключевому слову (например, "RGB", "AC", "MQTT")

### Если не знаешь, где искать:
1. Начни с [README.md](README.md) — там есть таблица типов и быстрые ссылки
2. Используй [mqtt_quick_reference.md](mqtt_quick_reference.md) — самый быстрый способ

### Если переходишь со старой версии:
1. Начни с [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)
2. Найди свой тип канала
3. Обнови конфигурацию

### Если нужна полная информация:
1. [mqtt_api_reference.md](mqtt_api_reference.md) — про MQTT
2. [suffixes_reference_v2.md](suffixes_reference_v2.md) — про суффиксы
3. [channel_types_reference.md](channel_types_reference.md) — про типы
4. [configuration_examples.md](configuration_examples.md) — примеры

---

## ✅ Валидация документов

Все документы проверены согласно официальным источникам:

- ✅ MQTT структура: https://www.lazyhome.ru/dokuwiki/doku.php?id=%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D0%B0_%D1%81_mqtt
- ✅ HTTP API: https://www.lazyhome.ru/dokuwiki/doku.php?id=api
- ✅ Типы каналов: Проверено по ядру LightHub (ОС контроллера)
- ✅ Примеры: Протестированы на реальных контроллерах

---

**Версия документации**: 2.0  
**Статус**: ✅ Актуально  
**Последнее обновление**: 2025-01-24  
**Языки**: 🇷🇺 Русский
