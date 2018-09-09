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

#include "inputs.h"
#include "item.h"
#include <PubSubClient.h>

#ifndef DHT_DISABLE
#if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <DHTesp.h>
#else
#include "DHT.h"
#endif
#endif

extern PubSubClient mqttClient;

static volatile unsigned long nextPollMillisValue[5];
static volatile int nextPollMillisPin[5] = {0,0,0,0,0};

#if defined(__AVR__)
static volatile long counter_value[6];
#endif

#if defined(ESP8266)
static volatile long counter_value[6];
#endif

#if defined(ARDUINO_ARCH_ESP32)
static volatile long counter_value[6];
#endif

#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32F1)
static short counter_irq_map[54];
    static long counter_value[54];
    static int counters_count;
#endif
Input::Input(char * name) //Constructor
{
  if (name)
       inputObj= aJson.getObjectItem(inputs, name);
  else inputObj=NULL;

     Parse();
}


Input::Input(int pin)
{
 // TODO
}


 Input::Input(aJsonObject * obj)
{
  inputObj= obj;
  Parse();

}


boolean Input::isValid ()
{
 return  (pin && store);
}

void Input::Parse()
{
    store = NULL;
    inType = 0;
    pin = 0;

    if (inputObj && (inputObj->type == aJson_Object)) {
        aJsonObject *s;

        s = aJson.getObjectItem(inputObj, "T");
        if (s) inType = static_cast<uint8_t>(s->valueint);

        pin = static_cast<uint8_t>(atoi(inputObj->name));

        s = aJson.getObjectItem(inputObj, "S");
        if (!s) {
            Serial.print(F("In: "));
            Serial.print(pin);
            Serial.print(F("/"));
            Serial.println(inType);
            aJson.addNumberToObject(inputObj, "S", 0);
            s = aJson.getObjectItem(inputObj, "S");
        }

        if (s) store = (inStore *) &s->valueint;
    }
}

int Input::poll() {
    if (!isValid()) return -1;
    if (inType & IN_DHT22)
        dht22Poll();
    else if (inType & IN_COUNTER)
        counterPoll();
    else if (inType & IN_UPTIME)
        uptimePoll();
    else
        contactPoll();
    return 0;
}

void Input::counterPoll() {
    if(nextPollTime()>millis())
        return;
    if (store->logicState == 0) {
#if defined(__AVR__)
#define interrupt_number pin
        if (interrupt_number >= 0 && interrupt_number < 6) {
            const short mega_interrupt_array[6] = {2, 3, 21, 20, 19, 18};
            short real_pin = mega_interrupt_array[interrupt_number];
            attachInterruptPinIrq(real_pin,interrupt_number);
        } else {
            Serial.print(F("IRQ:"));
            Serial.print(pin);
            Serial.print(F(" Counter type. INCORRECT Interrupt number!!!"));
            return;
        }
#endif

#if defined(__SAM3X8E__)
        attachInterruptPinIrq(pin,counters_count);
        counter_irq_map[counters_count]=pin;
        counters_count++;
#endif
        store->logicState = 1;
        return;
    }
    long counterValue = counter_value[pin];
    Serial.print(F("IN:"));Serial.print(pin);Serial.print(F(" Counter type. val="));Serial.print(counterValue);

    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    if (emit) {
        char valstr[10];
        char addrstr[100] = "";
        strcat(addrstr, emit->valuestring);
        sprintf(valstr, "%d", counterValue);
        mqttClient.publish(addrstr, valstr);
        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT);
        Serial.print(F(" NextPollMillis="));Serial.println(nextPollTime());
    }
    else
        Serial.print(F(" No emit data!"));
}

void Input::attachInterruptPinIrq(int realPin, int irq) {
    pinMode(realPin, INPUT);
    int real_irq;
#if defined(__AVR__)
    real_irq = irq;
#endif
#if defined(__SAM3X8E__)
    real_irq = realPin;
#endif
    switch(irq){
            case 0:
                attachInterrupt(real_irq, onCounterChanged0, RISING);
                break;
            case 1:
                attachInterrupt(real_irq, onCounterChanged1, RISING);
                break;
            case 2:
                attachInterrupt(real_irq, onCounterChanged2, RISING);
                break;
            case 3:
                attachInterrupt(real_irq, onCounterChanged3, RISING);
                break;
            case 4:
                attachInterrupt(real_irq, onCounterChanged4, RISING);
                break;
            case 5:
                attachInterrupt(real_irq, onCounterChanged5, RISING);
                break;
        default:
            Serial.print(F("Incorrect irq:"));Serial.println(irq);
            break;
        }
}

void Input::dht22Poll() {
#ifndef DHT_DISABLE
    if(nextPollTime()>millis())
        return;
#if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
    DHTesp dhtSensor;
    dhtSensor.setup(pin, DHTesp::DHT22);
    TempAndHumidity dhtSensorData = dhtSensor.getTempAndHumidity();
    float temp = dhtSensorData.temperature;
    float humidity = dhtSensorData.humidity;
#else
    DHT dht(pin, DHT22);
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
#endif
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    Serial.print(F("IN:"));
    Serial.print(pin);
    Serial.print(F(" DHT22 type. T="));
    Serial.print(temp);
    Serial.print(F("°C H="));
    Serial.print(humidity);
    Serial.print(F("%"));
    if (emit && temp && humidity && temp == temp && humidity == humidity) {
        char valstr[10];
        char addrstr[100] = "";
        strcat(addrstr, emit->valuestring);
        strcat(addrstr, "T");
        printFloatValueToStr(temp, valstr);
        mqttClient.publish(addrstr, valstr);
        addrstr[strlen(addrstr) - 1] = 'H';
        printFloatValueToStr(humidity, valstr);
        mqttClient.publish(addrstr, valstr);
        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT);
        Serial.print(" NextPollMillis=");
        Serial.println(nextPollTime());
    } else
        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT / 3);
