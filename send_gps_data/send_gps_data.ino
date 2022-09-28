#include <TinyGPSPlus.h>
#include <ESP32_LoRaWAN.h>
#include "Arduino.h"

/*license for Heltec ESP32 LoRaWan, quary your ChipID relevant license: http://resource.heltec.cn/search */
uint32_t  license[4] = {0x9F9176A5, 0x5E5B30EE, 0x78D0C1D4, 0xF8D6508C};

/* OTAA para*/
uint8_t DevEui[] = { 0x21, 0xE4, 0x3F, 0x4C, 0x2D, 0x33, 0x1F, 0xE3 };
uint8_t AppEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t AppKey[] = { 0x62, 0x4E, 0xAC, 0xA1, 0xFD, 0x38, 0x95, 0xF0, 0x1B, 0x12, 0xBC, 0x64, 0xFA, 0x7D, 0x67, 0xD2 };

/* ABP para*/
uint8_t NwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t AppSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t DevAddr =  ( uint32_t )0x007e6ae1;

static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

bool overTheAirActivation = true;

bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 8;
uint8_t debugLevel = LoRaWAN_DEBUG_LEVEL;
float latitude, longitude;
/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;


#define LEDPin 25  //LED light
void app(uint8_t data)
 {
    // lora_printf("data:%d\r\n",data);
   switch(data)
     {
    case 49:
    {
      pinMode(LEDPin,OUTPUT);
      digitalWrite(LEDPin, HIGH);
      break;
    }
    case 50:
    {
      pinMode(LEDPin,OUTPUT);
      digitalWrite(LEDPin, LOW);
      break;
    }
    case 51:
    {
      break;
    }
    default:
    {
      break;
    }
     }
 }


void  downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  lora_printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  lora_printf("+REV DATA:");
    app(mcpsIndication->Buffer[0]);

  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    lora_printf("%02X",mcpsIndication->Buffer[i]);
  }
  lora_printf("\r\n");
}



static void prepareTxFrame( uint8_t port )
{
    uint8_t* lati_arr = (uint8_t*)&latitude;
    uint8_t* long_arr = (uint8_t*)&longitude;
    appDataSize = 8;//AppDataSize max value is 64
    appData[0] = lati_arr[0];
    appData[1] = lati_arr[1];
    appData[2] = lati_arr[2];
    appData[3] = lati_arr[3];
    appData[4] = long_arr[0];
    appData[5] = long_arr[1];
    appData[6] = long_arr[2];
    appData[7] = long_arr[3];
}

void setup()
{
  Serial2.begin(GPSBaud, SERIAL_8N1, 2, 17); 
  Serial.begin(115200);
  while(!Serial);
  SPI.begin(SCK,MISO,MOSI,SS);
  Mcu.init(SS,RST_LoRa,DIO0,DIO1,license);
  deviceState = DEVICE_STATE_INIT;
}

void loop()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read()))
      displayInfo();
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    
    switch( deviceState )
    {
      case DEVICE_STATE_INIT:
      {
        #if(LORAWAN_DEVEUI_AUTO)
              LoRaWAN.generateDeveuiByChipID();
        #endif
        LoRaWAN.init(loraWanClass,loraWanRegion);
        break;
      }
      case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
      case DEVICE_STATE_SEND:
      {
        prepareTxFrame( appPort );
        LoRaWAN.send(loraWanClass);
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
      case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass,debugLevel);
        break;
      }
      default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
    }
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
  delay(5000);
}
