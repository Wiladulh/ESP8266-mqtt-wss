#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

struct IOConfig { String name; int8_t gpio; String mode; bool activeLow; };
struct DeviceConfig {
  String wifiSsid, wifiPassword;
  String brokerHost, wsPath, mqttUser, mqttPassword, clientId, topicPrefix, tlsCaPem;
  uint16_t brokerPort;
  String apSsid, apPassword;
  IOConfig io[10];
};
class ConfigManager {
public:
  static void begin();
  static const DeviceConfig& get();
  static bool save(JsonVariantConst input, String& error);
  static bool ready();
private:
  static DeviceConfig config;
  static bool load();
  static bool persist(String& error);
};
