#include "AsyncEspNow.h"

int lastData;

// Funcion Callback
void ReciveDataNow(const uint8_t *address, const char *msg)
{
  String mac = formatMacAddress(address);
  String text = String(msg);
  int period = millis() - lastData;
  lastData = millis();
  Serial.printf("Recive to %s, message: %s, period: %d\n", mac.c_str(), text, period);
}

void setup()
{
  Serial.begin(115200);
  AsyncEspNow.onMessage(ReciveDataNow);
  AsyncEspNow.begin();
}

void loop()
{
}