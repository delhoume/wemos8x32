#include "bitcoinapplication.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

static HTTPClient httpClient;
static WiFiClient wClient;

const char* bitcoinURL = "http://api.coindesk.com/v1/bpi/currentprice/EUR.json";
unsigned long lastBTCQuery = 0;
int btcValue = -1;

void getBitcoin() {
  if ((lastBTCQuery == 0) || ((millis() - lastBTCQuery) >= 16 * 60 * 1000)) { // every 16 mn
    lastBTCQuery = millis();
   if (WiFi.isConnected()) {
      httpClient.begin(wClient, bitcoinURL);
      if (httpClient.GET()) {
        String json = httpClient.getString();
       //Serial.println(json);
       //Serial.println(json.length());
        DynamicJsonDocument doc(1024);
        auto result = deserializeJson(doc, json);
        if (!result) {
         btcValue = (int)doc["bpi"]["EUR"]["rate_float"]; 
        }
      }
      httpClient.end();
    }
  }
}

BitcoinApplication::BitcoinApplication(GFXcanvas& matrix) : Application("bitcoin", matrix) {
}

void 
BitcoinApplication::display() {
  getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
  char buffer[8];
  getBitcoin();
  // 202 + 32 !
  if (btcValue != -1)
    sprintf(buffer, "%d\xEA", btcValue);
  else
    strcpy(buffer, "Bitcoin");
  drawDefaultIcon();
  getMatrix().setCursor(10, 7);
  getMatrix().print(buffer);
}
