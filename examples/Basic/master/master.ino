#include "AsyncEspNow.h"

void setup() {
  Serial.begin(115200);
  AsyncEspNow.begin();
}

void loop() {
  uint8_t peerAddress[] = {0x7C, 0xDF, 0xA1, 0x06, 0x1F, 0xBE};
  AsyncEspNow.sendMessage(peerAddress, "Hola");
  delay(1000);
}
