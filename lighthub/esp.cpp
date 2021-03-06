#include "options.h"
#ifdef __ESP__
#include "esp.h"

ESP8266WiFiMulti wifiMulti;
WiFiClient ethClient;

char mqtt_password[16];

//default custom static IP
//char static_ip[16] = "10.0.1.56";
//char static_gw[16] = "10.0.1.1";
//char static_sn[16] = "255.255.255.0"; 

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}


void espSetup () {
  Serial.println(F("Setting up Wifi"));
  shouldSaveConfig = true;
 //WiFiManager

  WiFiManagerParameter custom_mqtt_password("", "mqtt password", mqtt_password, 16);
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.setMinimumSignalQuality();
     
if (!wifiManager.autoConnect()) {
    Serial.println(F("failed to connect and hit timeout"));
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println(F("connected...yeey :)"));

  //read updated parameters
  strcpy(mqtt_password, custom_mqtt_password.getValue());

}
#endif
