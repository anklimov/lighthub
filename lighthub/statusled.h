/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Homepage: http://lazyhome.ru
GIT:      https://github.com/anklimov/lighthub
e-mail    anklimov@gmail.com

*/
#pragma once
#include <Arduino.h>

/** @brief LED pattern bit: red component on. */
#define ledRED 1
/** @brief LED pattern bit: green component on. */
#define ledGREEN 2
/** @brief LED pattern bit: blue component on. */
#define ledBLUE 4
/** @brief LED pattern bit: blink at the normal rate. */
#define ledBLINK 8
/** @brief LED pattern bit: blink at the fast rate. */
#define ledFASTBLINK 16
/** @brief Mask of all steady/blink pattern bits. */
#define ledParams (ledRED | ledGREEN | ledBLUE | ledBLINK | ledFASTBLINK)

/** @brief Internal flag: show the pattern once (flash). */
#define ledFlash 32
/** @brief Internal flag: LED is currently in the "hidden" (off) blink phase. */
#define ledHidden 64

#if defined(ARDUINO_ARCH_AVR)
#define pinRED 47
#define pinGREEN 48
#define pinBLUE 49
#else
// Due: RGB status LED on pins 50-52
#define pinRED 50
#define pinGREEN 51
#define pinBLUE 52
#endif

/** @brief Blink period in milliseconds (normal rate). */
#define ledDelayms 1000UL
/** @brief Blink period in milliseconds (fast rate). */
#define ledFastDelayms 300UL

/**
 * @brief RGB status LED controller.
 *
 * Drives the onboard RGB status LED (Due: pins 50-52, AVR: 47-49) with
 * color patterns, single flashes and slow/fast blinking. Used by the
 * system to signal state (e.g. errors blink red).
 */
class StatusLED {
public:
  /**
   * @brief Initializes the LED pins and sets the initial pattern.
   * @param pattern Initial LED pattern bits.
   */
  StatusLED(uint8_t pattern = 0);
  /**
   * @brief Sets the steady/blinking LED pattern.
   * @param pattern LED pattern bits.
   */
  void set (uint8_t pattern);
  /**
   * @brief Immediately drives the pins to the given color (no state change).
   * @param pattern LED pattern bits.
   */
  void show (uint8_t pattern);
  /**
   * @brief Advances blink/flash timing; call from the main loop.
   */
  void poll();
  /**
   * @brief Shows the given pattern once (single flash).
   * @param pattern LED pattern bits to flash.
   */
  void flash(uint8_t pattern);
private:
  /** @brief Current pattern including internal state flags. */
  uint8_t curStat;
  /** @brief millis() timestamp of the last timing step. */
  uint32_t timestamp;
};
