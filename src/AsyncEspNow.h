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
  // Callbacks de async funciones - NO LLAMAR DIRECTAMENTE
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

  // Funcion General
  String getMacAddress();

  // Send Message
  void send(uint8_t peerAddress[], uint8_t data[]);

  // Callbacks
  static void onMessage(void (*puntero)(const uint8_t *address, const char *msg));
  static void onSend(void (*puntero)(const uint8_t *address, bool status));

};

// extern AsyncEspNowClass AsyncEspNow();