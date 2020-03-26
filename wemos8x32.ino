#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <FastLED.h>
#include <FastLED_GFX.h>

// lametric custom font...accents are wrong (not iso-latin-1)
//#include "ModTomThumb.h"

// accents shoud be right but numbers look not so nice
// TODO: merge them
#include "ModTomThumb2.h"

#include <FS.h>
#include <ButtonDebounce.h>

#include "application.h"
#include "pacmanapplication.h"
#include "timeapplication.h"
#include "tpm2application.h"
#include "bitcoinapplication.h"
#include "owmapplication.h"

#define LED_PIN D5
#define CANVAS_WIDTH 8
#define CANVAS_HEIGHT 32
#define COLOR_ORDER     GRB
#define CHIPSET         WS2812
#define BRIGHTNESS      32
#define NUM_LEDS (CANVAS_WIDTH * CANVAS_HEIGHT)

// normal
#if 0
inline uint16_t XY(uint16_t x, uint16_t y, uint16_t width) {
  return y * width + x;
}
#else
// alternate (zigzag, bottom)
inline uint16_t XY(uint16_t x, uint16_t y, uint16_t width) {
  uint16_t i;
  if( y & 0x01) {
     // Even rows run forwards
      i = (y * width) + x;
  } else {
      // Odd rows run backwards
      uint8_t reverseX = (width - 1) - x;
      i = (y * width) + reverseX;
   }
    return i;
}
#endif

// deals with rotation and zigzag directly

class MyGFXcanvas : public GFXcanvas {
  public:
    MyGFXcanvas(uint16_t w, uint16_t h) : GFXcanvas(w, h) {}
    void drawPixel(int16_t x, int16_t y, CRGB color) {
      if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;
      uint16_t index = XY(WIDTH  - 1 - y, x, WIDTH);
      getBuffer()[index] = color;
  }
  Image* getImageDataRGB() {
    // RGB Image
    Image* image = new Image(width(), height());
    for (uint16_t y = 0; y < height(); ++y) {
      for (uint16_t x = 0; x < width(); ++x) {
            uint16_t index = XY(WIDTH  - 1 - y, x, WIDTH);
            image->setPixel(x, y,  getBuffer()[index]);
      }
    }
    return image;
  }
};
  
MyGFXcanvas matrix(CANVAS_WIDTH, CANVAS_HEIGHT);  

void bootPacman(unsigned long durationMillis) {
  Application* bootApp = new PacmanApplication(matrix);
  unsigned long startTime = millis();
  while((millis() - startTime) <= durationMillis) {
    bootApp->display();
    FastLED.show();
    delay(10);
  }
  delete bootApp;
}

void getBootFileName(char* buffer, int fileNum) {
   sprintf(buffer, "boot/%d", fileNum);
}

void bootGeneric(unsigned long durationMillis, int filesNum) {
  char buffer[10];
  int idx = 0;
  getBootFileName(buffer, idx);
  Application* bootApp = new Application(buffer, matrix);
    bootApp->setFps(50);
  unsigned long startTime = millis();
  // displayAnim only once
  while((idx < filesNum) && (millis() - startTime) <= durationMillis) {
    bootApp->display();
    FastLED.show();
    if (bootApp->getCurrentFrame() == (bootApp->getIconFrames() - 1)) {
      delete bootApp;
      ++idx;
      getBootFileName(buffer, idx);
       bootApp = new Application(buffer, matrix);
     bootApp->setFps(50);
   }
    delay(10);
  }
  delete bootApp;
}

void boot() {
    const char* filePrefix = "/applications/boot/";
    int len = strlen(filePrefix);
    Dir dir = SPIFFS.openDir(filePrefix);
    int bootFiles = 0;
    while (dir.next()) {
//      Serial.println(dir.fileName());
      if (strncmp(dir.fileName().c_str(), filePrefix, len) == 0)
          ++bootFiles;
    }
    if (bootFiles == 0) {
      bootPacman(4400);
    } else {
      bootGeneric(100000, bootFiles);
    }
}
 
void initOTA() {
  ArduinoOTA.setHostname("WemosTicker");
  ArduinoOTA.setPassword("wemos");
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int realprogress = (progress / (total / 100));
    char buffer[16];
    sprintf(buffer, "OTA /%.2d %%", realprogress);
    matrix.fillScreen(0);
    matrix.setCursor(0, 7);
    matrix.print(buffer);
    FastLED.show();
   });
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();
}

ESP8266WebServer webServer(80);       // Create a webserver object that listens for HTTP request on port 80

class GIFApplication : public Application {
  public:
  GIFApplication(GFXcanvas& matrix) : Application("gif", matrix), _imageLoaded (false), _image(0) {}
  ~GIFApplication() { delete _image; }
  void display();
  private:
  boolean _imageLoaded;
  Image* _image;
};

void GIFApplication::display() {
  if (_imageLoaded == false) { 
    _image = decodeFile("/yoshi2.gif");
    _imageLoaded = true;
  } else {
    getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
    getMatrix().setCursor(10, 7);
    getMatrix().print(getName());
    drawImage(0, 0, _image);
  }
}
 
