
#include "abstractin.h"
#include "abstractch.h"
#include <PubSubClient.h>
#include "utils.h"
#include <aJSON.h>
#include "inputs.h"
#include "main.h"

extern lan_status lanStatus;
extern PubSubClient mqttClient;

int abstractIn::publish(long value, char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publish(valstr,subtopic);
};

int abstractIn::publish(float value, char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(value, valstr);
  return publish(valstr,subtopic);
};

int abstractIn::publish(char * value, char* subtopic)
{
  char addrstr[MQTT_TOPIC_LENGTH];
  if (in)
  {
  aJsonObject *emit = aJson.getObjectItem(in->inputObj, "emit");
    if (emit)
     {
       return publishTopic(emit->valuestring,value,subtopic);
     }
  }
return 0;
};
