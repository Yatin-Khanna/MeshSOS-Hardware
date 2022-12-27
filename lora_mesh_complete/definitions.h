#pragma once
#include<ESP32_LoRaWAN.h>
#include<string.h>
#include<cstring>

extern uint32_t  license[4];
extern const uint32_t GPSBaud;
extern uint8_t DevEui[8], AppEui[8], AppKey[16];
extern uint8_t NwkSKey[16], AppSKey[16];
extern uint32_t DevAddr;
extern uint16_t userChannelsMask[6];
extern DeviceClass_t  loraWanClass;
extern uint32_t appTxDutyCycle;
extern bool overTheAirActivation, loraWanAdr, isTxConfirmed;
extern uint8_t appPort, confirmedNbTrials, debugLevel;
extern LoRaMacRegion_t loraWanRegion;

#define BAND 865E6
#define LEDPin 25  //LED light
#define HEALTH_BUTTON 34
#define POLICE_BUTTON 35
#define MESH_PREFIX "meshSOS"
#define MESH_PASSWORD "emergencyNetwork"
#define MESH_PORT 5555
#define GPS_IN 2
#define GPS_OUT 17
