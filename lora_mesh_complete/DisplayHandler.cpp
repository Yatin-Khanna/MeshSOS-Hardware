#include "DisplayHandler.h"

void DisplayHandler::init() {
  Display.init();
  Display.setFont(ArialMT_Plain_10);
  Display.setTextAlignment(TEXT_ALIGN_LEFT);
  /* Next 2 lines are only for testing */
  Display.drawString(0, 0, "Starting!!");
  Display.display();
}
void DisplayHandler::print(const std::string &str, uint8_t x /* = 0 */, uint8_t y /* = 0 */, bool noClear /* = false */) {
  if(!noClear)
    Display.clear();
  Display.drawString(x, y, str.c_str());
  Display.display();
}
void DisplayHandler::print(const char* str, uint8_t x /* = 0 */, uint8_t y /* = 0 */, bool noClear /* = false */) {
  if(!noClear)
    Display.clear();
  Display.drawString(x, y, str);
  Display.display();
}
void DisplayHandler::clear() {
  Display.clear();
}
