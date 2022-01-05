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

#define ledRED 1
#define ledGREEN 2
#define ledBLUE 4
#define ledBLINK 8
#define ledFASTBLINK 16
#define ledParams (ledRED | ledGREEN | ledBLUE | ledBLINK | ledFASTBLINK)

#define ledFlash 32
#define ledHidden 64

#if defined(ARDUINO_ARCH_AVR)
#define pinRED 47
#define pinGREEN 48
#define pinBLUE 49
#else
#define pinRED 50
#define pinGREEN 51
#define pinBLUE 52
#endif

#define ledDelayms 1000UL
#define ledFastDelayms 300UL

class StatusLED {
public:
  StatusLED(uint8_t pattern = 0);
  void set (uint8_t pattern);
  void show (uint8_t pattern);
  void poll();
  void flash(uint8_t pattern);
private:
  uint8_t curStat;
  uint32_t timestamp;
};
