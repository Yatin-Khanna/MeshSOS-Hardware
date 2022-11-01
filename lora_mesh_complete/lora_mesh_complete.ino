#include <ESP32_LoRaWAN.h>
#include <string.h>
#include <cstring>
#include "TinyGPS++.h"
#include "painlessMesh.h"

uint32_t  license[4] = {0x9F9176A5, 0x5E5B30EE, 0x78D0C1D4, 0xF8D6508C};
const uint32_t GPSBaud = 9600;
/* OTAA para*/
uint8_t DevEui[] = { 0x21, 0xE4, 0x3F, 0x4C, 0x2D, 0x33, 0x1F, 0xE3 };
uint8_t AppEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t AppKey[] = { 0x62, 0x4E, 0xAC, 0xA1, 0xFD, 0x38, 0x95, 0xF0, 0x1B, 0x12, 0xBC, 0x64, 0xFA, 0x7D, 0x67, 0xD2 };

/* ABP para*/
uint8_t NwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t AppSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t DevAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_C;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 8;

/*LoraWan debug level, select in arduino IDE tools.
* None : print basic info.
* Freq : print Tx and Rx freq, DR info.
* Freq && DIO : print Tx and Rx freq, DR, DIO0 interrupt and DIO1 interrupt info.
* Freq && DIO && PW: print Tx and Rx freq, DR, DIO0 interrupt, DIO1 interrupt and MCU deepsleep info.
*/
uint8_t debugLevel = LoRaWAN_DEBUG_LEVEL;

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

#define BAND 865E6
#define LEDPin 25  //LED light
#define HEALTH_BUTTON 34
#define POLICE_BUTTON 35
#define MESH_PREFIX "meshSOS"
#define MESH_PASSWORD "emergencyNetwork"
#define MESH_PORT 5555

struct GPSHandler {
   TinyGPSPlus gps;
   float latitude, longitude;
   uint16_t year;
   uint8_t month, day, hour, minute, second;
   GPSHandler(): latitude(0.0), longitude(0.0), year(0), month(0), day(0), hour(0), minute(0), second(0) {}
   void updateData() {
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
   void fillAppData(uint8_t* appData) {
      std::memcpy(appData, &latitude, 4);
      std::memcpy(appData + 4, &longitude, 4);
      /* 8th position is for emergency type, will be filled elsewhere */
      std::memcpy(appData + 9, &year, 2);
      appData[11] = month; appData[12] = day;
      appData[13] = hour; appData[14] = minute;
      appData[15] = second;
   }
};
GPSHandler GPS;

struct DisplayHandler {
  void init() {
    Display.init();
    Display.setFont(ArialMT_Plain_10);
    Display.setTextAlignment(TEXT_ALIGN_LEFT);
    /* Next 2 lines are only for testing */
    Display.drawString(0, 0, "Starting!!");
    Display.display();
  }
  void print(std::string &str, uint8_t x = 0, uint8_t y = 0) {
    Display.clear();
    Display.drawString(x, y, str.c_str());
    Display.display();
  }
  void print(const char* str, uint8_t x = 0, uint8_t y = 0) {
    Display.clear();
    Display.drawString(x, y, str);
    Display.display();
  }
  void clear() {
    Display.clear();
  }
};
DisplayHandler displayHandler;

struct PushButtonHandler {
   byte healthButtonState, policeButtonState;
   void init() {
     pinMode(HEALTH_BUTTON, INPUT);
     pinMode(POLICE_BUTTON, INPUT);
   }
   void readButtons() { 
     healthButtonState = digitalRead(HEALTH_BUTTON);
     policeButtonState = digitalRead(POLICE_BUTTON);
   }
   /* 0 -> No emergency, 1 -> Health emergency, 2 -> Police emergency */
   uint8_t getEmergency() {
     if(healthButtonState == HIGH) return 1;
     else if(policeButtonState == HIGH) return 2;
     return 0;
   }
};
PushButtonHandler buttonHandler;

struct Response {
   bool received;
   std::string message;
   Response(): received(false), message("") {} 
   void receive(uint8_t* buffer, uint8_t length) {
      received = true;
      message = "Response: ";
      for(int i=0; i<length; i++) {
        message += buffer[i];
      }
   }
   void reset() {
     received = false;
     message = "";
   }
};
Response response;


uint8_t emergency_type;

static void prepareTxFrame( uint8_t port, uint8_t emergency_type ) {
    appDataSize = 16;
    GPS.fillAppData(appData);
    appData[8] = emergency_type;
}


painlessMesh mesh;
Scheduler userScheduler;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

//Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  Serial.println("Sending from node 2...");
  String msg = "Hello from node 2";
  msg += mesh.getNodeId();
  mesh.sendBroadcast( msg ); 
//  taskSendMessage.setInterval( TASK_SECOND * 10 );
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  prepareTxFrame( appPort, 1 );
  LoRaWAN.send(loraWanClass);
  deviceState = DEVICE_STATE_CYCLE;
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


void app(uint8_t data) {
   switch(data) {
      case 49: {
       pinMode(LEDPin,OUTPUT);
       digitalWrite(LEDPin, HIGH);
       break;
     }
     case 50: {
       pinMode(LEDPin,OUTPUT);
       digitalWrite(LEDPin, LOW);
       break;
     }
     case 51: {
       break;
     }
     default: {
       break;
     }
   }
}

void  downLinkDataHandle(McpsIndication_t *mcpsIndication) {
  lora_printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  lora_printf("+REV DATA:");
  app(mcpsIndication->Buffer[0]);
  /* update response */
  response.receive(mcpsIndication->Buffer, mcpsIndication->BufferSize);
  
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++) {
      lora_printf("%c",mcpsIndication->Buffer[i]);
  }
}

