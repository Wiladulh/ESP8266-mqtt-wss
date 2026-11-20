#include "MqttManager.h"
#include "../config/ConfigManager.h"
#include "../io/IOManager.h"
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

static WebSocketsClient ws;
static bool started=false,wsUp=false,mqttUp=false,connectSent=false,pingOutstanding=false;
static unsigned long lastConnect=0,lastPing=0,lastStatePoll=0;
static uint16_t packetId=1;
static bool previousStates[10]={};

static size_t encLen(uint32_t n,uint8_t* out){size_t i=0;do{uint8_t b=n%128;n/=128;if(n)b|=128;out[i++]=b;}while(n&&i<4);return i;}
static bool decLen(const uint8_t* p,size_t len,uint32_t& n,size_t& used){n=0;uint32_t mul=1;used=0;do{if(used>=len||used>=4)return false;uint8_t b=p[used++];n+=(b&127)*mul;if(!(b&128))return true;mul*=128;}while(true);}
static void putStr(uint8_t* b,size_t& n,const String& s){b[n++]=s.length()>>8;b[n++]=s.length()&255;for(size_t i=0;i<s.length();i++)b[n++]=s[i];}
static void sendConnect(){
  const DeviceConfig& c=ConfigManager::get();uint8_t body[300];size_t n=0;putStr(body,n,"MQTT");body[n++]=4;
  uint8_t flags=2;if(c.mqttUser.length())flags|=128;if(c.mqttPassword.length())flags|=64;
  body[n++]=flags;body[n++]=0;body[n++]=45;putStr(body,n,c.clientId);
  if(c.mqttUser.length())putStr(body,n,c.mqttUser);if(c.mqttPassword.length())putStr(body,n,c.mqttPassword);
  uint8_t p[320];p[0]=0x10;size_t l=encLen(n,p+1);memcpy(p+1+l,body,n);ws.sendBIN(p,n+l+1);connectSent=true;lastConnect=millis();
}
static void subscribeCommands(){
  const DeviceConfig& c=ConfigManager::get();String topic=c.topicPrefix+"/io/+/set";uint8_t b[220];size_t n=0;
  b[n++]=packetId>>8;b[n++]=packetId&255;putStr(b,n,topic);b[n++]=0;
  uint8_t p[240];p[0]=0x82;size_t l=encLen(n,p+1);memcpy(p+1+l,b,n);ws.sendBIN(p,n+l+1);packetId++;
}
static void publish(const String& topic,const String& payload){
  uint8_t b[420];size_t n=0;putStr(b,n,topic);
  for(size_t i=0;i<payload.length()&&n<410;i++)b[n++]=payload[i];
  uint8_t p[440];p[0]=0x30;size_t l=encLen(n,p+1);memcpy(p+1+l,b,n);ws.sendBIN(p,n+l+1);
}
static void receivePublish(const uint8_t* p,size_t len){
  uint32_t rem;size_t l;if(!decLen(p+1,len-1,rem,l))return;size_t pos=1+l;if(pos+2>len)return;
  uint16_t tl=(p[pos]<<8)|p[pos+1];pos+=2;if(pos+tl>len)return;String topic;
  for(uint16_t i=0;i<tl;i++)topic+=(char)p[pos++];uint8_t qos=(p[0]>>1)&3;if(qos)pos+=2;if(pos>len)return;
  String value;while(pos<len)value+=(char)p[pos++];value.trim();
  const DeviceConfig& c=ConfigManager::get();String root=c.topicPrefix+"/io/";
  if(!topic.startsWith(root)||!topic.endsWith("/set"))return;
  int ch=topic.substring(root.length(),topic.length()-4).toInt();if(ch<1||ch>10)return;
  if(value.startsWith("{")){DynamicJsonDocument d(128);if(!deserializeJson(d,value))value=d["state"]|"";}
  value.toUpperCase();if(value=="ON"||value=="OFF")if(IOManager::setRelay(ch,value=="ON"))publish(root+String(ch)+"/state",value);
}
static void onWs(WStype_t t,uint8_t* p,size_t n){
  if(t==WStype_CONNECTED){wsUp=true;mqttUp=false;connectSent=false;}
  else if(t==WStype_DISCONNECTED){wsUp=false;mqttUp=false;connectSent=false;pingOutstanding=false;}
  else if(t==WStype_BIN&&n>=2){
    if(p[0]==0x20&&n>=4&&p[3]==0){mqttUp=true;subscribeCommands();publish(ConfigManager::get().topicPrefix+"/status","online");}
    else if(p[0]==0xD0){pingOutstanding=false;lastPing=millis();}
    else if((p[0]&0xF0)==0x30)receivePublish(p,n);
  }
}
namespace MqttManager {
void begin(){}
void loop(){
  const DeviceConfig& c=ConfigManager::get();
  if(WiFi.status()!=WL_CONNECTED||!c.brokerHost.length()||!c.tlsCaPem.length()){
    if(started){ws.disconnect();started=false;}wsUp=false;mqttUp=false;return;
  }
  if(!started){ws.beginSslWithCA(c.brokerHost.c_str(),c.brokerPort,c.wsPath.c_str(),c.tlsCaPem.c_str(),"mqtt");ws.onEvent(onWs);ws.setReconnectInterval(10000);started=true;lastConnect=millis();}
  ws.loop();
  if(wsUp&&!mqttUp&&!connectSent&&millis()-lastConnect>1500)sendConnect();
  if(mqttUp&&millis()-lastPing>20000){
    if(pingOutstanding){mqttUp=false;ws.disconnect();lastConnect=millis();pingOutstanding=false;}
    else{uint8_t ping[2]={0xC0,0};ws.sendBIN(ping,2);pingOutstanding=true;lastPing=millis();}
  }
  if(mqttUp&&millis()-lastStatePoll>250){
    lastStatePoll=millis();for(uint8_t i=0;i<10;i++){bool s=IOManager::state(i+1);const IOConfig& io=c.io[i];
      if(io.gpio>=0&&io.mode!="disabled"&&s!=previousStates[i]){previousStates[i]=s;publish(c.topicPrefix+"/io/"+String(i+1)+"/state",s?(io.mode=="button"?"PRESSED":"ON"):(io.mode=="button"?"RELEASED":"OFF"));}
    }
  }
}
bool connected(){return mqttUp;}
}
