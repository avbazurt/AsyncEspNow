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

String formatMacAddress(const uint8_t *MAC);

class AsyncEspNowClass
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

  // Callbacks de async funciones - NO LLAMAR DIRECTAMENTE
  static void _task_sendData(void *pvParameters);
  static void _task_onMessage(void *pvParameters);

  // Callbacks de ESP-NOW - NO LLAMAR DIRECTAMENTE
  static void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
  static void _receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);

  void _configWifiMode();
  friend void _uint8copy(uint8_t *mac, const uint8_t *macAddr);

  void _beginEspNow();
  void _endEspNow();
  void _changeMAC(const uint8_t *customMac);

public:
  // Constructor
  AsyncEspNowClass();

  // Funcion para iniciar ESPnow
  void begin();
  void end();

  void setAddress(uint8_t *customMac);

  String getMacAddress();

  // Callbacks
  static void onMessage(void (*puntero)(const uint8_t *address, const char *msg));

  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- SEND DATA ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/

  // Funcion para enviar Mensaje por ESPnow
  bool sendMessage(uint8_t peerAddress[], const String &message);

  /*------------------------------------------------------------------------------------------------------*/
  /*---------------------------------------------- CALLCBACK    ---------------------------------------------*/
  /*------------------------------------------------------------------------------------------------------*/
  // friend void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
};

// extern AsyncEspNowClass AsyncEspNow();