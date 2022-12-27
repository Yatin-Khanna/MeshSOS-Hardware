#include <ESP32_LoRaWAN.h>
#include <string.h>
#include <cstring>
#include "TinyGPS++.h"
#include "painlessMesh.h"
#include "definitions.h"        //contains default values of variables and macro definitions used in the below code
#include "GPSHandler.h"
#include "DisplayHandler.h"
#include "PushButtonHandler.h"
#include "Response.h"
#include "EncodeDecode.h"

std::string DEVICE_ID = "eui-21e43f4c2d331fe3";

bool allowBroadcast = true;

GPSHandler GPS;
DisplayHandler displayHandler;
PushButtonHandler buttonHandler;
Response response;

uint8_t emergency_type;

static void prepareTxFrame( uint8_t port, uint8_t emergency_type ) {
    appDataSize = 16 + DEVICE_ID.size();
    GPS.fillAppData(appData);
    appData[8] = emergency_type;
    for(int i=16; i<appDataSize; i++) {
      appData[i] = (uint8_t)DEVICE_ID[i - 16];
    }
}


painlessMesh mesh;
Scheduler userScheduler;

// User stub
void enableBroadcast() {
  Serial.printf("Enabled Broadcast");
  allowBroadcast = true;
}

Task taskEnableBroadcast( TASK_SECOND * 30 , TASK_FOREVER, &enableBroadcast );

void sendMessage() {
  Serial.println("Sending from node 2...");
  mesh.sendBroadcast(encode()); 
//  taskSendMessage.setInterval( TASK_SECOND * 10 );
}


std::string to_string(uint32_t from) {
  std::string result = "";
  while(from != 0) {
    result += (char)(from % 10 + '0');
    from /= 10;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  if(allowBroadcast == false) return;
  Serial.println("RECEIVE");
  decode(msg);
  std::string id = "";
  for(int i=16; i<appDataSize; i++) {
    id += appData[i];
  }
  displayHandler.print(id);
  displayHandler.print(to_string(from), 0, 15, true);
  displayHandler.print(appData[8] == 0x01 ? "Health Emergency" : "Police Emergency", 0, 30, true);
  displayHandler.print("[Received over Wi-Fi]", 20, 45, true); 
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

   LoRaWAN.send(loraWanClass);
   sendMessage();
   allowBroadcast = false;
   Serial.printf("Disabled Broadcast");
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
  Serial2.begin(GPSBaud, SERIAL_8N1, GPS_IN, GPS_OUT);
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

  userScheduler.addTask( taskEnableBroadcast );
  taskEnableBroadcast.enable();

}

// The loop function is called in an endless loop
void loop() {
  if(IsLoRaMacNetworkJoined == false) {
    //disconnect from WiFi mesh network
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
        sendMessage();
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
