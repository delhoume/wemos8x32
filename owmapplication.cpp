#include "owmapplication.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

static HTTPClient httpClient;
static WiFiClient wClient;

struct WeatherInfo {
  float temperature = 24;
  char description[32] = { 0 };
  char fulldescription[64] = { 0 };
  char icon[8] = { 0 };
};

struct WeatherInfo OWMInfo;

const char* owmApiKey    = "95b2ed7cdfc0136948c0a9d499f807eb"; // Fred's key
const char* owmCityId = "6455401"; // Cachan
const char* owmLang      = "fr"; // fr encodes accents in utf8, not easy to convert

char owmURL[128];

void initOWM() {
  sprintf(owmURL, 
    "http://api.openweathermap.org/data/2.5/weather?id=%s&lang=%s&appid=%s&units=metric", 
    owmCityId, 
    owmLang, 
    owmApiKey);
    OWMInfo.temperature = 50;
    strcpy(OWMInfo.fulldescription, "Chute de lapins");
    strcpy(OWMInfo.icon, "09d");
}

/* UTF-8 to ISO-8859-1/ISO-8859-15 mapper.
 * Return 0..255 for valid ISO-8859-15 code points, 256 otherwise.
*/
static inline unsigned int to_latin9(const unsigned int code) {
    /* Code points 0 to U+00FF are the same in both. */
    if (code < 256U)
        return code;
    switch (code) {
    case 0x0152U: return 188U; /* U+0152 = 0xBC: OE ligature */
    case 0x0153U: return 189U; /* U+0153 = 0xBD: oe ligature */
    case 0x0160U: return 166U; /* U+0160 = 0xA6: S with caron */
    case 0x0161U: return 168U; /* U+0161 = 0xA8: s with caron */
    case 0x0178U: return 190U; /* U+0178 = 0xBE: Y with diaresis */
    case 0x017DU: return 180U; /* U+017D = 0xB4: Z with caron */
    case 0x017EU: return 184U; /* U+017E = 0xB8: z with caron */
    case 0x20ACU: return 164U; /* U+20AC = 0xA4: Euro */
    default:      return 256U;
    }
}

/* Convert an UTF-8 string to ISO-8859-15.
 * All invalid sequences are ignored.
 * Note: output == input is allowed,
 * but   input < output < input + length
 * is not.
 * Output has to have room for (length+1) chars, including the trailing NUL byte.
*/
size_t utf8_to_latin9(char *const output, const char *const input, const size_t length) {
    unsigned char             *out = (unsigned char *)output;
    const unsigned char       *in  = (const unsigned char *)input;
    const unsigned char *const end = (const unsigned char *)input + length;
    unsigned int               c;

    while (in < end)
        if (*in < 128)
            *(out++) = *(in++); /* Valid codepoint */
        else
        if (*in < 192)
            in++;               /* 10000000 .. 10111111 are invalid */
        else
        if (*in < 224) {        /* 110xxxxx 10xxxxxx */
            if (in + 1 >= end)
                break;
            if ((in[1] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x1FU)) << 6U)
                             |  ((unsigned int)(in[1] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 2;

        } else
        if (*in < 240) {        /* 1110xxxx 10xxxxxx 10xxxxxx */
            if (in + 2 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x0FU)) << 12U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[2] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 3;

        } else
        if (*in < 248) {        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 3 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x07U)) << 18U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[3] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 4;

        } else
        if (*in < 252) {        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 4 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x03U)) << 24U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[4] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 5;

        } else
        if (*in < 254) {        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 5 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U &&
                (in[5] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x01U)) << 30U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 24U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[4] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[5] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 6;

        } else
            in++;               /* 11111110 and 11111111 are invalid */

    /* Terminate the output string. */
    *out = '\0';
    return (size_t)(out - (unsigned char *)output);
}

OpenWeatherMapApplication::OpenWeatherMapApplication(GFXcanvas& matrix) : Application("weather", matrix),
  _elapsedTextAnim(0),
  _textPos(32),
  _textLength(0),
  _elapsedIconAnim(0),
  _iconFrames(0),
  _currentIcon(0),
  _icon(0) {
  initOWM();
}

static unsigned long lastOWMQuery = 0;

