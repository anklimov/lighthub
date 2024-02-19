
#include "abstractch.h"
#if not defined (NOIP)   
#include <PubSubClient.h>
#endif
#include "utils.h"
#include <aJSON.h>
#include "main.h"

extern lan_status lanStatus;

#if not defined (NOIP)
extern PubSubClient mqttClient;
extern int8_t ethernetIdleCount;
#endif

int abstractCh::publishTopic(const char* topic, long value, const char* subtopic)
{ 
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publishTopic(topic, valstr,subtopic);
};

int abstractCh::publishTopic(const char* topic, float value, const char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(valstr, value);
  return publishTopic(topic, valstr,subtopic);
};

int abstractCh::publishTopic(const char* topic, const char * value, const char* subtopic)
{
  #if not defined (NOIP)
  char addrstr[MQTT_TOPIC_LENGTH];
 if (!isNotRetainingStatus()) return 0;
    if (topic)
     {
       strncpy(addrstr,topic,sizeof(addrstr));
       if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,topic);
       strncat(addrstr,subtopic,sizeof(addrstr)-1);
       if (mqttClient.connected() && lanStatus == OPERATION  && !ethernetIdleCount)
                       {
                        mqttClient.publish(addrstr, value, true);
                        return 1;
                      }
     }
 #endif    
return 0;
};
