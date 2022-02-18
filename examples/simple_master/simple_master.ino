#include "AsyncEspNow.h"

AsyncEspNow *EspNow;


void setup() {
  Serial.begin(115200);
  
  EspNow = new AsyncEspNow();
  //EspNow->setReciveCallback(ReciveDataNow);
  EspNow->begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}
