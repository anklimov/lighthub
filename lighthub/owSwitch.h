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

//define APU_OFF
#include <DS2482_OneWire.h>
#include <DallasTemperature.h>



int  owRead2408(uint8_t* addr);
int  ow2408out(DeviceAddress addr,uint8_t cur);
//int  read1W(int i);
int  cntrl2408(uint8_t* addr, int subchan, int val=-1);
int cntrl2413(uint8_t* addr, int subchan, int val=-1);
int  cntrl2890(uint8_t* addr, int val);
