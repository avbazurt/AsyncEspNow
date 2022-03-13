#include "AsyncEspNow.h"

//Clase AsyncEspNow
AsyncEspNow *EspNow;

//Constantes
int contador = 0;
String mensaje;

void setup() {
  Serial.begin(115200);
  EspNow = new AsyncEspNow();
  EspNow->begin();
}

void loop() {
  mensaje = "Hola Mundo " + String(contador);
  EspNow->sendMessageAll(mensaje);
  delay(1500);
}
