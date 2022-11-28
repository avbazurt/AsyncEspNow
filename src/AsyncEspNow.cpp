#include "AsyncEspNow.h"
#include "EspConfig.h"
#include <esp_wifi.h> //NEED THIS TO COMPILE

// Puntero Callback
bool status_send;

// Estructura de los mensajes
struct ESPNOW_mensaje
{
  const uint8_t *_address;
  char *msg;
};

// Semaphoro
SemaphoreHandle_t SendDataSemaphore = xSemaphoreCreateCounting(1, 0);

String formatMacAddress(const uint8_t *MAC)
{
  char macStr[18];
  // formatMacAddress
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

  return String(macStr);
}

/*-------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- GENERAL ESP ---------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/

AsyncEspNowClass::AsyncEspNowClass()
{
}

void AsyncEspNowClass::_configWifiMode()
{
  // Get WiFi Mode
  wifi_mode_t _mode = WiFi.getMode();
  if (_mode != WIFI_MODE_NULL)
  {
    return;
  }

  // Config WiFi Mode
  WiFi.mode(WIFI_AP_STA);

  /*
  // Configuro el Modo WiFi
  WiFi.mode(WIFI_AP_STA);

  String MAC = getMacAddress();
  log_d("My MAC Address is: %s", MAC.c_str());

  // Configuramos el nombre de la red
  MAC.replace(":", "");

  String name_mac = "{NAME}-{MAC}";
  name_mac.replace("{NAME}", "ESP");
  name_mac.replace("{MAC}", MAC);

  // Creamos la red
  WiFi.softAP(name_mac.c_str(), nullptr, 1, true, 4);
  */
}

String AsyncEspNowClass::getMacAddress()
{
  return WiFi.macAddress();
}

void AsyncEspNowClass::_beginEspNow()
{
  _configWifiMode();
  if (esp_now_init() == ESP_OK)
  {
    log_i("ESPNow Init Success");
    String MAC = getMacAddress();
    log_d("My MAC Address is: %s", MAC.c_str());
  }
  else
  {
    log_e("ESPNow Init Failed");
    delay(3000);
    ESP.restart();
  }

  esp_now_register_send_cb(this->sentCallback);
  esp_now_register_recv_cb(this->_receiveCallback);
}

void AsyncEspNowClass::begin()
{
  _beginEspNow();
}

void AsyncEspNowClass::_endEspNow()
{
  if (esp_now_deinit() == ESP_OK)
  {
    log_i("ESPNow Denit Success");
  }
  else
  {
    log_e("ESPNow Denit Failed");
  }
}

void AsyncEspNowClass::end()
{
  _endEspNow();
}

void AsyncEspNowClass::_changeMAC(const uint8_t *customMac)
{
  _configWifiMode();
  esp_err_t esp_status = esp_wifi_set_mac(WIFI_IF_STA, &customMac[0]);

  // Get MAC
  String MAC = getMacAddress();

  switch (esp_status)
  {
  case ESP_OK:
    log_d("Change MAC successful");
    log_d("New MAC Address is: %s", MAC.c_str());
    break;
  case ESP_ERR_WIFI_NOT_INIT:
    log_e("Change MAC Fail: WiFi is not initialized by esp_wifi_init");
    break;
  case ESP_ERR_INVALID_ARG:
    log_e("Change MAC Fail: Invalid argument");
    break;
  case ESP_ERR_WIFI_IF:
    log_e("Change MAC Fail: Invalid interface");
    break;
  case ESP_ERR_WIFI_MAC:
    log_e("Change MAC Fail: Invalid mac address");
    break;
  case ESP_ERR_WIFI_MODE:
    log_e("Change MAC Fail: WiFi mode is wrong");
    break;
  default:
    log_e("Change MAC Fail: UnKnown");
    break;
  }
}

void AsyncEspNowClass::setAddress(uint8_t *customMac)
{
  _changeMAC(customMac);
}

void _uint8copy(uint8_t *mac, const uint8_t *macAddr)
{
  for (int i = 0; i < 6; i++)
  {
    mac[i] = macAddr[i];
  }
}

/*------------------------------------------------------------------------------------------------------*/
/*------------------------------------- CALLBACKS ASYNC ESP  -------------------------------------------*/
/*------------------------------------------------------------------------------------------------------*/
void (*onMessageCallback)(const uint8_t *address, const char *msg);
void AsyncEspNowClass::onMessage(void (*puntero)(const uint8_t *address, const char *msg))
{
  onMessageCallback = puntero;
}

