#if !defined(OWM_APPLICATION)
#define OWM_APPLICATION

#include "application.h"

class OpenWeatherMapApplication : public Application {
  public:
    OpenWeatherMapApplication(GFXcanvas& matrix);
    ~OpenWeatherMapApplication() {}
    void display();
    private:
    unsigned long _elapsedTextAnim;
    int _textPos;
    int _textLength;
    unsigned long _elapsedIconAnim;
    int _iconFrames;
    int _currentIcon;
    Image* _icon;
};

#endif
