#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <esp_now.h>

// Global copy of slave
#define NUMSLAVES 20
#define CHANNEL 3

#define MAX_SLAVES 16

String formatMacAddress(const uint8_t *MAC);

class AsyncEspNowClass
{
private:
  // Callbacks de ESP-NOW - NO LLAMAR DIRECTAMENTE
  static void _sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
  static void _receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);

  void _beginEspNow();
  void _endEspNow();

  void _configWifiMode();
  void _changeMAC(const uint8_t *customMac);

public:
  // Constructor
  AsyncEspNowClass();

  // Funcion para iniciar ESPnow
  void begin() { _beginEspNow(); }
  void end() { _endEspNow(); }

  // Funcion General
  void setAddress(uint8_t *customMac) { _changeMAC(customMac); }
  String getMacAddress();

  // Send Message
  void send(uint8_t peerAddress[], uint8_t *data, size_t len);

  // Callbacks
  static void onMessage(void (*puntero)(const uint8_t *macAddr, const uint8_t *data, int dataLen));
  static void onSend(void (*puntero)(const uint8_t *address, bool status));
};

// extern AsyncEspNowClass AsyncEspNow();
