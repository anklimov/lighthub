
#include "abstractin.h"
#include "abstractch.h"
#if not defined (NOIP)   
#include <PubSubClient.h>
extern PubSubClient mqttClient;
#endif
#include "utils.h"
#include <aJSON.h>
#include "inputs.h"
#include "main.h"

extern lan_status lanStatus;


int abstractIn::publish(long value, const char* subtopic)
{
  char valstr[16];
  printUlongValueToStr(valstr, value);
  return publish(valstr,subtopic);
};

int abstractIn::publish(float value, const  char* subtopic)
{
  char valstr[16];
  printFloatValueToStr(valstr, value);
  return publish(valstr,subtopic);
};

int abstractIn::publish(char * value, const char* subtopic)
{
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
