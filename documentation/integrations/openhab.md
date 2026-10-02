# Интеграция с OpenHAB

> Источник: Wiki LightHub (lazyhome.ru), страница «openhab». Снапшот от 2026-10-02.

Initially, LightHub was developed taking in mind integration with OpenHAB. Thus that might be configured easy.

All integration done, using MQTT broker as extension of OpenHab's message bus.

First of all, you need to install MQTT binding via "Paper UI" and then configure broker connection for your OpenHab.

The following examples are for OpenHab 2.
The path of your configuration files is system-dependent.
For example, for UNIX installations from repository configuration path is /etc/openhab2/

File: services/mqtt.cfg

```
# URL to the MQTT broker, e.g. tcp://localhost:1883 or ssl://localhost:8883
mqtt.url=tcp://192.168.88.2:1883
# Optional. User id to authenticate with the broker.
mqtt.user=<user>
# Optional. Password to authenticate with the broker.
mqtt.pwd=<password>
```

then configure openhab message bus
File: services/mqtt-eventbus.cfg

```
# Name of the broker from mqtt.cfg 
broker=mqtt
# Publish command
commandPublishTopic=/myhome/in/${item}
# Receive status
stateSubscribeTopic=/myhome/s_out/${item}
```

after such kind of configuring, you can use same items name between OpenHab and LightHub
Example:
File items/demo.item (sor sitemap demo)

```
Dimmer  bth2    "Ванная люстра [%d %%]" <slider>        (gBath,Lights) ["Lighting"]
Color   kuh     "RGB Вся Кухня" <slider>        (gKitchen,Lights) ["Lighting"]
Switch  h_entr     "Corridor"      <heating>   (gEntr, Heating) ["Switchable"]
```

And appropriate configuration of LightHub items

```json
{
  "dmx":[3,60], //Init 60 channels dmx out on pin 3 (pin def for Mega only)
  "items": {
    "bth1":[4,[96,0,0]],
    "bth2":[4,[96,0,1]], // 4=Modbus dimmer on modbus address 96, Register 0, subchannel 1
    "kuh":[1,5],  // 1=RGBW DMX channel with base address 5
    "h_entr":[5,39,32] //5=thermostat with relay on pin 39 and default temperature preset = 32 degrees Celsium
   }
}
```

Note:
There is not necessary to set-up persistent topics in Openhab nor configure persistent storage in OpenHab.

LightHub using persistent MQTT publishing for Status topic /myhome/s_out. After "cold start" and establishing MQTT connection with broker, Lighthub subscribing on Status topic for 5 seconds interval to capture and retrieve previous status from persistent MQTT topic /myhome/s_out

## Сверка с кодом (2026-10-02)

- Страница не содержит специфических ключей конфигурации LightHub, кроме стандартных типов item; прямая сверка с кодом не требуется.
- Пример использует тип item 4 (`CH_MODBUS`, «Modbus AC Dimmer», item.h:52) — драйвер Modbus v1; определение в текущем коде присутствует. Для произвольных Modbus-устройств (драйвер v2) используется тип 14 (`CH_MBUS`, item.h:62) — см. [Настройка Modbus](modbus_setup.md).
- MQTT-префиксы `/myhome/in` и `/myhome/s_out` соответствуют значениям по умолчанию контроллера.
