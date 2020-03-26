
#include "pacmanapplication.h"

PacmanApplication::PacmanApplication(GFXcanvas& matrix)
 : Application("pacman", matrix),
   _pos(-55),
   _currentPacman(0) {
 }
   
static unsigned long pps = 2;  // two pacman per second
static unsigned long fps = 20; // frames per second for scrolling

void PacmanApplication::display() {
  unsigned long animLength = 1000 / fps;
     getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);

    if (_loadedImages == false) {
      _ghosts[0] = decodeFile("/pacman/ghost1.bmp");
      _ghosts[1] = decodeFile("/pacman/ghost2.bmp");
      _ghosts[2] = decodeFile("/pacman/ghost3.bmp");
      _ghosts[3] = decodeFile("/pacman/ghost4.bmp");

      _pacman[0] = decodeFile("/pacman/pacman1.bmp");
      _pacman[1] = decodeFile("/pacman/pacman2.bmp");
      _loadedImages = true;
    }
    // change pos and limit framerate for animation
    if ((millis() - _elapsedAnim) >= animLength) {
      _pos += 1;
      if (_pos > 32)
        _pos = -55;
      _elapsedAnim = millis();
    }
    // change pacman
    unsigned long pacmanLength = 1000 / pps;
       if ((millis() - _elapsedPacman) >= pacmanLength) {
          _currentPacman = (_currentPacman + 1) % 2;
          _elapsedPacman = millis();
      }
    drawImage(_pos +  0, 0, _ghosts[0]);
    drawImage(_pos + 10, 0, _ghosts[1]);
    drawImage(_pos + 20, 0, _ghosts[2]);
    drawImage(_pos + 30, 0, _ghosts[3]);
    
    drawImage(_pos + 45, 0, _pacman[_currentPacman]);
}

PacmanApplication::~PacmanApplication() {
  delete _ghosts[0];
  delete _ghosts[1];
  delete _ghosts[2];
  delete _ghosts[3];
  delete _pacman[0];
  delete _pacman[1];
}
