#include "tpm2application.h"

#include <WiFiUdp.h>


#define TPM2NET_IN_PORT 65506
#define TPM2NET_OUT_PORT 65442

WiFiUDP udpTPM;

const uint8_t packet_start_byte = 0x9c;
const uint8_t packet_type_data = 0xda;
const uint8_t packet_type_cmd = 0xc0;
const uint8_t packet_type_response = 0xaa;
const uint8_t packet_end_byte = 0x36;
const uint8_t packet_response_ack = 0xac;

// maximum udp esp8266 payload = 1460 -> 484 rgb leds per frame (22x22 matrix)
// then you have to use packet numbers and split the payload in 32x8 (4 packets)
const uint8_t rowsPerFrame = 8;
const uint16_t payload = 32 * rowsPerFrame * 3;
const uint16_t expected_packet_size = payload + 7;

const void sendAckTPM() {
  udpTPM.beginPacket(udpTPM.remoteIP(), TPM2NET_OUT_PORT);
  udpTPM.write(&packet_response_ack, 1);
  udpTPM.endPacket();
}

TPM2Application::TPM2Application(GFXcanvas& matrix) : Application("TPM2.NET", matrix), _init(false) {
}

void TPM2Application::init() {
  getMatrix().fillRect(0, 0, getMatrix().width(), getMatrix().height(), CRGB::Black);
  getMatrix().setCursor(2, 7);
  getMatrix().print("TPM2...");
  show();
}

void TPM2Application::display() {
  if (_init == false) {
      udpTPM.begin(TPM2NET_IN_PORT);
      _init = true;
  }
  uint16_t packet_size = udpTPM.parsePacket();
  if (packet_size == 0) {
 //   Serial.println("No TMP2 data");
    return;
  }
  // then parse to check
  if (udpTPM.read() != packet_start_byte)
    return;
  // packet type
  uint8_t ptype = udpTPM.read();
  uint16_t frame_size = (udpTPM.read() << 8) | udpTPM.read();
  uint8_t packet = udpTPM.read() - 1; //  starts at 1
  uint8_t npackets = udpTPM.read();
//  Serial.println(String("frameSize: ") + frame_size + " packet: " + packet + "/" + npackets);

  switch (ptype) {
    case packet_type_response:
//      Serial.println("Response");
      sendAckTPM();
      break;

    case packet_type_data: {
        // reunite splitted packets
        if (frame_size != payload)
          return;
 //       Serial.println("Data");
        // fills the buffer directly, starting at packet number offset
        CRGB tpmBuffer[8 * 32];
        udpTPM.read((char*)tpmBuffer, payload);
        if (packet == (npackets - 1)) {
            uint16_t index = 0;
            for (unsigned int y = 0; y < 8; ++y) {
             for (unsigned int x = 0; x < 32; ++x) { 
               getMatrix().drawPixel(x, y, tpmBuffer[index]);
               ++index;
             }
           }
         }
        break;
      }
    case packet_type_cmd: {
        break;
      }
  }
  // skip end byte, maybe not neccessary 0x36
  udpTPM.read();
}
