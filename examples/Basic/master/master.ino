#include "AsyncEspNow.h"

//Clase AsyncEspNow
AsyncEspNow *EspNow;

void setup() {
  Serial.begin(115200);
  EspNow = new AsyncEspNow();
  EspNow->begin();
}

void loop() {
  uint8_t peerAddress[] = {0x7C, 0xDF, 0xA1, 0x06, 0x1F, 0xBE};
  EspNow->sentData(peerAddress, "Hola");
  delay(1000);
}