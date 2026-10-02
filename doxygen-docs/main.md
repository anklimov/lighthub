# LightHub

LightHub is a flexible open-source smart home controller firmware for Arduino Mega 2560,
Arduino Due, ESP8266, ESP32, ESP32-C3, STM32, NRF52840 and M5Stack boards.

It turns a microcontroller into an MQTT-enabled controller for:

- lighting (relays, PWM/DMX dimmers, RGB/RGBW/RGBWW, addressable NeoPixel strips);
- HVAC (thermostats, PID underfloor heating, multizone ventilation `VENTS`, Haier AC);
- irrigation (autonomous sprinkler controller with tank level management);
- meters (Mercury energy meters, generic counters);
- sensors (1-Wire, DHT, CCS811, HDC1080, analog inputs);
- integration buses: Modbus RTU, CAN (lightspot), 1-Wire, UART bridges.

## Getting started

- In-repo user documentation (RU): `documentation/README.md`
- Wiki: https://www.lazyhome.ru/dokuwiki/doku.php?id=start
- Project site: https://lighthub.github.io/

## Core concepts

- **Item** (`item.h`) — a virtual channel of a given type (`CH_*`, 0..23) with arguments,
  current value and last command. All external control goes through `Item::Ctrl()`.
- **Suffix** (`suffix_P` in `item.h`) — the topic suffix that selects the action:
  `cmd`, `set`, `val`, `del`, `HSV`, `RGB`, `fan`, `mode`, `ctrl`, `hue`, `sat`, `temp`, `raw`.
- **Command** (`itemCmd.h`) — the payload action: `ON`, `OFF`, `TOGGLE`, `INCREASE`, `DECREASE`,
  `ENABLE`, `DISABLE`, `FREEZE`, `UNFREEZE`, `AUTO`, `HEAT`, `COOL`, `DRY`, `FAN_ONLY`, ...
- **MQTT topics** — `root/[id|bcst|out]/item/[subitem/]suffix` (defaults: `myhome/in`, `myhome/s_out`).
- **Drivers** (`modules/out_*.cpp`) — one class per channel type; `Setup()` parses JSON config,
  `Ctrl()` handles commands, `Poll()` publishes status.
- **Inputs** (`inputs.cpp`) — buttons (single/double/triple/long presses), analog inputs,
  rotary encoder (`T:32`), DHT/CCS811/HDC1080 sensors.
- **CAN** (`candriver.cpp`) — 50 kbit/s bus for non-IP "lightspot" controllers,
  config download and controller-to-controller communication.

## Documentation layout

| Group | Files |
|-------|-------|
| Channel core | `item.h`, `itemCmd.h`, `abstractch.h`, `abstractin.h`, `abstractout.h`, `colorchannel.h`, `bright.h` |
| Inputs | `inputs.h` (+ rotary encoder), `modules/in_ccs811_hdc1080.h` |
| Config / main | `config.h`, `main.h`, `options.h`, `systemconfigdata.h`, `textconst.h` |
| Buses | `candriver.h`, `ipmodbus.h`, `owTerm.h`, `dmx.h` |
| Drivers | `modules/out_*.h` (one class per `CH_*` type) |
