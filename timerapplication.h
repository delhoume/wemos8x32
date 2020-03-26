#if !defined(TIMER_APPLICATION)
#define TIMER_APPLICATION

#include "application.h"

class TimerApplication : public Application {
  public:
    TimerApplication(GFXcanvas& matrix) : Application("timer", matrix), _startMillis(0), _currentMillis(0), _running(true) {}
    ~TimerApplication() {}
    void display();
    void reset();
    void stop();
    void start();
  private:
  unsigned long _startMillis;
  unsigned long _currentMillis;
  boolean _running;
};

#endif
