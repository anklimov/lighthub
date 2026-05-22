# LightHub: Модуль многоканальной системы полива (out_sprinkler)
Данный модуль реализует многозональную систему полива

## Система состоит из следующих компонент:

* Накопительный водяной бак. Снабжен двумя поплавками. Максимум воды и минимум воды. Заведены на входы wMax и wMin 
* Насос полива высокого давления. Запитан из бака. Включается реле, подключенным к выходу rPump. Датчик тока для контроля того, что насос включен, заведен на вход fbPump
* Набор клапанов зон полива. Подключены через оптореле к выходам, заданным в параметре pin соответствующей зоны полива.
* Клапан налива из водопровода. Подключен через оптореле к выходу vIn
* Насос дренажного колодца. Приоритетный источник для наполнения бака полива. Когда система полива находится в ждущем или активном режиме, бак пытается максимально наполнится из дренажного колодца. Насос дренажа имеет поплавковый выключатель, отключающий насос при осушении дренажного колодца. Насос включается реле, которое подключено к выходу rDren. Для контроля того, что насос включен и момента осушения колодца, используется датчик тока, который подключен в входу fbDren
* Опциональный водосчетчик. Контакты подключены к входу wCtr

## Система налива воды реализована при помощи конечного автомата со следующими состояниями:

* SP\_UNKNOWN
* SP\_OFF
* SP\_DREN\_ON - дренажный насос включен
* SP\_DREN\_OPERATE - дренажный насос работает
* SP\_DREN\_EMPTY - дренажный насос выключился встроенным поплавком - колодец пуст
* SP\_VIN - включено наполнение из водопровода
* SP\_FULL - бак наполнен

граф переходов конечного автомата системы налива воды

Состояние SP\_* | Условие перехода | перейти в состояние | выполнить при переходе |
|------|--------|-------|---------|
INIT			|  		true				| OFF                  | выключить клапана и насосы |
OFF          | vMax && !FREEZE             | FULL                 | выключить vIN, rDren|
OFF          | ! vMax && !FREEZE    | DREN\_ON|включить rDren|
DREN\_ON|fbDren (насос дренажа реально работает)|DREN\_OPERATE|
DREN\_ON|таймаут 10 сек|DREN\_EMPTY| насос так и не заработал - видимо в колодце пусто
DREN\_EMPTY|включен цикл полива и бак не полон|VIN|включить клапан vIN для набора бака из водопровода|
DREN\_OPERATE| ! fbDren (насос дренажа более не работает)|DREN\_EMPTY||
VIN|fbDren|DREN\_OPERATE|вылючить клапан vIN для набора бака из водопровода - заработал дренажный насос|
VIN, DREN\_OPERATE|vMax|FULL                 | выключить vIN, rDren  - бак полон|
VIN|таймаут 1200 сек	| FAULT\_VIN| выключить vIN - бак так и не наполнился|
DREN\_OPERATE|таймаут 2000 сек	|FAULT_DREN|выключить rDren  - не смотря на продолжительную работу насоса бак не наполнен|



## Конфигурирование:


```

"items":
{
         "sprinkler":[23,
   {
   "":{
			"vIn":3,
			"wMax":44,
			"wMin":46,
			"rDren":23,
			"fbDren":63,
			"rPump":24,	
			"fbPump":62,
			"wCtr":49	
   		},
   "nord":{"pin":6,"set":60,"cmd":1},
   "south":{"pin":7,"set":100,"cmd":1},
   "trees":{"pin":10,"set":60,"cmd":2},
   "outlets:{} 

	}]
}

```

## Алгоритм работы
в настройки зон полива задаем интенсивность для каждой зоны.

Это можно сделать как в конфиге так динамически, (стандартными механизмами управления по MQTT, HTTP, CAN)

Рассмотрим на примере MQTT:

**топик** ```root/name/sprinkler/garden/set -> 60```

Задаем обьем полива 60 отсчетов счетчика воды (если счетчик не сконфигурирован - 60 секунд)

Контроллер должен передать это значение в выходной топик ```root/name/s_out/sprinkrer/garden/set``` и оно будет восстановлено при перезагрузке контроллера


отработанный обьем воды или время будет сохраняться в параметре "val" каждой зоны (параметр будет автоматически увеличиваться при работе зоны, передаваться в соответствующий зоне топик для мониторинга и восстановления в случае перезагрузки контроллера)

