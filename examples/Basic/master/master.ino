#include "AsyncEspNow.h"

AsyncEspNow AsyncEspNow;

typedef struct msgData {
  float temperature;
  int contador;
};

uint8_t peerAddress[] = {0xC8, 0x2B, 0x96, 0xA8, 0xF6, 0x50};
int period = 100;
int cnt = 0;

void sendDataNow(const uint8_t *address, bool status) {
  String mac = formatMacAddress(address);
  Serial.printf("Send message to %s, Status: %s\n", mac.c_str(), status ? "true" : "false");
}

void setup()
{
  Serial.begin(115200);
  AsyncEspNow.onSend(sendDataNow);
  AsyncEspNow.setMode(ESP_MASTER);
}

void loop()
{
  msgData msg;
  msg.temperature = random(2400, 3200) / 100.00;
  msg.contador = cnt;

  AsyncEspNow.send(peerAddress, ((uint8_t *)&msg), sizeof(msg));
  cnt += 1;
  delay(period);
}