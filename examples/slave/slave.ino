#include "AsyncEspNow.h"

//Clase AsyncEspNow
AsyncEspNow *EspNow;

//Funcion Callback
void ReciveDataNow(char MAC[], char text[]);

void setup() {
  Serial.begin(115200);

  EspNow = new AsyncEspNow();
  EspNow->setReciveCallback(ReciveDataNow);
  EspNow->begin();
}

void loop() {
}

void ReciveDataNow(char MAC[], char text[]) {
  Serial.printf("Received message from: %s - %s\n", MAC, text);
}
