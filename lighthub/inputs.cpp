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
#include "utils.h"
#include <PubSubClient.h>

#ifndef DHT_DISABLE
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <DHTesp.h>
#else
#include "DHT.h"
#endif
#endif

extern PubSubClient mqttClient;

#if !defined(DHT_DISABLE) || !defined(COUNTER_DISABLE)
static volatile unsigned long nextPollMillisValue[5];
static volatile int nextPollMillisPin[5] = {0,0,0,0,0};
#endif

#ifndef COUNTER_DISABLE
#if defined(ARDUINO_ARCH_AVR)
static volatile long counter_value[6];
#endif

#if defined(ARDUINO_ARCH_ESP8266)
static volatile long counter_value[6];
#endif

#if defined(ARDUINO_ARCH_ESP32)
static volatile long counter_value[6];
#endif

#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_STM32)
static short counter_irq_map[54];
    static long counter_value[54];
    static int counters_count;
#endif
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
        aJsonObject *itemBuffer;
        itemBuffer = aJson.getObjectItem(inputObj, "T");
        if (itemBuffer) inType = static_cast<uint8_t>(itemBuffer->valueint);
        pin = static_cast<uint8_t>(atoi(inputObj->name));
        itemBuffer = aJson.getObjectItem(inputObj, "S");
        if (!itemBuffer) {
            debugSerial<<F("In: ")<<pin<<F("/")<<inType<<endl;
            aJson.addNumberToObject(inputObj, "S", 0);
            itemBuffer = aJson.getObjectItem(inputObj, "S");
        }
        if (itemBuffer) store = (inStore *) &itemBuffer->valueint;
    }
}

int Input::poll() {
    if (!isValid()) return -1;
    if (0) ;

    #ifndef DHT_DISABLE
    else if (inType & IN_DHT22)
        dht22Poll();
    #endif
    #ifndef COUNTER_DISABLE
    else if (inType & IN_COUNTER)
        counterPoll();
    else if (inType & IN_UPTIME)
        uptimePoll();
    #endif
    else if (inType & IN_ANALOG)
        analogPoll();
    else
        contactPoll();
    return 0;

  //  contactPoll();
}

#ifndef COUNTER_DISABLE
void Input::counterPoll() {
    if(nextPollTime()>millis())
        return;
    if (store->logicState == 0) {
#if defined(ARDUINO_ARCH_AVR)
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
    debugSerial<<F("IN:")<<(pin)<<F(" Counter type. val=")<<counterValue;

    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    if (emit) {
        char valstr[10];
        char addrstr[MQTT_TOPIC_LENGTH];
        strncpy(addrstr,emit->valuestring,sizeof(addrstr));
        if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
        sprintf(valstr, "%d", counterValue);
        mqttClient.publish(addrstr, valstr);
        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT);
        debugSerial<<F(" NextPollMillis=")<<nextPollTime();
    }
    else
        debugSerial<<F(" No emit data!");
}
#endif

