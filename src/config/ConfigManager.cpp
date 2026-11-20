#include "ConfigManager.h"
#include <LittleFS.h>

DeviceConfig ConfigManager::config;
static bool within(const String& s,size_t n){return s.length()<=n;}

void ConfigManager::begin(){
  if(!LittleFS.begin()){Serial.println(F("LittleFS mount failed"));LittleFS.format();LittleFS.begin();}
  if(load())return;
  config.wifiSsid="";config.wifiPassword="";
  config.brokerHost="";config.brokerPort=443;config.wsPath="/mqtt";
  config.mqttUser="";config.mqttPassword="";
  config.clientId="esp8266-"+String(ESP.getChipId(),HEX);
  config.topicPrefix="devices/"+config.clientId;config.tlsCaPem="";
  config.apSsid="ESP8266-Setup";config.apPassword="esp8266setup";
  for(uint8_t i=0;i<10;i++){config.io[i].name="IO"+String(i+1);config.io[i].gpio=-1;config.io[i].mode="disabled";config.io[i].activeLow=false;}
}
bool ConfigManager::load(){
  if(!LittleFS.exists("/config.json"))return false;
  File f=LittleFS.open("/config.json","r");if(!f)return false;
  DynamicJsonDocument d(8192);auto e=deserializeJson(d,f);f.close();if(e)return false;
  config.wifiSsid=d["wifiSsid"]|"";config.wifiPassword=d["wifiPassword"]|"";
  config.brokerHost=d["brokerHost"]|"";config.brokerPort=d["brokerPort"]|443;config.wsPath=d["wsPath"]|"/mqtt";
  config.mqttUser=d["mqttUser"]|"";config.mqttPassword=d["mqttPassword"]|"";
  config.clientId=d["clientId"]|"";config.topicPrefix=d["topicPrefix"]|"";config.tlsCaPem=d["tlsCaPem"]|"";
  config.apSsid=d["apSsid"]|"ESP8266-Setup";config.apPassword=d["apPassword"]|"esp8266setup";
  JsonArray a=d["io"].as<JsonArray>();
  for(uint8_t i=0;i<10;i++){JsonObject o=a[i];config.io[i].name=o["name"]|("IO"+String(i+1));config.io[i].gpio=o["gpio"]|-1;config.io[i].mode=o["mode"]|"disabled";config.io[i].activeLow=o["activeLow"]|false;}
  return true;
}
bool ConfigManager::persist(String& error){
  DynamicJsonDocument d(8192);
  d["wifiSsid"]=config.wifiSsid;d["wifiPassword"]=config.wifiPassword;
  d["brokerHost"]=config.brokerHost;d["brokerPort"]=config.brokerPort;d["wsPath"]=config.wsPath;
  d["mqttUser"]=config.mqttUser;d["mqttPassword"]=config.mqttPassword;d["clientId"]=config.clientId;
  d["topicPrefix"]=config.topicPrefix;d["tlsCaPem"]=config.tlsCaPem;d["apSsid"]=config.apSsid;d["apPassword"]=config.apPassword;
  JsonArray a=d.createNestedArray("io");
  for(uint8_t i=0;i<10;i++){JsonObject o=a.createNestedObject();o["name"]=config.io[i].name;o["gpio"]=config.io[i].gpio;o["mode"]=config.io[i].mode;o["activeLow"]=config.io[i].activeLow;}
  File f=LittleFS.open("/config.tmp","w");if(!f){error="Cannot open config";return false;}
  size_t n=serializeJson(d,f);f.close();if(!n){LittleFS.remove("/config.tmp");error="Config write failed";return false;}
  LittleFS.remove("/config.json");if(!LittleFS.rename("/config.tmp","/config.json")){error="Config rename failed";return false;}return true;
}
bool ConfigManager::save(JsonVariantConst in,String& error){
  DeviceConfig next=config;
  if(in.containsKey("wifiSsid"))next.wifiSsid=in["wifiSsid"].as<String>();
  if(in["wifiPassword"].is<const char*>()&&String(in["wifiPassword"].as<const char*>()).length())next.wifiPassword=in["wifiPassword"].as<String>();
  if(in.containsKey("brokerHost"))next.brokerHost=in["brokerHost"].as<String>();
  if(in.containsKey("brokerPort"))next.brokerPort=in["brokerPort"].as<uint16_t>();
  if(in.containsKey("wsPath"))next.wsPath=in["wsPath"].as<String>();
  if(in.containsKey("mqttUser"))next.mqttUser=in["mqttUser"].as<String>();
  if(in["mqttPassword"].is<const char*>()&&String(in["mqttPassword"].as<const char*>()).length())next.mqttPassword=in["mqttPassword"].as<String>();
  if(in.containsKey("clientId"))next.clientId=in["clientId"].as<String>();
  if(in.containsKey("topicPrefix"))next.topicPrefix=in["topicPrefix"].as<String>();
  if(in["tlsCaPem"].is<const char*>()&&String(in["tlsCaPem"].as<const char*>()).length())next.tlsCaPem=in["tlsCaPem"].as<String>();
  if(in.containsKey("apSsid"))next.apSsid=in["apSsid"].as<String>();
  if(in["apPassword"].is<const char*>()&&String(in["apPassword"].as<const char*>()).length())next.apPassword=in["apPassword"].as<String>();
  JsonArrayConst a=in["io"].as<JsonArrayConst>();
  if(!a.isNull()){
    if(a.size()!=10){error="Exactly ten IO entries are required";return false;}
    bool used[17]={};
    for(uint8_t i=0;i<10;i++){
      JsonObjectConst o=a[i];next.io[i].name=o["name"]|("IO"+String(i+1));next.io[i].gpio=o["gpio"]|-1;next.io[i].mode=o["mode"]|"disabled";next.io[i].activeLow=o["activeLow"]|false;
      int p=next.io[i].gpio;
      if(p>=0){if(p==6||p==7||p==8||p==9||p==10||p==11||p>16){error="GPIO reserved or invalid";return false;}if(used[p]){error="Duplicate GPIO assignment";return false;}used[p]=true;}
      if(next.io[i].mode!="disabled"&&next.io[i].mode!="relay"&&next.io[i].mode!="button"){error="Invalid IO mode";return false;}
      if(next.io[i].mode!="disabled"&&p<0){error="Enabled IO requires GPIO";return false;}
      if(!within(next.io[i].name,24)){error="IO name too long";return false;}
    }
  }
  if(!within(next.wifiSsid,32)||!within(next.wifiPassword,64)||!within(next.brokerHost,128)||!within(next.wsPath,128)||!within(next.mqttUser,64)||!within(next.mqttPassword,64)||!within(next.clientId,64)||!within(next.topicPrefix,128)||!within(next.tlsCaPem,4096)){error="Field length exceeded";return false;}
  if(!next.brokerPort||!next.wsPath.length()||next.wsPath[0]!='/'||!next.clientId.length()||!next.topicPrefix.length()){error="Broker port, path, Client ID, or topic prefix invalid";return false;}
  if(next.apPassword.length()<8||next.apPassword.length()>63){error="AP password must be 8–63 characters";return false;}
  config=next;return persist(error);
}
const DeviceConfig& ConfigManager::get(){return config;}
bool ConfigManager::ready(){return config.wifiSsid.length()>0;}
