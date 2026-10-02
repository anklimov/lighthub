# Backlog

Known issues and deferred items (code and docs). Format: `ID — item — context/status`.

## Code

- **AC-LEGACY** — Remove the legacy `queit` subitem alias for AC quiet mode
  (`out_ac.cpp`, `QUIET_LEGACY_P`). Fixed the typo to `quiet` in v4.6.0; drop the alias
  in the next major release.
- **RGB-DEAD** — The `/RGB` topic suffix is dead: `S_RGB` is not handled by the core
  (`item.cpp`) nor by color drivers (`colorchannel.cpp`); numeric strings in the common/`/set`
  topic are parsed as HSV. Either implement RGB input parsing or remove `/RGB` from the
  suffix table.
- **HEX-RRGGBB** — Home Remote `#RRGGBB` notation does not work: `getIntFromStr`
  (`utils.cpp`) silently skips hex characters, so `#FF0000` yields 0. Either implement
  hex parsing in `Ctrl()` or drop the notation from docs.
- **PID-CHAN-TYPE** — `out_pid::getChanType()` returns `CH_THERMO` (5) instead of `CH_PID` (13)
  (`out_pid.cpp`). Verify whether this is intentional (core treats PID as a thermostat for
  ON/OFF semantics) and document or fix.
- **CAN-SPI** — External SPI CAN controller support for chips without built-in CAN
  (Mega2560, ESP8266) — designed in the CAN draft doc, not implemented in `candriver.cpp`.
- **ROTARY-T99** — Rotary input `T:99` with profiles and `goto`/`onenter` handlers
  ("scenario" rotary encoder) — designed in the rotary draft doc, not implemented.
- **PWM-ESP32** — PWM on ESP32 is a stub: `analogWrite` body is `//TBD`
  (`out_pwm.cpp`). CH_PWM/RGB/RGBW/RGBWW do not work on ESP32 targets.
- **SPILED-DMX-SUBITEM** — LED pixel addressing via subitem (10-20) works only for
  CH_SPILED; the DMX driver ignores subitem. Clarify or implement.

## Docs

- **DOC-EN** — Bilingualization: port RU docs to EN (GitHub Wiki / developer site).
  Baseline: `documentation/reference/dokuwiki-2026-10-02/` + audited RU docs.
- **DOC-WIKI** — Sync the lazyhome.ru dokuwiki with the repo (CAN 50 kbit/s, sprinkler DRY
  features, rotary encoder, new channel types).
- **DOC-SITE** — lighthub.github.io: push, optional CI regeneration of Doxygen on code
  changes.

## Release v4.6.0 (see RELEASE_PLAN_v4.6.0.md)

- Build all 16 targets, attach binaries, tag `v4.6.0`, release notes.