void (*onSendCallback)(const uint8_t *address, bool status);
void AsyncEspNowClass::onSend(void (*puntero)(const uint8_t *address, bool status))
{
  onSendCallback = puntero;
}

/*------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- SEND DATA ---------------------------------------------*/
/*------------------------------------------------------------------------------------------------------*/

SemaphoreHandle_t _sendSemaphore = xSemaphoreCreateMutex();

void AsyncEspNowClass::sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  // Copiamos la MAC
  uint8_t MAC[6];
  _uint8copy(MAC, macAddr);

  // Callback Send
  if (onSendCallback)
    onSendCallback(MAC, status == ESP_NOW_SEND_SUCCESS);
}

void AsyncEspNowClass::send(uint8_t peerAddress[], uint8_t data[])
{
  if (xSemaphoreTake(_sendSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE)
  {
    // Create Peer Info
    esp_now_peer_info_t slaveInfo = {};
    memcpy(slaveInfo.peer_addr, peerAddress, 6);
    slaveInfo.encrypt = 0;

    // Add New Peer
    if (!esp_now_is_peer_exist(slaveInfo.peer_addr))
      esp_now_add_peer(&slaveInfo);

    // Send Result
    esp_err_t result = esp_now_send(slaveInfo.peer_addr, data, sizeof(data));

    if (result == ESP_OK)
    {
      xSemaphoreGive(_sendSemaphore);
      return;
    }
    else
    {
      switch (result)
      {
      case ESP_ERR_ESPNOW_NOT_INIT:
        log_e("Send data Fail: ESPNOW is not initialized");
        break;
      case ESP_ERR_ESPNOW_ARG:
        log_e("Send data Fail: Invalid argument");
        break;
      case ESP_ERR_ESPNOW_FULL:
        log_e("Send data Fail: Peer list is full");
        break;
      case ESP_ERR_ESPNOW_NO_MEM:
        log_e("Send data Fail: Out of memory");
        break;
      case ESP_ERR_ESPNOW_EXIST:
        log_e("Send data Fail: Peer has existed");
        break;
      default:
        log_e("Send data Fail: UnKnown");
        break;
      }
      xSemaphoreGive(_sendSemaphore);
      return;
    }
  }
}

/*------------------------------------------------------------------------------------------------------*/
/*------------------------------------------ SLAVE FUNTION   -------------------------------------------*/
/*------------------------------------------------------------------------------------------------------*/

// Estructura de los mensajes
struct espnow_message
{
  const uint8_t *_address;
  const char *_msg;
};

// Semaphoro
SemaphoreHandle_t _receiveSemaphore = xSemaphoreCreateMutex();

void AsyncEspNowClass::_receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
  if (xSemaphoreTake(_receiveSemaphore, pdMS_TO_TICKS(2000)) == pdTRUE)
  {
    // only allow a maximum of 250 characters in the message + a null terminating byte
    char buffer[ESP_NOW_MAX_DATA_LEN + 1];
    int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);

    strncpy(buffer, (const char *)data, msgLen);
    // make sure we are null terminated
    buffer[msgLen] = 0;

    // format the mac address
    espnow_message msg = {macAddr, buffer};

    xTaskCreatePinnedToCore(
        _task_onMessage, // Function to implement the task
        "onMessage",     // Name of the task
        3500,            // Stack size in words
        (void *)&msg,    // Task input parameter
        3,               // Priority of the task
        NULL,            // Task handle.
        CORE_ESP);       // Core where the task should run

    // one tick delay (10ms) in between reads for stability
    vTaskDelay(pdMS_TO_TICKS(50));

    // Liberamos el semaphore
    xSemaphoreGive(_receiveSemaphore);
  }
}

void AsyncEspNowClass::_task_onMessage(void *pvParameters)
{
  // Get parameters
  espnow_message *_data = (espnow_message *)pvParameters;

  // Copiamos la MAC
  uint8_t MAC[6];
  _uint8copy(MAC, _data->_address);

  // Copiamos el mensaje
  String text = String(_data->_msg);

  if (onMessageCallback)
    onMessageCallback(MAC, text.c_str());
  vTaskDelay(pdMS_TO_TICKS(10)); // one tick delay (10ms) in between reads for stability

  vTaskDelete(NULL);
  Serial.println("Esto no deberia aparecer en el serial");
}

//
