#if !defined(IMAGE_HEADER)
#define IMAGE_HEADER

#include <Arduino.h>
#include <FastLED.h>

class Image {
  public:
    Image(uint16_t width, uint16_t height, uint16_t colors = 0) 
      : _width(width), _height(height), _dataRGB(0), _dataI(0), _cmap(0) {
         if (colors > 0) {
          _dataI = (byte*)malloc(_width * _height);
          _cmap = (CRGB*)malloc(colors * sizeof(CRGB));
        } else {
          _dataRGB = (CRGB*)malloc(_width * _height * sizeof(CRGB));
        }
  }
  virtual ~Image() {  
    free(_dataRGB);
    free(_dataI);
    free(_cmap);
  }
  uint16_t getWidth() const { return _width; }
  uint16_t getHeight() const { return _height; }
  
  CRGB getPixel(int16_t x, int16_t y) const {
    int idx = y * _width + x;
    if (_cmap)
       return _cmap[_dataI[idx]];
    else
      return _dataRGB[idx];
  }
  void setPixel(int16_t x, int16_t y, CRGB color) {
    int idx = y * _width + x;
    _dataRGB[idx] = color;
  }
  void setPixel(int16_t x, int16_t y, byte color) {
    int idx = y * _width + x;
    _dataI[idx] = color;
  }
  void setCmap(int index, CRGB color) {
    _cmap[index] = color;
  }
  
  CRGB* getDataRGB() const { 
    return _dataRGB; 
  }
  private:
  uint16_t _width;
  uint16_t _height;
  CRGB*    _dataRGB;
  byte*    _dataI;
  CRGB*    _cmap; 
};

#endif