void establishLoRaWANConnection() {
  while(IsLoRaMacNetworkJoined == false) {
    switch( deviceState ) {
      case DEVICE_STATE_INIT: {
        #if(LORAWAN_DEVEUI_AUTO)
            LoRaWAN.generateDeveuiByChipID();
        #endif
        LoRaWAN.init(loraWanClass,loraWanRegion);
        break;
      }
      case DEVICE_STATE_JOIN: {
        LoRaWAN.join();
        break;
      }
      case DEVICE_STATE_SEND: {
        return;
      }
      case DEVICE_STATE_CYCLE: {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
      case DEVICE_STATE_SLEEP: {
        LoRaWAN.sleep(loraWanClass,debugLevel);
        break;
      }
      default: {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
    }
  }
}

void establishMeshNetwork() {
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Removed for testing whether it works or not without this
//  userScheduler.addTask( taskSendMessage );
//  taskSendMessage.enable();
}
void setup() {
  Serial.begin(115200);
  Serial2.begin(GPSBaud, SERIAL_8N1, 2, 17);
  while (!Serial);
  /* Initialise display */
  displayHandler.init();
  /* Initialise buttons (associate them with pins) */
  buttonHandler.init();

  /* Library stuff for LoRaWAN/LoRa */
  SPI.begin(SCK,MISO,MOSI,SS);
  Mcu.init(SS,RST_LoRa,DIO0,DIO1,license);
  deviceState = DEVICE_STATE_INIT;
  
  establishLoRaWANConnection();
  establishMeshNetwork();

}

// The loop function is called in an endless loop
void loop() {

  if(IsLoRaMacNetworkJoined == false) {
    //disconnect from mesh
    mesh.stop();
    //reconnect LoRaWAN
    establishLoRaWANConnection(); 
    //reconnect to WiFi mesh network
    establishMeshNetwork();
  }

  mesh.update();
  
  if(response.received) {
    displayHandler.print(response.message);
    delay(5000);
    response.reset();
  }
  
  while(Serial2.available()) {    /* TODO: maybe run it less frequently (every minute maybe??)  */
     GPS.updateData();            /* update GPS data if it can be updated */
  }  

  buttonHandler.readButtons();

  switch( deviceState ) {
    case DEVICE_STATE_INIT: {
      #if(LORAWAN_DEVEUI_AUTO)
          LoRaWAN.generateDeveuiByChipID();
      #endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN: {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND: {
      uint8_t emergency_type = buttonHandler.getEmergency();
      if(emergency_type != 0) {
        std::string message = std::string("Sending ") + std::string(emergency_type == 1 ? "Health " : "Police ") + std::string("Emergency");
        displayHandler.print(message);
        prepareTxFrame( appPort, emergency_type );
        LoRaWAN.send(loraWanClass);
        deviceState = DEVICE_STATE_CYCLE;
      }
      break;
    }
    case DEVICE_STATE_CYCLE: {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP: {
      LoRaWAN.sleep(loraWanClass,debugLevel);
      break;
    }
    default: {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}
