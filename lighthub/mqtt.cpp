/*
int mqtt::publish(int value)
{};

int mqtt::publish(float value)
{

};

int mqtt::publish(char * value)
{
  char addrstr[MQTT_TOPIC_LENGTH];
  aJsonObject *emit = aJson.getObjectItem(in, "emit");
  strncpy(addrstr,emit->valuestring,sizeof(addrstr));
  if (!strchr(addrstr,'/')) setTopic(addrstr,sizeof(addrstr),T_OUT,emit->valuestring);
  if mqttClient.connected() mqttClient.publish(addrstr, value, true);
};
*/
