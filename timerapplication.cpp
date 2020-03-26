#include "timerapplication.h"

void TimerApplication::reset() {
  _startMillis = millis();
  _currentMillis = _startMillis;
}

void TimerApplication::stop() {
  _running = false;
}

void TimerApplication::start() {
  _running = true;
}

void TimerApplication::display() {
  if (_running) 
    _currentMillis = millis();
  unsigned long elapsed = _currentMillis - _startMillis;
  // format
  unsigned long s = elapsed / 1000;
  uint16_t h = s / (60 * 60);
  uint16_t  m = s / 60;
  char buffer[10];
   uint8_t hh = h;
   uint8_t mm = m % 60;
   uint8_t ss = s % 60;
  bool flasher = (ss % 2) == 0;
  sprintf(buffer, "%.2d%c%.2d%c%.2d", hh, (flasher ? ':' : ' '), mm, (flasher ? ':' : ' '), ss);
  getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
  getMatrix().setCursor(3, 7);
  getMatrix().print(buffer); 
}
