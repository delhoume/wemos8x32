#if !defined(INVADER_APPLICATION)
#define INVADER_APPLICATION

#include "application.h"

class InvaderApplication : public Application {
  public:
    InvaderApplication(GFXcanvas& matrix) : Application("invader", matrix), _invader(0), _pos(-8), _elapsedAnim(0) {
    }
    ~InvaderApplication() {
        delete _invader;
    }
    void display();
  private:
  Image* _invader;
  int _pos;
  unsigned long _elapsedAnim = 0; 
};

#endif
