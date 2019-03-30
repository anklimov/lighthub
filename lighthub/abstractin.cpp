
#include "abstractin.h"
#include <PubSubClient.h>
#include "utils.h"
#include <aJSON.h>
#include "inputs.h"

extern PubSubClient mqttClient;

int abstractIn::publish(int value, char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  publish(valstr,subtopic);
};

int abstractIn::publish(float value, char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(value, valstr);
  publish(valstr,subtopic);
};

int abstractIn::publish(char * value, char* subtopic)
{
  char addrstr[MQTT_TOPIC_LENGTH];
  aJsonObject *emit = aJson.getObjectItem(in->inputObj, "emit");
  strncpy(addrstr,emit->valuestring,sizeof(addrstr));
  if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
  strncat(addrstr,subtopic,sizeof(addrstr));
  if (mqttClient.connected()) mqttClient.publish(addrstr, value, true);
};
