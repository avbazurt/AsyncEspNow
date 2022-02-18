#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>


struct structReciveData {
  uint8_t macAddr;
  uint8_t data;
  int dataLen;

};


class AsyncEspNow
{
  private:

    //Variable guardar name
    String nameGroup;


    //Variables a utilizar en Send Data
    uint8_t _peerAddressSend[6];
    String  _message_send;


    // Variables a utilizar en Recive Data
    uint8_t _peerAddressRecive[6];
    String  _message_recive;

    static String txt;

    //Semaphoros a Utilizar


    //Timer para control de las tareas
    TaskHandle_t TaskHandSendData = NULL;
    TaskHandle_t TaskHandReciData = NULL;


    // Callbacks de timers - NO LLAMAR DIRECTAMENTE
    static void _task_sendData(void *pvParameters);


    static void _task_reciveData(void *pvParameters);




  public:
    //Constructor
    AsyncEspNow(String name = "ESP");

    //Funcion para iniciar ESPnow
    void begin();

    //Funcion para designar la funcion CallBack
    static void setReciveCallback(void (*puntero)(char MAC[], char text[]));

    esp_now_peer_info_t *dispositivos;

    bool sentData(uint8_t peerAddress[], const String &message);
    void ScanForSlave();

    //Callback
    friend void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
    
    static void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
    
    
    static void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);


};
