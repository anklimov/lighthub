
#include "abstractin.h"
#include "abstractch.h"
#include <PubSubClient.h>
#include "utils.h"
#include <aJSON.h>
#include "inputs.h"
#include "main.h"

extern lan_status lanStatus;
extern PubSubClient mqttClient;

int abstractIn::publish(long value, const char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publish(valstr,subtopic);
};

int abstractIn::publish(float value, const  char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(value, valstr);
  return publish(valstr,subtopic);
};

int abstractIn::publish(char * value, const char* subtopic)
{
  char addrstr[MQTT_TOPIC_LENGTH];
  if (in)
  {
  aJsonObject *emit = aJson.getObjectItem(in->inputObj, "emit");
    if (emit && emit->type == aJson_String)
     {
       return publishTopic(emit->valuestring,value,subtopic);
     }
  }
return 0;
};