// TODO: icon
void getOWMInfo() {
    if (WiFi.isConnected()) {
      httpClient.begin(wClient, owmURL);
      if (httpClient.GET()) {
        String json = httpClient.getString();
//        Serial.println(json);
        DynamicJsonDocument doc(2048);
        auto result = deserializeJson(doc, json);
        if (!result) {
          float temp = doc["main"]["temp"];
          OWMInfo.temperature = round(temp);
          JsonObject weather = doc["weather"][0];
          strcpy(OWMInfo.description, weather["main"]);
          static char utf8buf[64];
          strcpy(utf8buf, weather["description"]);
           utf8_to_latin9(OWMInfo.fulldescription, utf8buf, strlen(utf8buf));
           strcpy(OWMInfo.icon, weather["icon"]);
 //          Serial.print(OWMInfo.fulldescription);
        }
      }
      httpClient.end();
    }
 }

static int textFps = 15;
static int iconFps = 5;
static   char buffer[64];

const char* getIcon(const char* icon) {
  if (!strcmp(icon, "01d")) return "/weather/sunny.bmp";
  if (!strcmp(icon, "01n")) return "/weather/moon-stars.bmp";
  if (!strcmp(icon, "02d")) return "/weather/cloudy-partly.bmp";
  if (!strcmp(icon, "02n")) return "/weather/cloudy-partly.bmp";
  if (!strcmp(icon, "03d")) return "/weather/cloudy.bmp";
  if (!strcmp(icon, "03n")) return "/weather/cloudy.bmp";
  if (!strcmp(icon, "04d")) return "/weather/cloudy.bmp";
  if (!strcmp(icon, "04n")) return "/weather/cloudy.bmp";
  if (!strcmp(icon, "09d")) return "/weather/rain.bmp";
  if (!strcmp(icon, "09n")) return "/weather/rain.bmp";
  if (!strcmp(icon, "10d")) return "/weather/rain.bmp";
  if (!strcmp(icon, "10n")) return "/weather/rain.bmp";
  if (!strcmp(icon, "11d")) return "/weather/thunderstorm.bmp";
  if (!strcmp(icon, "11n")) return "/weather/thunderstorm.bmp";
  if (!strcmp(icon, "13d")) return "/weather/snowy.bmp";
  if (!strcmp(icon, "13n")) return "/weather/snowy.bmp";
  if (!strcmp(icon, "50d")) return "/weather/fog.bmp";
  if (!strcmp(icon, "50n")) return "/weather/fog.bmp";
  if (!strcmp(icon, "25d")) return "/weather/snow-house.bmp";
  
  return "/weather/sunny.bmp";
  
}

void 
OpenWeatherMapApplication::display() {
   getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
    if ((lastOWMQuery == 0) || ((millis() - lastOWMQuery) >= 6 * 60 * 1000)) { // every 6 mn
       getOWMInfo();
      lastOWMQuery = millis();
      // degree is 110 + 32 !
      sprintf(buffer, "%d\x8E / %s", (int)OWMInfo.temperature, OWMInfo.fulldescription);
     int16_t x, y;
     uint16_t w, h;
     getMatrix().getTextBounds(buffer, 0, 0, &x, &y, &w, &h);
    _textLength = w;
    _textPos = 32;
    // recompute icon
    delete _icon;
    _icon = decodeFile(getIcon(OWMInfo.icon));
    _currentIcon = 0;
    _iconFrames = _icon ? _icon->getHeight() / 8 : 0;
  }

  getMatrix().setCursor(_textPos, 7);
   getMatrix().print(buffer);
  // black
   getMatrix().drawFastVLine(8, 0, 8, 0); 
   getMatrix().drawFastVLine(9, 0, 8, 0); 
   //drawDefaultImage();
   drawImage(0, -8 * _currentIcon, _icon);
   unsigned long animLength = 1000 / textFps;
  if ((millis() - _elapsedTextAnim) >= animLength) {
      _textPos -= 1;
      if (_textPos <= (-_textLength + 8))
        _textPos = 32;
      _elapsedTextAnim = millis();
  }
  animLength = 1000 / iconFps;
  if ((millis() - _elapsedIconAnim) >= animLength) {
      _currentIcon = (_currentIcon + 1) % _iconFrames;
      _elapsedIconAnim = millis();
  }

}