**Пример топика:** ```root/s_out/sprinkler/garden/val```

Когда данный параметр достигнет значения (или времени), заданного в параметре "set" контроллер завершит полив данной зоны и перейдет к следующей.

Важно: если set=0 (по умолчанию) то время работы зоны не лимитируется. Если такая зона включена - система полива не будет отключаться после окончания полива прочих зон и насос не будет обесточиваться. Это удобно, если в системе полива есть водяная розетка для подключения поливочного шланга, которая всегда должна находиться под давлением. Такую зону конфигурируйте последней в списке.

Для сброса счетчиков можно использовать как непосредственную установку значения параметра "val" для каждой зоны так и команду RESET, отправленную в нужную зону или в объект sprinkler через суффикс /cmd. 

В последнем случае, контроллер итерационно сбросит счетчики в значение 0 для каждой зоны полива.
А также, отключит систему полива, чтобы программа не стартовала в момент сброса счетчиков (например, в полночь)

**Пример:** ```root/name/sprinkler/cmd -> RESET```


## Управление

### Включение/выключение  полива конкретной зоны:

**Включить** ```root/name/sprinkler/garden/cmd -> ON```

**Выключить** ```root/name/sprinkler/garden/cmd -> OFF```



### Включение/выключение  цикла полива:

**Включить** ```root/name/sprinkler/cmd -> ON```
Система начнет или продолжит цикл полива, переходя от зоны к зоне по мере завершения работы с каждой предыдущей зоной. После завершения работы со всеми зонами, sprinkler перейдет в состояние OFF

Перед включением полива, система убедится что бак наполнен или до-наполнит его до максимума из водопровода.



**Выключить** ```root/name/sprinkler/cmd -> OFF```
Система немедленно остановит текущий цикл полива (закроет клапаны зон, выключит насос полива)


Аналогично, будут работать команды XON и XOFF, с одним исключением, что команда XON может быть запрещена и игнорироваться если активирован режим DISABLE. Это базовая функция контроллера и не относится к функционалу данного модуля. Но может быть использована, например, для запрета полива на определенное время после выпадения осадков

**Пример** 

```
root/name/sprinkler/ctrl -> DISABLE
root/name/sprinkler/cmd -> XON  //Будет проигнорировано

root/name/sprinkler/ctrl -> ENABLE
root/name/sprinkler/cmd -> XON  //А вот теперь сработает

```

Даже в выключенном состоянии (OFF) , система полива работает в дежурном режиме, поддерживая максимальный уровень воды в баке за счет немедленной перекачки из дренажного колодца

При попытке включения системы после завершения дневного задания по поливу всех зон (параметр val для всех зон достиг параметра set), система сразу перейдет в состояние OFF


 
### Полная блокировка системы полива (в зимнее время)
Ддя перевода канала полива в полностью заблокированное состояние и обратно  импользуется системная команда FREEZE/UNFREEZE соответственно

В режиме FREEZE полностью заблокирована обработка всех команд, кроме UNFREEZE, заблокирован автомат пополнения бака из дренажного насоса и выключены насосы и все клапана

Рекомендуется задать флаг FREEZE в конфигурации канала (см документ ...) , чтобы избежать разблокировки при утере значений топика /clrl и перезагрузки системы

Также, на вход /val обЪекта sprinkler можно подать значение уличной  температуры. И если значения будут ниже нуля, система автоматически перейдет в режим FREEZE	

**Пример** 

```
root/name/sprinkler/ctrl -> FREEZE
root/name/sprinkrer/cmd -> ON  //Будет проигнорировано

root/name/sprinkler/ctrl -> UNFREEZE
root/name/sprinkrer/cmd -> ON  //А вот теперь сработает

root/name/sprinkler/val -> -1 //система перейдет в режим FREEZE


```

### Передача статусных значений


**Примеры выдачи в топики:** 

