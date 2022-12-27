#include "EncodeDecode.h"

/*Encode decode functions */
String encode() {
  std::string encoded = "";
  for(int i=0; i<appDataSize; i++) {
    //appData[i] -> 8 characters
    for(int j=7; j>=0; j--) {
      encoded += (char)('0' + ((appData[i] >> j)&1));
    }
  }
  return String(encoded.c_str());
}


void decode(String& encoded) {
  appDataSize = encoded.length()/8;
  const char* ptr = encoded.c_str();
  for(int i=0; i<encoded.length()/8; i++) {
    uint8_t num = 0;
    for(int j=0; j<8; j++) {
      num *= 2;
      num += (encoded[8*i + j] - '0');    
    }
    appData[i] = num;
  }
}
