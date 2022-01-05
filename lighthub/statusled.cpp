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

#include "statusled.h"
#include "utils.h"


StatusLED::StatusLED(uint8_t pattern)
{
#if defined (STATUSLED)
  pinMode(pinRED, OUTPUT);
  pinMode(pinGREEN, OUTPUT);
  pinMode(pinBLUE, OUTPUT);
  set(pattern);
  timestamp=millis();//+ledDelayms;
#endif
}

void StatusLED::show (uint8_t pattern)
{
#if defined (STATUSLED)
    digitalWrite(pinRED,(pattern & ledRED)?HIGH:LOW );
    digitalWrite(pinGREEN,(pattern & ledGREEN)?HIGH:LOW);
    digitalWrite(pinBLUE,(pattern & ledBLUE)?HIGH:LOW);
#endif
}

void StatusLED::set (uint8_t pattern)
{
#if defined (STATUSLED)
    short newStat = pattern & ledParams;

    if (newStat!=(curStat & ledParams))
    {
    //if (!(curStat & ledHidden))
    show(pattern);
    curStat=newStat | (curStat & ~ledParams);
    }
#endif
}

void StatusLED::flash(uint8_t pattern)
{
#if defined (STATUSLED)
  show(pattern);
  curStat|=ledFlash;
#endif
}

void StatusLED::poll()
{
#if defined (STATUSLED)
  if (curStat & ledFlash)
    {
      curStat&=~ledFlash;
      show(curStat);
    }
//if (millis()>timestamp)
if (isTimeOver(timestamp,millis(),(curStat & ledFASTBLINK)?ledFastDelayms:ledDelayms))
  {
        timestamp=millis();
        //if (curStat & ledFASTBLINK) timestamp=millis()+ledFastDelayms;
        //                    else    timestamp=millis()+ledDelayms;

    if (( curStat & ledBLINK) || (curStat & ledFASTBLINK))
    {
    curStat^=ledHidden;
    if (curStat & ledHidden)
        show(0);
    else show(curStat);
   }
  }
#endif
}
