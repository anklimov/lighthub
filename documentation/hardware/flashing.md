# Заливка прошивки

> Источник: Wiki LightHub (lazyhome.ru), страница «заливка_прошивки». Снапшот от 2026-10-02.

## Заливка уже откомпилированной прошивки

В первую очередь необходимо скачать сам проект.
Это можно сделать либо с использованием утилиты git:
`git clone https://github.com/anklimov/lighthub.git`

Либо, если у вас она не установлена - открываем [https://github.com/anklimov/lighthub](https://github.com/anklimov/lighthub)
Далее, нажимаем зеленую кнопку "Clone or download"
Выбираем пункт "Download ZIP", скачиваем и распаковываем архив

Переходим в директорию lighthub-master/compiled/<требуемая_платформа> (список платформ выше)

Подключаете плату USB кабелем к компьютеру (кроме платформы NRF52 - там программирование только при помощи специального устройства JLink, об этом позже)

В диспетчере устройств находите порт, на который села плата (например, com3)
корректируете номер порта в файле upload.bat, который находится в директории, в которую вы вошли

Запускаете файл на выполнение (если выдается запрос "нажмите любую клавишу" - нажимаете)
Прошивка должна загрузиться в ваш контроллер.

## Компиляция и заливка с использованием platformio + Atom или VSC

Проект создан с использование Platformio , аналога Arduino IDE.

Сперва необходимо скачать Visual Studio Code или Atom, установить Platformio в качестве плагина. В интернете много материалов и видеоуроков на эту тему. Скорее всего потребуется установить клиент Git (Для Windows https://git-scm.com/download/win).

Далее, как и в предыдущем случае, необходимо скачать сам проект.
Проект создан с использование Platformio , аналога Arduino IDE.

Это можно сделать либо с использованием утилиты git:
`git clone https://github.com/anklimov/lighthub.git`

Либо, если у вас она не установлена - открываем  [https://github.com/anklimov/lighthub](https://github.com/anklimov/lighthub)
Далее, нажимаем зеленую кнопку "Clone or download"
Выбираем пункт "Download ZIP", скачиваем и распаковываем архив

В Atom или VSC выбираем пункт "Открыть директорию" (Open Folder), и указываем директорию lighthub.master (или в директорию lighthub если использовался git)

Далее, выбираем файл platformio.ini и раскомментариваем в нем нужный вам вариант платформы (платы) в пункте env_default (убираем ; перед строкой, содержащей нужную платформу, добавляем - перед строкой, содержащей ненужные платформы)

NEW - если используете среду VSC (рекомендуется) - то можете ничего не комментировать/раскомментировать а просто выбрать нужную платформу в меню "switch PlatformIO Project environment" (нажмите на Default (lighthub) в самом низу экрана)

**Для платформы DUE:** обязательно придется найти фаил variant.cpp по пути

`.platformio/packages/framework-arduinosam/variants/arduino_due_x`

 и вставить в него **последней строкой** следующую строку

`void USART0_Handler(void) __attribute__((weak));`

сохранить.

(К сожалению, внести это простое изменение в штатный репозиторий arduino не удалось никому из тех, кто создавал Pull request)


Нажимаем кнопку "Build"
Platformio скачивает все нужные библиотеки и компилирует код. Возможно придется несколько раз повторить эту операцию, так как по непонятным причинам, platformio может выдавать ошибки, не скачав какую-то библиотеку.

Если компиляция прошла успешно, можно подключить плату USB кабелем и нажать кнопку Upload



## Компиляция и заливка с использованием platformio. Вариант командная строка

Для того что бы прошить плату в первую очередь необходимо скачать сам проект [https://github.com/anklimov/lighthub](https://github.com/anklimov/lighthub) и установить Platformio


* Если Вы считатет себя джедаем и не ищете легких путей то работать с платформио можно через командную строку. Для этого нужно:
  -   установить Python 2.7,
  -   установить git.

* Модифицировать PATH. идем: мой компьютер->правой кнопокой мыши вызываем контекстное меню->свойства->в появившемся окне "переменные среды"-> в окне "Системные переменные" выбираем переменную Path ->изменить->добавляем строку

`;C:\Python27;C:\Python27\Scripts`

* Установить Platformio открываем командную строку (поиск в "Пуске" cmd.exe) вводим команду

`pip install -U platformio`

Учтите, что Platformio установится по умолчанию в папку пользователя `С:\Пользователи\Имя пользователя\.platformio` Python 2.7 не поддерживает кириллицу, если имя пользователя написано кириллицей, необходимо скопировать папку Platformio в корень диска C:\ и изменить путь к рабочему каталогу в фале platformio.ini (который находится в ранее скачанной папке lighthub-master), указав после строки [platformio] следующую строку: `home_dir = C:\.platformio`

Сохраняем файл.
Открываем папку lighthub-master, в строке отображающей путь вводим cmd и нажимаем enter или же выбираем папку в командной строке любым другим способом.

**Для платформы DUE:** обязательно придется найти фаил variant.cpp по пути

`.platformio/packages/framework-arduinosam/variants/arduino_due_x`

 и вставить в него **последней строкой** следующую строку

`void USART0_Handler(void) __attribute__((weak));`

сохранить.

Далее вводим `pio run -e <платформа>`

Вариант <платформа> выберете в зависимости от используемой аппаратной платформы.

```
; Arduino Mega compact build + Ethernet shield Wiznet 5X00, 1-wire, thermostates.
;    mega2560slim

; Another Arduino Mega compact build without 1-wire, DMX, but with OTA
; OPTIBOOT bootloader required! https://github.com/MCUdude/MegaCore
;    mega2560-optiboot

; Arduino Mega + Ethernet shield Wiznet 5100
;    mega2560-5100

; Arduino Mega (Ethernet shield Wiznet 5X00, DMX, Artnet)
;    mega2560

; LightHub controller HW revision 2.1 and above (Wiznet 5500 CS on pin 53)
; Если у вас плата LightHub версии 2.1 или более новая + DUE - используйте эту конфигурацию
;    **lighthub21**

; Arduino DUE + Ethernet shield Wiznet 5X00
; Предпочтительный вариант для DUE с Ethernet Shield (не для платы LightHub), базированный на универсальном драйвере
;    **due**

; ESP 8266
;    **esp8266-wifi**

; ESP-32
;    **esp32-wifi**

; ESP-32-C3
;    **esp32c3-wifi**

; M5Stack core (ESP32 based)
;    m5stack

; Controllino
;    controllino

; STM32 board + ENC28j60 network
;    stm32-enc2860

; STM32 board (olmexino) with WIZNET 5X00 network
;    stm32

; STM32 board, no IP (LightSpot)
;    stm32-noip

; NRF52 board + Wiznet 5x00 network
;    nrf52840
```

> **Примечание (сверка с кодом, 2026-10-02):** в оригинальном тексте wiki список целей в этом блоке был устаревшим — там фигурировали `mega2560slim-5100`, `mega2560-5500`, `due-5100`, `due-5500` и `nrf52840-5500`, а отсутствовали `nrf52840`, `mega2560`, `mega2560-optiboot`, `esp32c3-wifi`, `stm32`, `stm32-noip`. Список выше восстановлен по фактическим окружениям из `platformio.ini`.

Пример: "due" - это плата Arduino DUE + Ethernet Shield на чипе Wiznet-5X00 (универсальный драйвер)

**В общем случае, используйте варианты без указания 5100 или 5500 - в этом случае будет использован универсальный драйвер Wiznet**

Если плата LightHub версии 2.1 или выше то используйте опцию "lighthub21"

Проект должен скомпилироваться без ошибок.

**ЧАСТО Platformio выдает ошибки на ровном месте (не находит какие-либо файлы). В этом случае, повторный запуск компиляции часто решает эту проблему**

Если ошибок нет вводим `pio run -e <платформа> -t upload`
Программа должна залиться в аппаратную платформу.

Если возникли ошибки:
Попробовать очистить папку с библиотеками .piolibdeps и .pioenv удалив все их содержимое


## Использование Arduino IDE для компиляции и заливки прошивки
Это достаточно трудоемкий процесс, который требует, чтобы вы вручную установили массу библиотек в IDE.
platformio - намного более гибкое и профессиональное средство - лучше и проще использовать его.
Если для кого-то будет важно освоить именно этот метод - пишите, мы пополним инструкцию.


Если возникают проблемы - пишите на форуме или в [группу Телеграмм](https://t.me/lighthub)

(Вы, также, можете улучшать эту инструкцию)

---

## Сверка с кодом (2026-10-02)

**Фактические окружения (env) в `platformio.ini`:**

| env | Платформа / описание | Папка в `compiled/` |
|-----|----------------------|---------------------|
| `due` | Arduino DUE, универсальный Wiznet-драйвер | `due/` |
| `lighthub21` | LightHub HW rev 2.1+ (Wiznet 5500, CS на пине 53), DUE | `lighthub21/` |
| `mega2560` | Arduino Mega + Wiznet 5X00, DMX, Artnet | `mega2560/` |
| `mega2560slim` | Mega компактная сборка + Wiznet, 1-wire, термостаты | `mega2560slim/` |
| `mega2560-optiboot` | Mega компактная + OTA (нужен OPTIBOOT) | `Mega2560-optiboot/` |
| `mega2560-5100` | Mega + Ethernet Wiznet 5100 | `mega2560-5100/` |
| `esp8266-wifi` | ESP8266 (Wi-Fi) | `esp8266-wifi/` |
| `esp32-wifi` | ESP32 (Wi-Fi) | `esp32-wifi/` |
| `esp32c3-wifi` | ESP32-C3 (Wi-Fi) | `esp32c3-wifi/` |
| `m5stack` | M5Stack (ESP32) | `m5stack/` |
| `nrf52840` | NRF52840 + Wiznet 5x00 | `nrf52840/` |
| `controllino` | Controllino (AVR) | `controllino/` |
| `stm32` | STM32 (olmexino) + WIZNET 5X00 | `stm32/` |
| `stm32-enc2860` | STM32 + ENC28J60 | `stm32-enc2860/` |
| `stm32-noip` | STM32, без IP-стека (LightSpot) | — (папки нет) |

(Также в `platformio.ini` есть служебные env `Upload_ISP` и `fuses_bootloader` для прошивки OPTIBOOT — они не являются целевыми сборками.)

**Папки в `compiled/`:** `controllino`, `due`, `DUE_16u2_reflash`, `esp32-wifi`, `esp32c3-wifi`, `esp8266-wifi`, `lighthub21`, `m5stack`, `mega2560`, `mega2560-5100`, `Mega2560-optiboot`, `mega2560slim`, `nrf52840`, `nrf52840-5500`, `stm32`, `stm32-enc2860`, `tools`.

**Расхождения wiki ↔ кода:**
- Список целей в wiki устарел: упоминаются несуществующие env `mega2560slim-5100`, `mega2560-5500`, `due-5100`, `due-5500`. В текущем `platformio.ini` их нет — для DUE и Mega используется универсальный Wiznet-драйвер (`due`, `mega2560`). Список в тексте исправлен по факту (см. примечание выше).
- `nrf52840-5500` в wiki указан как env, в `platformio.ini` такого env нет (есть только `nrf52840`), но в `compiled/` папка `nrf52840-5500/` присутствует — вероятно, это отдельная уже скомпилированная сборка, не являющаяся отдельным env.
- `stm32-noip` — есть в `platformio.ini`, но готовой папки в `compiled/` нет.
- `DUE_16u2_reflash/` в `compiled/` — утилита для перепрошивки USB-чипа 16u2, не целевая сборка.
- Замечание о DUE: инструкция про правку `variant.cpp` (добавление `void USART0_Handler(void) __attribute__((weak));`) актуальна — в репозитории собственного `lighthub/variant.cpp` нет, используется вариант DUE из пакета PlatformIO `framework-arduinosam`.
