#include "invaderapplication.h"

static int fps = 15;

void InvaderApplication::display() {
 unsigned long animLength = 1000 / fps;
  getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
  if ((millis() - _elapsedAnim) >= animLength) {
      _pos += 1;
      if (_pos > 32)
        _pos = -8;
      _elapsedAnim = millis();
  }
  drawImage(_pos, 0, getDefaultIcon());
}