#ifndef COUNTER_DISABLE
void Input::attachInterruptPinIrq(int realPin, int irq) {
    pinMode(realPin, INPUT);
    int real_irq;
#if defined(ARDUINO_ARCH_AVR)
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





void Input::uptimePoll() {
    if (nextPollTime() > millis())
        return;
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    if (emit) {
#ifdef WITH_DOMOTICZ
        if(getIdxField()){
                publishDataToDomoticz(DHT_POLL_DELAY_DEFAULT, emit, "{\"idx\":%s,\"svalue\":\"%d\"}", getIdxField(), millis());
                return;
            }
#endif

        char valstr[11];
//        printUlongValueToStr(valstr,millis());
        printUlongValueToStr(valstr, millis());
        mqttClient.publish(emit->valuestring, valstr);
    }
    setNextPollTime(millis() + UPTIME_POLL_DELAY_DEFAULT);
}

void Input::onCounterChanged(int i) {
#if defined(__SAM3X8E__)
    counter_value[counter_irq_map[i]]++;
#endif

#if defined(ARDUINO_ARCH_AVR)
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

#endif

#if !defined(DHT_DISABLE) || !defined(COUNTER_DISABLE)
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
#endif

#ifndef DHT_DISABLE

void Input::dht22Poll() {
    if (nextPollTime() > millis())
        return;
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    DHTesp dhtSensor;
    dhtSensor.setup(pin, DHTesp::DHT22);
    TempAndHumidity dhtSensorData = dhtSensor.getTempAndHumidity();
    float temp = roundf(dhtSensorData.temperature * 10) / 10;
    float humidity = roundf(dhtSensorData.humidity);
#else
    DHT dht(pin, DHT22);
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
#endif
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    aJsonObject *item = aJson.getObjectItem(inputObj, "item");
    if (item) thermoSetCurTemp(item->valuestring, temp);
    debugSerial << F("IN:") << pin << F(" DHT22 type. T=") << temp << F("°C H=") << humidity << F("%")<<endl;
    if (emit && temp && humidity && temp == temp && humidity == humidity) {
        char addrstr[MQTT_TOPIC_LENGTH] = "";
#ifdef WITH_DOMOTICZ
        if(getIdxField()){
            publishDataToDomoticz(DHT_POLL_DELAY_DEFAULT, emit, "{\"idx\":%s,\"svalue\":\"%.1f;%.0f;0\"}", getIdxField(), temp, humidity);
            return;
        }
#endif
        char valstr[10];

        strncpy(addrstr, emit->valuestring, sizeof(addrstr));
        if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
        strcat(addrstr, "T");
        printFloatValueToStr(temp, valstr);
        mqttClient.publish(addrstr, valstr);
        addrstr[strlen(addrstr) - 1] = 'H';
        printFloatValueToStr(humidity, valstr);
        mqttClient.publish(addrstr, valstr);

        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT);
        debugSerial << F(" NextPollMillis=") << nextPollTime() << endl;
    } else
        setNextPollTime(millis() + DHT_POLL_DELAY_DEFAULT / 3);
}
#endif

void Input::contactPoll() {
    boolean currentInputState;
/*
#if defined(ARDUINO_ARCH_STM32)
     WiringPinMode inputPinMode;
#endif

#if defined(__SAM3X8E__)||defined(ARDUINO_ARCH_AVR)||defined(ARDUINO_ARCH_ESP8266)||defined(ARDUINO_ARCH_ESP32)

#endif
*/
     uint32_t inputPinMode;
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
            if (inType & IN_PUSH_TOGGLE) {
                if (currentInputState) { //react on leading edge only (change from 0 to 1)
                    store->logicState = !store->logicState;
                    onContactChanged(store->logicState);
                }
            } else {
                store->logicState = currentInputState;
                onContactChanged(currentInputState);
            }
            store->currentValue = currentInputState;
        }
    } else // no change
        store->bounce = SAME_STATE_ATTEMPTS;
}


void Input::analogPoll() {
    int16_t  mappedInputVal;
    aJsonObject *inputMap = aJson.getObjectItem(inputObj, "map");
    short Noize = ANALOG_NOIZE;
    short simple = 0;

/*
#if defined(ARDUINO_ARCH_STM32)
     WiringPinMode inputPinMode;
#endif


#if defined(__SAM3X8E__)||defined(ARDUINO_ARCH_AVR)||defined(ARDUINO_ARCH_ESP8266)||defined(ARDUINO_ARCH_ESP32)
#endif */
    uint32_t inputPinMode;
    if (inType & IN_ACTIVE_HIGH) {
        inputPinMode = INPUT;
    } else {
        inputPinMode = INPUT_PULLUP;
    }
    pinMode(pin, inputPinMode);
    mappedInputVal = analogRead(pin);
    // Mapping
    if (inputMap && inputMap->type == aJson_Array)
     {
     int max;
     if (aJson.getArraySize(inputMap)>=4)
        mappedInputVal  = map (mappedInputVal,
              aJson.getArrayItem(inputMap, 0)->valueint,
              aJson.getArrayItem(inputMap, 1)->valueint,
              aJson.getArrayItem(inputMap, 2)->valueint,
              max=aJson.getArrayItem(inputMap, 3)->valueint);
      if (aJson.getArraySize(inputMap)==5) Noize = aJson.getArrayItem(inputMap, 4)->valueint;
      if (mappedInputVal>max) mappedInputVal=max;
      if (aJson.getArraySize(inputMap)==2)
        {
          simple = 1;
          if (mappedInputVal < aJson.getArrayItem(inputMap, 0)->valueint) mappedInputVal = 0;
            else if (mappedInputVal > aJson.getArrayItem(inputMap, 1)->valueint) mappedInputVal = 1;
                 else return;
        }
      }
    if (simple) {
       if (mappedInputVal != store->currentValue)
       {
           onContactChanged(mappedInputVal);
           store->currentValue = mappedInputVal;
       }}
    else
    if (abs(mappedInputVal - store->currentValue)>Noize) // value changed >ANALOG_NOIZE
        store->bounce = 0;
     else // no change
        if (store->bounce<ANALOG_STATE_ATTEMPTS) store->bounce ++;

        if (store->bounce<ANALOG_STATE_ATTEMPTS-1 && (mappedInputVal != store->currentValue))  //confirmed change
        {
            onAnalogChanged(mappedInputVal);
            store->currentValue = mappedInputVal;
        }

}




