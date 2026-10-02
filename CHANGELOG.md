# Changelog

All notable releases of the LightHub firmware.
Detailed release notes are published on [GitHub Releases](https://github.com/anklimov/lighthub/releases).

## v4.6.0 (unreleased)

- Sprinkler (CH 23): DRY mode now overrides ON; DRY mode survives restart (persisted in config);
  status topics published on controller start and on ON/OFF mode change
- Sprinkler: `FREEZE` takes precedence over `DISABLE` in `/ctrl` status
- Multivent (CH 18): finalized (released in production); zone-level `FREEZE`/`UNFREEZE`,
  `HIGH`/`MEDIUM`/`LOW` fan control, `boost`/`supress`/`overridecmd`/`turnbyfan` AC-node options
- Mercury (CH 22): `pollingInterval` widened to 32-bit (longer timer)
- CAN: full-featured driver (released in v4.4.0) — controller-to-controller communication,
  lightspot non-IP controllers, config download and OTA over CAN
- Rotary encoder input (T:32) with profiles (`act`/`activate`)
- OTA for ESP32/ESP8266 via ArduinoOTA with `min_spiffs.csv` partitions
- Build matrix: 16 targets (due, mega2560 family, esp8266-wifi, esp32-wifi, esp32c3-wifi,
  lighthub21, m5stack, nrf52840, nrf52840-5500, controllino, stm32, stm32-enc2860, stm32-noip)
- Documentation: full audit against source code, restructured `documentation/` tree,
  in-repo port of the official wiki, EN Doxygen comments + developer site

## v4.5.2

Bug-fix release (see [release notes](https://github.com/anklimov/lighthub/releases)).

## v4.5.1

Bug-fix release (see [release notes](https://github.com/anklimov/lighthub/releases)).

## v4.5.0

See [release notes](https://github.com/anklimov/lighthub/releases).

## v4.4.0

- Full-featured CAN driver released — inter-controller communication and non-IP lightspot support

## v4.3.x and earlier

See [GitHub Releases](https://github.com/anklimov/lighthub/releases).
