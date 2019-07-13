
#include "abstractch.h"
#include <PubSubClient.h>
#include "utils.h"
#include <aJSON.h>
#include "main.h"

extern lan_status lanStatus;
extern PubSubClient mqttClient;

int abstractCh::publish(char* topic, long value, char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publish(topic, valstr,subtopic);
};

int abstractCh::publish(char* topic, float value, char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(value, valstr);
  return publish(topic, valstr,subtopic);
};

int abstractCh::publish(char* topic, char * value, char* subtopic)
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
