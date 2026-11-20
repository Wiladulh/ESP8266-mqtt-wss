#include "WiFiManager.h"
#include "../config/ConfigManager.h"
#include <ESP8266WiFi.h>

static bool apMode=false,connecting=false;
static unsigned long attemptAt=0;
namespace WiFiManagerApp {
void begin(){
  const DeviceConfig& c=ConfigManager::get();
  WiFi.persistent(false);WiFi.setAutoReconnect(true);WiFi.mode(WIFI_STA);
  if(!ConfigManager::ready()){
    WiFi.disconnect();WiFi.mode(WIFI_AP);WiFi.softAP(c.apSsid.c_str(),c.apPassword.c_str());apMode=true;return;
  }
  WiFi.begin(c.wifiSsid.c_str(),c.wifiPassword.c_str());connecting=true;attemptAt=millis();
}
void loop(){
  if(apMode||!connecting)return;
  if(WiFi.status()==WL_CONNECTED){connecting=false;return;}
  if(millis()-attemptAt>20000){
    const DeviceConfig& c=ConfigManager::get();WiFi.disconnect();WiFi.mode(WIFI_AP);
    WiFi.softAP(c.apSsid.c_str(),c.apPassword.c_str());apMode=true;connecting=false;
  }
}
bool isApMode(){return apMode;}
}