Application* applications[] = {
  new TimeApplication(matrix),
  new GIFApplication(matrix),
  new BitcoinApplication(matrix),
  new OpenWeatherMapApplication(matrix),
  new Application("test", matrix), // for BMP file in applications/test.bmp, can be uploaded using /upload web page
  new TPM2Application(matrix)
};

uint8_t numApplications = sizeof(applications) / sizeof(Application*);
uint8_t currentApplication = 0; // default is time


void slidePrevImage(int direction) {
  uint16_t width = matrix.width();
//  uint16_t height = matrix.height();
  Image* prevImage = matrix.getImageDataRGB();
  int start = 0;
  for (int idx = 0; idx < width; ++idx) {
    applications[currentApplication]->display();
    applications[currentApplication]->drawImage(start, 0, prevImage);
    start += direction;
    FastLED.show();
    delay(5);
  }
  delete prevImage;
}

void nextApplication() {
  applications[currentApplication]->pageOut();
   currentApplication = (currentApplication + 1) % numApplications;
   applications[currentApplication]->init();
   slidePrevImage(-1); // left
}

void prevApplication() {
  applications[currentApplication]->pageOut();
  if (currentApplication >= 1)
      currentApplication -= 1;
   else
    currentApplication = numApplications - 1;
   applications[currentApplication]->init();
   slidePrevImage(1); // right
}

static unsigned long lastAppTime;

void leftChanged(int state) {
  if (state == HIGH)
    prevApplication();
}

void rightChanged(int state) {
  if (state == HIGH)
    nextApplication();
}

#define L_PIN D6
#define R_PIN D7

ButtonDebounce left(L_PIN, 250); 
ButtonDebounce right(R_PIN, 250); 

String fileList() {
      String str = "";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    str += dir.fileName();
    str += " / ";
    str += dir.fileSize();
    str += "\r\n";
  }
  return str;
}

void debugSPIFFS() {
  Serial.print(fileList());
}

void listFiles() {
  webServer.send(200, "text/plain", fileList()); 
}

File fsUploadFile;

// curl -F "file=filename" <esp8266fs>/edit
void handleFileUpload() {
  if (webServer.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename.clear();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

const char* html = "<form method=\"post\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"name\"><input class=\"button\" type=\"submit\" value=\"edit\"></form>";

void 
sendUploadPage() {
      webServer.send(200, "text/html", html);
}

void setup() {
  Serial.begin(57600);
   // start file system
  SPIFFS.begin();

// debugSPIFFS();


  left.setCallback(leftChanged);
  right.setCallback(rightChanged);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(matrix.getBuffer(), NUM_LEDS); // .setCorrection(TypicalSMD5050); 
  FastLED.setBrightness(BRIGHTNESS);
  matrix.setRotation(1);
  matrix.setFont(&TomThumb);
  matrix.setTextWrap(false);
  matrix.setTextColor(CRGB::White);

    boot();
  
  matrix.fillScreen(0);
  matrix.setCursor(1, 7);
  matrix.print("Wifi...");
  FastLED.show();
 
#if 1
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(20); // increase in production
  wifiManager.setDebugOutput(true);
  wifiManager.setMinimumSignalQuality(30);
  wifiManager.autoConnect("ticker"); //  no password
#else
  WiFi.begin("", "");             // Connect to the network
  Serial.print("Connecting to ");
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }
#endif
   if (WiFi.isConnected()) {
    initOTA();
    webServer.on("/next", HTTP_GET, []() { nextApplication(); webServer.send(200, "text/plain", "ok"); });   
    webServer.on("/prev", HTTP_GET, []() { prevApplication(); webServer.send(200, "text/plain", "ok"); });
    webServer.on("/edit", HTTP_POST, []() { webServer.send(200, "text/plain", "ok"); }, handleFileUpload);
    webServer.on("/list", HTTP_GET, []() { listFiles(); });   
    webServer.on("/upload", HTTP_GET, []() { sendUploadPage(); });   

    webServer.begin();
    MDNS.begin("esp");
   } 
   
  // display IP
    if (WiFi.isConnected()) {
      IPAddress wAddress = WiFi.localIP();
      for (int idx = 0; idx < 4; ++idx) {
           matrix.fillScreen(0);
            matrix.setCursor(1, 7);
            matrix.print(wAddress[idx]);
            FastLED.show();
            delay(1000);
      }
      // display mDNS
      matrix.fillScreen(0);
      matrix.setCursor(1, 7);
      matrix.print("esp.local");
      FastLED.show();
      delay(1000);
    
   } else {
    matrix.print("no");
    FastLED.show();
  } 
  delay(2000);
  lastAppTime = millis();
  applications[currentApplication]->init();
 }

void loop() {
  if (WiFi.isConnected()) {
    ArduinoOTA.handle();
    webServer.handleClient();
    MDNS.update();
  }
  applications[currentApplication]->display();
  FastLED.show();

  // TEST: every minute switch application
#if 0
  if ((millis() - lastAppTime) >= (1000 * 60)) {
    nextApplication();
    lastAppTime = millis();
  }
#endif
  
  left.update();
  right.update();
  delay(10);
}
