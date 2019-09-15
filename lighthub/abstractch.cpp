
#include "abstractch.h"
#include <PubSubClient.h>
#include "utils.h"
#include <aJSON.h>
#include "main.h"

extern lan_status lanStatus;
extern PubSubClient mqttClient;

int abstractCh::publishTopic(const char* topic, long value, const char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publishTopic(topic, valstr,subtopic);
};

int abstractCh::publishTopic(const char* topic, float value, const char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(value, valstr);
  return publishTopic(topic, valstr,subtopic);
};

int abstractCh::publishTopic(const char* topic, const char * value, const char* subtopic)
{
  char addrstr[MQTT_TOPIC_LENGTH];

    if (topic)
     {
       strncpy(addrstr,topic,sizeof(addrstr));
       if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,topic);
       strncat(addrstr,subtopic,sizeof(addrstr));
       if (mqttClient.connected() && lanStatus == OPERATION)
                       {
                        mqttClient.publish(addrstr, value, true);
                        return 1;
                      }
     }

};
