#include "IOManager.h"
#include "../config/ConfigManager.h"

static bool relayState[10]={};
static bool stableInput[10]={};
static bool lastRaw[10]={};
static unsigned long edgeAt[10]={};

void IOManager::begin(){
  const DeviceConfig& c=ConfigManager::get();
  for(uint8_t i=0;i<10;i++){
    int p=c.io[i].gpio;if(p<0||c.io[i].mode=="disabled")continue;
    if(c.io[i].mode=="relay"){pinMode(p,OUTPUT);relayState[i]=false;digitalWrite(p,c.io[i].activeLow?HIGH:LOW);}
    else{pinMode(p,INPUT_PULLUP);lastRaw[i]=digitalRead(p);stableInput[i]=lastRaw[i];}
  }
}
void IOManager::loop(){
  const DeviceConfig& c=ConfigManager::get();
  for(uint8_t i=0;i<10;i++){
    if(c.io[i].gpio<0||c.io[i].mode!="button")continue;
    bool v=digitalRead(c.io[i].gpio);
    if(v!=lastRaw[i]){lastRaw[i]=v;edgeAt[i]=millis();}
    if(millis()-edgeAt[i]>=35)stableInput[i]=lastRaw[i];
  }
}
bool IOManager::setRelay(uint8_t ch,bool on){
  if(ch<1||ch>10)return false;const IOConfig& c=ConfigManager::get().io[ch-1];
  if(c.mode!="relay"||c.gpio<0)return false;relayState[ch-1]=on;digitalWrite(c.gpio,c.activeLow?!on:on);return true;
}
bool IOManager::state(uint8_t ch){
  if(ch<1||ch>10)return false;const IOConfig& c=ConfigManager::get().io[ch-1];
  return c.mode=="button"?(c.activeLow?!stableInput[ch-1]:stableInput[ch-1]):relayState[ch-1];
}
