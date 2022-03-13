#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// Global copy of slave
#define NUMSLAVES 20
#define CHANNEL 3

#define MAX_SLAVES 16


struct structReciveData
{
  uint8_t macAddr;
  uint8_t data;
  int dataLen;
};

class AsyncEspNow
{
private:
  // Variable guardar name
  String nameGroup;

  // Struct Message
  struct NowMessage
  {
    esp_now_peer_info_t now_peer = {};
    String message = "";
  };

  /*----- SEND DATA -------*/
  // FUncion interna para enviar Data por ESPnow
  bool sentData(NowMessage NowMessageSend);


  // Variables a utilizar en Recive Data
  uint8_t _peerAddressRecive[6];
  String _message_recive;

  static String txt;


  // Timer para control de las tareas
  TaskHandle_t TaskHandSendData = NULL;
  TaskHandle_t TaskHandReciData = NULL;

  // Callbacks de timers - NO LLAMAR DIRECTAMENTE
  static void _task_sendData(void *pvParameters);
  static void _task_reciveData(void *pvParameters);



public:
  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- -GENERAL- ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/

  // Constructor
  AsyncEspNow(String name = "ESP");

  // Funcion para iniciar ESPnow
  void begin();

  // Funcion para designar la funcion CallBack
  static void setReciveCallback(void (*puntero)(char MAC[], char text[]));

  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- SEND DATA ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/

  // Funcion para enviar Mensaje por ESPnow
  bool sendMessage(uint8_t peerAddress[], const String &message);

  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- SCANER    ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/

  // Funciones por Escaner
  int ScanForSlaves(esp_now_peer_info_t *slaves_devices);
  int sendMessageAll(String msg);

  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- CALLCBACK    ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/
  friend void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);

  static void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);

  static void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
};
