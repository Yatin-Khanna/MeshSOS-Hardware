#pragma once
#include "definitions.h"
#include <ESP32_LoRaWAN.h>

struct PushButtonHandler {
   byte healthButtonState, policeButtonState;
   void init();
   void readButtons();
   uint8_t getEmergency();
};