void Input::onContactChanged(int newValue) {
    debugSerial << F("IN:") << (pin) << F("=") << newValue << endl;
    aJsonObject *item = aJson.getObjectItem(inputObj, "item");
    aJsonObject *scmd = aJson.getObjectItem(inputObj, "scmd");
    aJsonObject *rcmd = aJson.getObjectItem(inputObj, "rcmd");
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");
    if (emit) {
#ifdef WITH_DOMOTICZ
        if (getIdxField()) {
            (newValue)? publishDataToDomoticz(0, emit, "{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"On\"}", getIdxField())
            : publishDataToDomoticz(0,emit,"{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"Off\"}",getIdxField());
        } else
#endif
char addrstr[MQTT_TOPIC_LENGTH];
strncpy(addrstr,emit->valuestring,sizeof(addrstr));
if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
        if (newValue) {  //send set command
            if (!scmd) mqttClient.publish(addrstr, "ON", true);
            else if (strlen(scmd->valuestring))
                mqttClient.publish(addrstr, scmd->valuestring, true);
        } else {  //send reset command
            if (!rcmd) mqttClient.publish(addrstr, "OFF", true);
            else if (strlen(rcmd->valuestring))mqttClient.publish(addrstr, rcmd->valuestring, true);
        }
    }

    if (item) {
        Item it(item->valuestring);
        if (it.isValid()) {
            if (newValue) {  //send set command
                if (!scmd) it.Ctrl(CMD_ON, 0, NULL, true);
                else if (strlen(scmd->valuestring))
                    it.Ctrl(scmd->valuestring, true);
            } else {  //send reset command
                if (!rcmd) it.Ctrl(CMD_OFF, 0, NULL, true);
                else if (strlen(rcmd->valuestring))
                    it.Ctrl(rcmd->valuestring, true);
            }
        }
    }
}


void Input::onAnalogChanged(int newValue) {
    debugSerial << F("IN:") << (pin) << F("=") << newValue << endl;
    aJsonObject *item = aJson.getObjectItem(inputObj, "item");
    aJsonObject *emit = aJson.getObjectItem(inputObj, "emit");


    if (emit) {

//#ifdef WITH_DOMOTICZ
//        if (getIdxField()) {
//            (newValue)? publishDataToDomoticz(0, emit, "{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"On\"}", getIdxField())
//            : publishDataToDomoticz(0,emit,"{\"command\":\"switchlight\",\"idx\":%s,\"switchcmd\":\"Off\"}",getIdxField());
//        } else
//#endif
              char addrstr[MQTT_TOPIC_LENGTH];
              strncpy(addrstr,emit->valuestring,sizeof(addrstr));
              if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
              char strVal[16];
              itoa(newValue,strVal,10);
              mqttClient.publish(addrstr, strVal, true);
}

    if (item) {
        Item it(item->valuestring);
        if (it.isValid()) {
           it.Ctrl(0, 1, &newValue, true);
        }
    }
}


bool Input::publishDataToDomoticz(int pollTimeIncrement, aJsonObject *emit, const char *format, ...)
{
#ifdef WITH_DOMOTICZ
    debugSerial << F("\nDomoticz valstr:");
    char valstr[50];
    va_list args;
    va_start(args, format);
    vsnprintf(valstr, sizeof(valstr) - 1, format, args);
    va_end(args);
    debugSerial << valstr;
    mqttClient.publish(emit->valuestring, valstr);
    if (pollTimeIncrement)
        setNextPollTime(millis() + pollTimeIncrement);
    debugSerial << F(" NextPollMillis=") << nextPollTime() << endl;

#endif
    return true;
}

char* Input::getIdxField() {
    aJsonObject *idx = aJson.getObjectItem(inputObj, "idx");
    if(idx&&idx->valuestring)
        return idx->valuestring;
    return nullptr;
}
