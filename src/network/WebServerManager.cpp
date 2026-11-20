#include "WebServerManager.h"
#include "WiFiManager.h"
#include "../config/ConfigManager.h"
#include "../io/IOManager.h"
#include "../mqtt/MqttManager.h"
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

static ESP8266WebServer server(80);
static void respond(int code,DynamicJsonDocument& d){String s;serializeJson(d,s);server.send(code,"application/json",s);}
static void configGet(){
  const DeviceConfig& c=ConfigManager::get();DynamicJsonDocument d(4096);
  d["wifiSsid"]=c.wifiSsid;d["hasWifiPassword"]=c.wifiPassword.length()>0;
  d["brokerHost"]=c.brokerHost;d["brokerPort"]=c.brokerPort;d["wsPath"]=c.wsPath;
  d["mqttUser"]=c.mqttUser;d["hasMqttPassword"]=c.mqttPassword.length()>0;
  d["clientId"]=c.clientId;d["topicPrefix"]=c.topicPrefix;d["hasTlsCa"]=c.tlsCaPem.length()>0;
  d["apSsid"]=c.apSsid;d["hasApPassword"]=c.apPassword.length()>0;
  JsonArray a=d.createNestedArray("io");
  for(uint8_t i=0;i<10;i++){JsonObject o=a.createNestedObject();o["name"]=c.io[i].name;o["gpio"]=c.io[i].gpio;o["mode"]=c.io[i].mode;o["activeLow"]=c.io[i].activeLow;}
  respond(200,d);
}
static void configPost(){
  DynamicJsonDocument d(8192);if(deserializeJson(d,server.arg("plain"))){server.send(400,"application/json","{\"error\":\"Invalid JSON\"}");return;}
  String error;if(!ConfigManager::save(d.as<JsonVariantConst>(),error)){DynamicJsonDocument e(256);e["error"]=error;respond(400,e);return;}
  server.send(200,"application/json","{\"ok\":true,\"message\":\"Saved. Device is rebooting to apply changes.\"}");delay(350);ESP.restart();
}
static void statusGet(){
  DynamicJsonDocument d(2048);d["wifiConnected"]=WiFi.status()==WL_CONNECTED;d["apMode"]=WiFiManagerApp::isApMode();
  d["ip"]=WiFiManagerApp::isApMode()?WiFi.softAPIP().toString():WiFi.localIP().toString();d["rssi"]=WiFi.RSSI();d["mqttConnected"]=MqttManager::connected();
  JsonArray a=d.createNestedArray("io");const DeviceConfig& c=ConfigManager::get();
  for(uint8_t i=0;i<10;i++){JsonObject o=a.createNestedObject();o["name"]=c.io[i].name;o["mode"]=c.io[i].mode;o["configured"]=c.io[i].gpio>=0&&c.io[i].mode!="disabled";o["state"]=IOManager::state(i+1);}
  respond(200,d);
}
static void ioPost(){
  DynamicJsonDocument d(256);if(deserializeJson(d,server.arg("plain"))){server.send(400,"application/json","{\"error\":\"Invalid JSON\"}");return;}
  uint8_t ch=d["channel"]|0;String s=d["state"]|"";
  if((s!="ON"&&s!="OFF")||!IOManager::setRelay(ch,s=="ON")){server.send(400,"application/json","{\"error\":\"Invalid channel or relay mode\"}");return;}
  server.send(200,"application/json","{\"ok\":true}");
}
namespace WebServerManager {
void begin(){
  server.on("/",HTTP_GET,[]{File f=LittleFS.open("/index.html","r");if(!f){server.send(500,"text/plain","Upload LittleFS filesystem image first");return;}server.streamFile(f,"text/html");f.close();});
  server.on("/app.js",HTTP_GET,[]{File f=LittleFS.open("/app.js","r");if(!f){server.send(404,"text/plain","Not found");return;}server.streamFile(f,"application/javascript");f.close();});
  server.on("/style.css",HTTP_GET,[]{File f=LittleFS.open("/style.css","r");if(!f){server.send(404,"text/plain","Not found");return;}server.streamFile(f,"text/css");f.close();});
  server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configPost);
  server.on("/api/status",HTTP_GET,statusGet);server.on("/api/io",HTTP_POST,ioPost);server.begin();
}
void loop(){server.handleClient();}
}
