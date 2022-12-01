#include "AsyncEspNow.h"

AsyncEspNowClass AsyncEspNow;

typedef struct msgData {
  float temperature;
  int contador;
};

// Funcion Callback
int cnt;
void reciveDataNow(const uint8_t *address, const uint8_t *data, int dataLen)
{
  msgData msg;
  memcpy(&msg, data, sizeof(msg));

  String mac = formatMacAddress(address);
  Serial.printf("Recive to %s \n", mac.c_str());
  Serial.print("Bytes received: ");
  Serial.println(dataLen);
  Serial.print("Temperature: ");
  Serial.println(msg.temperature);
  Serial.print("Contador: ");
  Serial.println(msg.contador);
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  AsyncEspNow.onMessage(reciveDataNow);

  /* If necesary, you change MAC de slave ESP.
  uint8_t newAddress[] = {0xC8, 0x2B, 0x96, 0xA8, 0xF6, 0x50};
  AsyncEspNow.setAddress(newAddress);
  */
  
  AsyncEspNow.setMode(ESP_SLAVE);
}

void loop()
{
}