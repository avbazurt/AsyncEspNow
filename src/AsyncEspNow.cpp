#include "AsyncEspNow.h"
#include "EspConfig.h"

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

void AsyncEspNowClass::_configWifiMode()
{
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
}

String AsyncEspNowClass::getMacAddress()
{
  return WiFi.macAddress();
}

void AsyncEspNowClass::begin()
{
  // Iniciamos el WiFi
  _configWifiMode();

  if (esp_now_init() == ESP_OK)
  {
    log_i("ESPNow Init Success");
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

/*------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- SEND DATA ---------------------------------------------*/
/*------------------------------------------------------------------------------------------------------*/

void AsyncEspNowClass::sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  // MacStr
  char macStr[18];

  // formatMacAddress
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

  status_send = status == ESP_NOW_SEND_SUCCESS ? true : false;

  log_i("Last Packet Sent to: %s", macStr);
  log_i("Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  xSemaphoreGive(SendDataSemaphore);
}

void AsyncEspNowClass::_task_sendData(void *pvParameters)
{
  // Variable que toma la estructura
  NowMessage *NowMessageSend = (NowMessage *)pvParameters;

  if (!esp_now_is_peer_exist(NowMessageSend->now_peer.peer_addr))
  {
    esp_now_add_peer(&NowMessageSend->now_peer);
  }

  esp_err_t result = esp_now_send(NowMessageSend->now_peer.peer_addr, (const uint8_t *)NowMessageSend->message.c_str(), NowMessageSend->message.length());
  vTaskDelay(750); // one tick delay (15ms) in between reads for stability
}

bool AsyncEspNowClass::sentData(NowMessage NowMessageSend)
{
  // Iniciamos la tarea
  xTaskCreatePinnedToCore(
      this->_task_sendData,    // Function to implement the task
      "httpsTask",             // Name of the task
      5000,                    // Stack size in words
      (void *)&NowMessageSend, // Task input parameter
      2,                       // Priority of the task
      &TaskHandSendData,       // Task handle.
      CORE_ESP);               // Core where the task should run

  log_d("Inicia el semaphore esperando");
  xSemaphoreTake(SendDataSemaphore, portMAX_DELAY);
  log_d("Se libera el semaphore");

  // Eliminamos la tarea
  vTaskDelete(TaskHandSendData);
  return status_send;
}

bool AsyncEspNowClass::sendMessage(uint8_t peerAddress[], const String &message)
{
  // Creo una structura
  NowMessage NowMessageSend;

  // Asigno los nuevos valores al Struct
  for (int ii = 0; ii < 6; ++ii)
  {
    NowMessageSend.now_peer.peer_addr[ii] = (uint8_t)peerAddress[ii];
  }
  NowMessageSend.now_peer.encrypt = 0;

  // Copiamos la informacion del mensaje
  NowMessageSend.message = message;

  // Enviamos la data
  bool status_send = sentData(NowMessageSend);

  return status_send;
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
        NULL,           // Task handle.
        CORE_ESP);      // Core where the task should run

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
AsyncEspNowClass AsyncEspNow;