```
root/s_out/sprinkler/$fbPump - ON/OFF признак того, что включен основной насос (от датчика тока)
root/s_out/sprinkler/$fbDren - ON/OFF признак того, что включен дренажный насос (от датчика тока)
root/s_out/sprinkler/$state - состояние конечного автомата Системы налива воды
root/s_out/sprinkler/$wMax - ON/OFF достигнут максимум воды в баке (от поплавкового датчика)
root/s_out/sprinkler/$wMin - ON/OFF достигнут минимум воды в баке (от поплавкового датчика)
root/s_out/sprinkler/$rDren - ON/OFF включено реле дренажного насоса
root/s_out/sprinkler/$rPump - ON/OFF включено реле основного насоса
root/s_out/sprinkler/set - значение счетчика воды (восстанавливается при перезагрузке из данного топика)
root/s_out/sprinkler/$vIN - ON/OFF - признак открытия клапана налива бака из водопровода

root/s_out/sprinkler/garden/set - требуемый обьем (или время) полива зоны
root/s_out/sprinkler/garden/cmd - ON или OFF - признак включения полива зоны 
root/s_out/sprinkler/garden/$state - ON или OFF - признак того что зона поливается в настоящее время
root/s_out/sprinkler/garden/val - текущее время или обьем полива данной зоны

```

### Пример конфигурации Home Assistant
```
sensor:
  - name: "Полив бак Макс"
    state_topic:  "root/s_out/sprinkler/$wMax"
    
  - name: "Полив бак Мin"
    state_topic:  "root/s_out/sprinkler/$wMin"    
    
  - name: "Полив водопровод"
    state_topic:  "root/s_out/sprinkler/$vIN"    
    
  - name: "Полив дренаж вкл"
    state_topic:  "root/s_out/sprinkler/$rDren"    

  - name: "Полив дренаж качает"
    state_topic:  "root/s_out/sprinkler/$fbDren"    
    
  - name: "Полив насос вкл"
    state_topic:  "root/s_out/sprinkler/$rPump"    

  - name: "Полив насос качает"
    state_topic:  "root/s_out/sprinkler/$fbPump"       
    
  - name: "Полив состояние"
    state_topic:  "root/s_out/sprinkler/$state"  
    
  - name: "Полив ошибка"
    state_topic:  "root/s_out/sprinkler/$fault"
    
  - name: "Полив юг выполнено"
    state_topic:  "root/s_out/sprinkler/south/val"   

  - name: "Полив север выполнено"
    state_topic:  "root/s_out/sprinkler/nord/val"   

  - name: "Полив капельный выполнено"
    state_topic:  "root/s_out/sprinkler/trees/val"   
   
  - name: "Полив блокировки"
    state_topic:  "root/s_out/sprinkler/ctrl"   
    
switch:

  - name: "Полив"
    state_topic:  "root/s_out/sprinkler/cmd"
    command_topic: "root/air/sprinkler/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 

  - name: "Полив север"
    state_topic:  "root/s_out/sprinkler/nord/cmd"
    command_topic: "root/air/sprinkler/nord/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 

  - name: "Полив юг"
    state_topic:  "root/s_out/sprinkler/south/cmd"
    command_topic: "root/air/sprinkler/south/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 

  - name: "Полив капельный"
    state_topic:  "root/s_out/sprinkler/trees/cmd"
    command_topic: "root/air/sprinkler/trees/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 

  - name: "Полив розетки"
    state_topic:  "root/s_out/sprinkler/outlets/cmd"
    command_topic: "root/air/sprinkler/outlets/cmd"
    availability_topic: "root/air/$state"
    payload_available: "ready"
    payload_not_available: "disconnected" 
    
button:
 - name: "Полив сброс"
   command_topic: "root/air/sprinkler/cmd"
   payload_press: "RESET"
   
 - name: "Полив блокировка"
   command_topic: "root/air/sprinkler/cmd"
   payload_press: "FREEZE"

 - name: "Полив разблокировка"
   command_topic: "root/air/sprinkler/cmd"
   payload_press: "UNFREEZE"   
   
 - name: "Полив разрешить"
   command_topic: "root/air/sprinkler/cmd"
   payload_press: "ENABLE"

 - name: "Полив запретить"
   command_topic: "root/air/sprinkler/cmd"
   payload_press: "DISABLE"      

number:

 - name: "Полив юг"
   state_topic:  "root/s_out/sprinkler/south/set"   
   command_topic: "root/air/sprinkler/south/set"   
   min: 0
   max: 6000
   
 - name: "Полив север"
   state_topic:  "root/s_out/sprinkler/nord/set"   
   command_topic: "root/air/sprinkler/nord/set"   
   min: 0
   max: 6000   
   
 - name: "Полив капельный"
   state_topic:  "root/s_out/sprinkler/trees/set"   
   command_topic: "root/air/sprinkler/trees/set"   
   min: 0
   max: 6000
   
          

```