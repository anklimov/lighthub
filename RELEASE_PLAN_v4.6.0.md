# RELEASE PLAN v4.6.0

Статус: подготовка (docs-фаза завершена 2026-10-02).

## Что вошло в релизную линию с v4.5.2 (28 коммитов)

- Multivent (CH 18) — завершён, в продакшене
- Sprinkler (CH 23) — релиз
- CAN (full-featured driver, lightspot) — релиз с v4.4.0
- Rotary encoder (T:32) с профилями
- DRY-режим полива: переопределяет ON, персистентен; статусные публикации при старте/смене режима; FREEZE > DISABLE
- Mercury: длинный таймер сессии (uint32 pollingInterval)
- OTA на ESP (ArduinoOTA, partitions min_spiffs)

## Готово (этап «документация и код», 2026-10-02)

- [x] Аудит всей документации репо по коду; исправлены расхождения (типы каналов, суффиксы, команды, JSON-форматы, HTTP API)
- [x] Импорт 16 страниц dokuwiki в репо (core/integrations/hardware) + снапшот wiki в `documentation/reference/dokuwiki-2026-10-02/`
- [x] Перенос авторских docx: CAN (`integrations/can_lightspot.md`), Rotary Encoder (`core/rotary_encoder.md`); WIP-секции помечены «Не реализовано»
- [x] Реструктуризация `documentation/`: core/ integrations/ modules/ hardware/ reference/; новый мастер-индекс `documentation/README.md`; удалены AI-артефакты (MIGRATION_GUIDE, START_HERE, COMPLETION_REPORT и пр.)
- [x] `CHANGELOG.md` в корне (ex CHANGELOG_v2.md)
- [x] EN-комментарии Doxygen по всем ключевым заголовкам (item, itemCmd, abstractch/in/out, inputs, utils, config, main, candriver, owTerm, ipmodbus, dmx, statusled, options, все modules/out_*, in_ccs811_hdc1080, colorchannel, bright)
- [x] `doxygen-docs/Doxyfile` — относительные пути, `main.md`, PROJECT_NUMBER 4.6.0; генерация `doxygen-docs/html` (gitignored)
- [x] `lighthub.github.io` (отдельная папка `~/dev/lighthub.github.io`): `index.md` + `doxygen/`, `git init` (push — руками)
- [x] Корневой `README.md` актуализирован (16 целей, CAN/sprinkler/multivent, ссылки)
- [x] Контрольная компиляция due — SUCCESS

## Осталось (следующие этапы / другие сессии)

### 1. GitHub Wiki (EN, полная версия)
- Порт актуальной документации в GitHub Wiki (EN): структура по `documentation/README.md`
- Источники: `documentation/` (RU) + этот план; перевод, а не копирование

### 2. DokuWiki (RU)
- Обновить lazyhome.ru wiki: синхронизировать со снапшотом 2026-10-02 изменения (CAN 50 kbit/s, новые типы, sprinkler DRY-фичи, rotary encoder)
- Решить: wiki остаётся RU-источником, репо — каноническим

### 3. Сайт lighthub.github.io
- Push репо `lighthub.github.io` (папка `~/dev/lighthub.github.io`)
- При желании: домен, CI-перегенерация doxygen (GitHub Actions) при изменениях кода

### 4. Релиз v4.6.0
1. `pio run` по всем 16 env (due, mega2560, mega2560slim, mega2560-optiboot, mega2560-5100, esp8266-wifi, esp32-wifi, esp32c3-wifi, lighthub21, m5stack, nrf52840, nrf52840-5500, controllino, stm32, stm32-enc2860, stm32-noip)
2. Бинарники в `compiled/<target>/` (git) — или в GitHub Release assets
3. Тег `v4.6.0` + GitHub Release с release notes (из CHANGELOG.md + этот план)
4. Проверить OTA-процедуру на живых устройствах

## Как перегенерировать Doxygen

```bash
brew install doxygen           # 1.18+
doxygen doxygen-docs/Doxyfile  # из корня репо → doxygen-docs/html
cp -R doxygen-docs/html ~/dev/lighthub.github.io/doxygen
cd ~/dev/lighthub.github.io && git add -A && git commit -m "doxygen update" && git push
```
