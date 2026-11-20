#pragma once
#include <Arduino.h>
namespace IOManager { void begin(); void loop(); bool setRelay(uint8_t channel,bool on); bool state(uint8_t channel); }
