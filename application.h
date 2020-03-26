#if !defined(APPLICATION_HEADER)
#define APPLICATION_HEADER

#include <FastLED.h>
#include <FastLED_GFX.h>
#include <FS.h>
 
#include "image.h"

class Application {
public:
  Application(String name, GFXcanvas& matrix) : 
    _name(name), _matrix(matrix), _defaultIconLoaded(false), _defaultIcon(0),
    _elapsedIconAnim(0), _iconFrames(1), _currentFrame(0), _iconFps(10) {}
  virtual ~Application() { delete _defaultIcon; }
  virtual void display();
  virtual void button(boolean shortPress) {} // TODO: long and short press
  String& getName() { return _name; }
  GFXcanvas& getMatrix() { return _matrix; }
  Image* decodeFile(const char* name);
  Image* decodeBMPFile(File f);
  Image* decodeGIFFile(File f);
  void drawImage(int xpos, int ypos, Image* image);
  Image* getDefaultIcon() {
    if (_defaultIconLoaded == false) { //  only once
      char buffer[64];
      sprintf(buffer, "/applications/%s.bmp", getName().c_str());
     _defaultIcon = decodeFile(buffer);
      _defaultIconLoaded = true;
      if (_defaultIcon) {
        _iconFrames = _defaultIcon->getHeight() / 8;
      }
    }
     return _defaultIcon;
  }
  int getIconFrames() const { return _iconFrames; }
  int getCurrentFrame() const { return _currentFrame; }
  void setFps(int fps) { _iconFps = fps; }
  void drawDefaultIcon() {
    getDefaultIcon();
    drawImage(0, -8 * _currentFrame, _defaultIcon);
   unsigned long animLength = 1000 / _iconFps;
    if ((millis() - _elapsedIconAnim) >= animLength) {
        _currentFrame = (_currentFrame + 1) % _iconFrames;
        _elapsedIconAnim = millis();
    }
  }
  virtual void init() {}
  virtual void pageOut() {
   // release memory on application switch
   // and allows reloading of new icon from FS
    delete _defaultIcon;
    _defaultIcon = 0;
    _defaultIconLoaded = false;
  }
  void show() { FastLED.show(); }
  private:
   String _name;
   GFXcanvas& _matrix;
   boolean _defaultIconLoaded;
   Image* _defaultIcon;

    unsigned long _elapsedIconAnim;
    int _iconFrames;
    int _currentFrame;
  int _iconFps;
};

#endif
