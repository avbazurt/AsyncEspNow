#include "AsyncEspNow.h"

int contador = 0;
uint8_t peerAddress[] = {0xC8, 0x2B, 0x96, 0xA8, 0xF6, 0x50};
int period = 100;

void setup() {
  Serial.begin(115200);
  AsyncEspNow.begin();
}

void loop() {
  String msg = "Contador " + String(contador);
  AsyncEspNow.sendMessage(peerAddress, msg.c_str());
  contador += 1;
  delay(period);
}