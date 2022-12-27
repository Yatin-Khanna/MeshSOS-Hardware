#pragma once
#include <string.h>
#include <cstring>
#include <Arduino.h>
struct Response {
   bool received;
   std::string message;
   Response(); 
   void receive(uint8_t*, uint8_t);
   void reset();
};
