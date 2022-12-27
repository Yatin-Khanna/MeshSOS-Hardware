#pragma once
#include "TinyGPS++.h"
struct GPSHandler {
   TinyGPSPlus gps;
   float latitude, longitude;
   uint16_t year;
   uint8_t month, day, hour, minute, second;
   GPSHandler();
   void updateData();
   void fillAppData(uint8_t*);
};
