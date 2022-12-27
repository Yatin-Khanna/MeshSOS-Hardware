#include "GPSHandler.h"
#include <cstring>
#include <string.h>

GPSHandler::GPSHandler(): latitude(0.0), longitude(0.0), year(2000), month(1), day(1), hour(0), minute(0), second(0) {}
void GPSHandler::updateData() {
if(gps.encode(Serial2.read())) {
    latitude = (float)gps.location.lat();
    longitude = (float)gps.location.lng();
    year = gps.date.year();
    month = gps.date.month();
    day = gps.date.day();
    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
  }
}
void GPSHandler::fillAppData(uint8_t* appData) {
   std::memcpy(appData, &latitude, 4);
   std::memcpy(appData + 4, &longitude, 4);
   /* 8th position is for emergency type, will be filled elsewhere */
   std::memcpy(appData + 9, &year, 2);
   appData[11] = month; appData[12] = day;
   appData[13] = hour; appData[14] = minute;
   appData[15] = second;
}
