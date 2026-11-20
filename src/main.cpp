#include <Arduino.h>
#include "config/ConfigManager.h"
#include "io/IOManager.h"
#include "network/WiFiManager.h"
#include "network/WebServerManager.h"
#include "mqtt/MqttManager.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("ESP8266 Modular MQTT WSS"));
  ConfigManager::begin();
  IOManager::begin();
  WiFiManagerApp::begin();
  WebServerManager::begin();
  MqttManager::begin();
}
void loop() {
  WiFiManagerApp::loop();
  WebServerManager::loop();
  IOManager::loop();
  MqttManager::loop();
  yield();
}
