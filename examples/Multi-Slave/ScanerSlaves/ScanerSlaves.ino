#include "AsyncEspNow.h"

//Clase AsyncEspNow
AsyncEspNow *EspNow;


void setup() {
  Serial.begin(115200);
  EspNow = new AsyncEspNow();
  EspNow->begin();
}

void loop() {
  Serial.println("Iniciando Escaner: ");
  
  //Escanemos los dispositivos cercanos
  esp_now_peer_info_t slaves_devices[MAX_SLAVES] = {};
  int SlaveCnt = EspNow->ScanForSlaves(slaves_devices);

  //Constantes
  unsigned char *macAddr;
  char macStr[18];

  //Verificamos los dispositivos
  for (int i=0; i<SlaveCnt;i++){
    macAddr = slaves_devices[i].peer_addr;

    // formatMacAddress
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    //Mostrar resultados
    Serial.printf("%d) %s\n",i,macStr);
  }

  Serial.println("Se termino Escaner\n");
  delay(1000);

}
