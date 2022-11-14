#include "AsyncEspNow.h"

//Funcion Callback
void ReciveDataNow(char MAC[], char text[]);

void setup() {
  Serial.begin(115200);

  AsyncEspNow.onMessage(ReciveDataNow);
  AsyncEspNow.begin();
}

void loop() {
}

void ReciveDataNow(char MAC[], char text[]) {
  Serial.printf("Received message from: %s - %s\n", MAC, text);
}
