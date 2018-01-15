/* Copyright © 2017 Andrey Klimov. All rights reserved.

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

#include <Arduino.h>
void PrintBytes(uint8_t* addr, uint8_t count, bool newline=0); 
void SetBytes(uint8_t* addr, uint8_t count, char * out);
void SetAddr(char * out,  uint8_t* addr);
uint8_t HEX2DEC(char i);
int getInt(char ** chan);
