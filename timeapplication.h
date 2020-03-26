#if !defined(TIME_APPLICATION)
#define TIME_APPLICATION

#include "application.h"


class TimeApplication : public Application {
  public:
    TimeApplication(GFXcanvas& matrix);
    ~TimeApplication();
    void display();
  private:
   boolean _init;  
 };

#endif
