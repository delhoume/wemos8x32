#if !defined(PACMAN_APPLICATION)
#define PACMAN_APPLICATION

#include "application.h"


class PacmanApplication : public Application {
  public:
    PacmanApplication(GFXcanvas& matrix);
    ~PacmanApplication();
    void display();
  private:
    boolean _loadedImages = false;
    Image* _ghosts[4];
    Image* _pacman[2];
    unsigned long _elapsedAnim = 0; 
    unsigned long _elapsedPacman = 0;
    int _currentPacman;
    int _pos;
};

#endif
