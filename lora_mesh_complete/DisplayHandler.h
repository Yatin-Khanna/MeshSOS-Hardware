#pragma once

#include<ESP32_LoRaWAN.h>
#include<string.h>

struct DisplayHandler {
  void init();
  void print(const std::string&, uint8_t = 0, uint8_t = 0, bool = false);
  void print(const char*, uint8_t = 0, uint8_t = 0, bool = false);
  void clear();
};