#endif
}

unsigned long Input::nextPollTime() const {
    for(int i=0;i<5;i++){
        if(nextPollMillisPin[i]==pin)
            return nextPollMillisValue[i];
        else if(nextPollMillisPin[i]==0) {
            nextPollMillisPin[i]=pin;
            return nextPollMillisValue[i] = 0;
        }
    }
    return 0;
}

void Input::setNextPollTime(unsigned long pollTime) {
    for (int i = 0; i < 5; i++) {
        if (nextPollMillisPin[i] == pin) {
            nextPollMillisValue[i] = pollTime;
            return;
        } else if (nextPollMillisPin[i] == 0) {
            nextPollMillisPin[i] == pin;
            nextPollMillisValue[i] = pollTime;
            return;
        }
    }
}

void Input::printFloatValueToStr(float value, char *valstr) {
    #if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
    sprintf(valstr, "%2.1f", value);
    #endif
    #if defined(__AVR__)
    sprintf(valstr, "%d", (int)value);
    int fractional = 10.0*((float)abs(value)-(float)abs((int)value));
    int val_len =strlen(valstr);
    valstr[val_len]='.';
    valstr[val_len+1]='0'+fractional;
    valstr[val_len+2]='\0';
    #endif
    #if defined(__SAM3X8E__)
    sprintf(valstr, "%2.1f",value);
    #endif
}

void Input::contactPoll() {
    boolean currentInputState;
#if defined(ARDUINO_ARCH_STM32F1)
     WiringPinMode inputPinMode;
#endif
#if defined(__SAM3X8E__)||defined(__AVR__)||defined(ESP8266)||defined(ARDUINO_ARCH_ESP32)
     uint32_t inputPinMode;
#endif

     uint8_t inputOnLevel;
    if (inType & IN_ACTIVE_HIGH) {
        inputOnLevel = HIGH;
        inputPinMode = INPUT;
    } else {
        inputOnLevel = LOW;
        inputPinMode = INPUT_PULLUP;
    }
    pinMode(pin, inputPinMode);
    currentInputState = (digitalRead(pin) == inputOnLevel);
    if (currentInputState != store->currentValue) // value changed
    {
        if (store->bounce) store->bounce = store->bounce - 1;
        else //confirmed change
        {
            onContactChanged(currentInputState);
            store->currentValue = currentInputState;
        }
    } else // no change
        store->bounce = SAME_STATE_ATTEMPTS;
}

void Input::uptimePoll() {
    if(nextPollTime()>millis())
        return;
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    if (emit) {
        char valstr[11];
//        printUlongValueToStr(valstr,millis());
        printUlongValueToStr(valstr,millis());
        mqttClient.publish(emit->valuestring, valstr);
    }
    setNextPollTime(millis() +UPTIME_POLL_DELAY_DEFAULT);
}

void Input::onContactChanged(int val)
{
  Serial.print(F("IN:"));  Serial.print(pin);Serial.print(F("="));Serial.println(val);
  aJsonObject * item = aJson.getObjectItem(inputObj,"item");
  aJsonObject * scmd = aJson.getObjectItem(inputObj,"scmd");
  aJsonObject * rcmd = aJson.getObjectItem(inputObj,"rcmd");
  aJsonObject * emit = aJson.getObjectItem(inputObj,"emit");

  if (emit)
  {

  if (val)
            {  //send set command
               if (!scmd) mqttClient.publish(emit->valuestring,"ON",true); else  if (strlen(scmd->valuestring)) mqttClient.publish(emit->valuestring,scmd->valuestring,true);
            }
       else
            {  //send reset command
              if (!rcmd) mqttClient.publish(emit->valuestring,"OFF",true);  else  if (strlen(rcmd->valuestring)) mqttClient.publish(emit->valuestring,rcmd->valuestring,true);
            }
  }

  if (item)
  {
  Item it(item->valuestring);
  if (it.isValid())
      {
       if (val)
            {  //send set command
               if (!scmd) it.Ctrl(CMD_ON,0,NULL,true); else if   (strlen(scmd->valuestring))  it.Ctrl(scmd->valuestring,true);
            }
       else
            {  //send reset command
               if (!rcmd) it.Ctrl(CMD_OFF,0,NULL,true); else if  (strlen(rcmd->valuestring))  it.Ctrl(rcmd->valuestring,true);
            }
      }
  }
}

void Input::onCounterChanged(int i) {
#if defined(__SAM3X8E__)
    counter_value[counter_irq_map[i]]++;
#endif

#if defined(__AVR__)
    counter_value[i]++;
#endif
}

void Input::onCounterChanged0() {
    onCounterChanged(0);
}
void Input::onCounterChanged1() {
    onCounterChanged(1);
}
void Input::onCounterChanged2() {
    onCounterChanged(2);
}
void Input::onCounterChanged3() {
    onCounterChanged(3);
}
void Input::onCounterChanged4() {
    onCounterChanged(4);
}
void Input::onCounterChanged5() {
    onCounterChanged(5);
}

void Input::printUlongValueToStr(char *valstr, unsigned long value) {
    char buf[11];
    int i=0;
    for(;value>0;i++){
        unsigned long mod = value - ((unsigned long)(value/10))*10;
        buf[i]=mod+48;
        value = (unsigned long)(value/10);
    }

    for(int n=0;n<=i;n++){
        valstr[n]=buf[i-n-1];
    }
    valstr[i]='\0';
}

