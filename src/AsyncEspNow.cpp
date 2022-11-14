#include "AsyncEspNow.h"
#include "EspConfig.h"

// Puntero Callback
void (*punteroCallback)(char MAC[], char text[]);
bool status_send;

// Semaphoro

// Estructura de los mensajes
struct ESPNOW_mensaje
{
  const uint8_t *_address;
  char *msg;
};

// Semaphoro
SemaphoreHandle_t SendDataSemaphore = xSemaphoreCreateCounting(1, 0);

AsyncEspNow::AsyncEspNow(String name)
{
  // Guardo el nombre a utilizar
  nameGroup = name;

  WiFi.mode(WIFI_AP_STA);

  String MAC = WiFi.macAddress();
  log_i("My MAC Address is: %s", MAC.c_str());
  status_send = false;

  // Configuramos el nombre de la red
  MAC.replace(":", "");

  String name_mac = "{NAME}-{MAC}";

  name_mac.replace("{NAME}", name);
  name_mac.replace("{MAC}", MAC);

  // Creamos la red
  WiFi.softAP(name_mac.c_str());
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
}

/*------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- SEND DATA ---------------------------------------------*/
/*------------------------------------------------------------------------------------------------------*/

void AsyncEspNow::sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
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

void AsyncEspNow::_task_sendData(void *pvParameters)
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


bool AsyncEspNow::sentData(NowMessage NowMessageSend)
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

  //Eliminamos la tarea
  vTaskDelete(TaskHandSendData);
  return status_send;
}


bool AsyncEspNow::sendMessage(uint8_t peerAddress[], const String &message)
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



/*-------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- RECIVE DATA ---------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/

void AsyncEspNow::receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
  // only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
  // make sure we are null terminated
  buffer[msgLen] = 0;
  // format the mac address

  ESPNOW_mensaje msg = {macAddr, buffer};

  xTaskCreatePinnedToCore(
      _task_reciveData, // Function to implement the task
      "httpsTask",      // Name of the task
      5000,             // Stack size in words
      (void *)&msg,     // Task input parameter
      2,                // Priority of the task
      NULL,             // Task handle.
      CORE_ESP);        // Core where the task should run
}

void AsyncEspNow::_task_reciveData(void *pvParameters)
{
  // debug log the message to the serial port
  ESPNOW_mensaje *mensaje = (ESPNOW_mensaje *)pvParameters;

  // Le damos formato a nuestra MAC
  char macStr[18];
  snprintf(macStr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mensaje->_address[0], mensaje->_address[1], mensaje->_address[2], mensaje->_address[3], mensaje->_address[4], mensaje->_address[5]);

  // Enviamos la informacion all Callback del usuario
  punteroCallback(macStr, mensaje->msg);
  vTaskDelay(100); // one tick delay (15ms) in between reads for stability

  // Anulamos La tarea
  vTaskDelete(NULL);
}

void AsyncEspNow::setReciveCallback(void (*puntero)(char MAC[], char text[]))
{
  punteroCallback = puntero;
}

/*-------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- GENERAL ESP ---------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------*/

void AsyncEspNow::begin()
{
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
  esp_now_register_recv_cb(this->receiveCallback);
}
