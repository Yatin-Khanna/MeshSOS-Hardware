#include "PushButtonHandler.h"

void PushButtonHandler::init() {
  pinMode(HEALTH_BUTTON, INPUT);
  pinMode(POLICE_BUTTON, INPUT);
}
void PushButtonHandler::readButtons() { 
  healthButtonState = digitalRead(HEALTH_BUTTON);
  policeButtonState = digitalRead(POLICE_BUTTON);
}
/* 0 -> No emergency, 1 -> Health emergency, 2 -> Police emergency */
uint8_t PushButtonHandler::getEmergency() {
  if(healthButtonState == HIGH) return 1;
  else if(policeButtonState == HIGH) return 2;
  return 0;
}
