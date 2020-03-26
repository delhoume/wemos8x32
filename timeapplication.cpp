#include "timeapplication.h"

#include <Timezone.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>


unsigned int localPort = 1024;      // local port to listen for UDP packets, above 1023....
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.google.com";
//const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

WiFiUDP udp;

void sendNTPpacket(IPAddress& address, byte* packet) {
//   Serial.println("Requesting NTP");
    memset(packet, 0, NTP_PACKET_SIZE);
    packet[0] = 0b11100011;   // LI, Version, Mode
    packet[1] = 0;     // Stratum, or type of clock
    packet[2] = 6;     // Polling Interval
    packet[3] = 0xEC;  // Peer Clock Precision
                                // 8 bytes of zero for Root Delay & Root Dispersion
    packet[12] = 49;
    packet[13] = 0x4E;
    packet[14] = 49;
    packet[15] = 52;

    if (udp.beginPacket(address, 123) == 0) { //NTP requests are to port 123
//     Serial.println("Failed beginPacket");
    }
    udp.write(packet, NTP_PACKET_SIZE);
    udp.endPacket();
}

time_t getNtpTime() {
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
  WiFi.hostByName(ntpServerName, timeServerIP);
//  Serial.print("Getting time from ");
//  Serial.println(timeServerIP);
  while (udp.parsePacket() > 0) { // discard any previously received packets
 //   Serial.print("x");
  }
  sendNTPpacket(timeServerIP, packetBuffer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    Serial.print(".");
    int size = udp.parsePacket();
//    Serial.print("Received UDP Response ");
//    Serial.println(size);
    if (size >= NTP_PACKET_SIZE) {      
 ////     Serial.println("Received NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    } else {
      delay(100);
    }
  }
  Serial.println();
  Serial.println("No NTP Response :-(");
  udp.stop();
  return 0;
}

TimeApplication::TimeApplication(GFXcanvas& matrix) : Application("time", matrix),
 _init(false) {
}
    
TimeApplication::~TimeApplication() {}

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;

const char* days[] = { "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"  };

unsigned int lastSwitchTime = 0;
unsigned int switchTime = 10 * 1000; // 10 seconds
boolean displayTime = false; // or date

inline void drawTime(GFXcanvas& matrix) {
  int iconW = 8;
  uint16_t textW = 0;
  char timeBuffer[12];
  time_t utc = now(); // triggers update if necessary
  time_t local = CE.toLocal(utc, &tcr);
  matrix.setTextColor(CRGB::White);
   if ((lastSwitchTime == 0) || ((millis() - lastSwitchTime) >= switchTime)) {
      displayTime = !displayTime;
      lastSwitchTime = millis();
  } if (displayTime) {
      uint16_t  m = minute(local);
      uint16_t  h = hour(local);
      uint16_t  s = second(local);
      uint8_t m1 = m / 10;
      uint8_t m2 = m % 10;
      uint8_t h1 = h / 10;
      uint8_t h2 = h % 10;
      bool flasher = (s % 2) == 0;
      sprintf(timeBuffer, "%d%d%c%d%d", h1, h2, (flasher ? ':' : ' '), m1, m2);
      textW = 17;
   } else {
    uint16_t d = day(local);
      sprintf(timeBuffer, "%s %d", days[weekday(local) - 1], d);
      textW = d > 9 ? 21 : 17;
   }
   // does not work
 #if 0
   // center in remaining area
   int16_t x, y;
   uint16_t textW, h; 
   matrix.getTextBounds(timeBuffer, 0, 0, &x, &y, &textW, &h);
#endif
   int pos = iconW + (matrix.width() - iconW  - textW) / 2;
   matrix.setCursor(pos, 7);
   matrix.print(timeBuffer);
}

void TimeApplication::display() {
  if (_init == false) {
    if (WiFi.isConnected()) {
      if (udp.begin(localPort) == 0) {
        Serial.println("Cannot start UDP");
      }
      setSyncInterval(3600 * 10); // every 10 hours
      setSyncProvider(getNtpTime);
    }
    _init = true;
  }
    getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
    drawDefaultIcon();
    drawTime(getMatrix());
}
