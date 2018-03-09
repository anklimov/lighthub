#include <ESP8266WiFi.h> 
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266WiFiMulti.h>
 
extern ESP8266WiFiMulti wifiMulti;
extern WiFiClient ethClient;
//WiFiManager wifiManager; 

//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1 
extern char mqtt_password[16];

//default custom static IP
//char static_ip[16] = "10.0.1.56";
//char static_gw[16] = "10.0.1.1";
//char static_sn[16] = "255.255.255.0"; 

//flag for saving data
extern bool shouldSaveConfig;

void espSetup (); 
