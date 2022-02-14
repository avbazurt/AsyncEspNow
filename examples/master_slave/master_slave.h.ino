#include "AsyncEspNow.h"

#define LED 4

//Clase AsyncEspNow
AsyncEspNow *EspNow;

//Funcion Callback
void ReciveDataNow(char MAC[], char text[]);


void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);


  EspNow = new AsyncEspNow();
  EspNow->setReciveCallback(ReciveDataNow);
  EspNow->begin();
}

void loop() {
  uint8_t peerAddress[] = {0x7C, 0xDF, 0xA1, 0x06, 0x1F, 0xBE};
  EspNow->sentData(peerAddress, "Hola");
  delay(1000);
}

void ReciveDataNow(char MAC[], char text[]) {
  Serial.printf("Received message from: %s - %s\n", MAC, text);
  digitalWrite(LED, HIGH);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  digitalWrite(LED, LOW);
}
