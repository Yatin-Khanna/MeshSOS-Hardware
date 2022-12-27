#include "Response.h"

Response::Response(): received(false), message("") {} 
void Response::receive(uint8_t* buffer, uint8_t length) {
   received = true;
   message = "Response: ";
   for(int i=0; i<length; i++) {
     message += buffer[i];
   }
}
void Response::reset() {
  received = false;
  message = "";
}
