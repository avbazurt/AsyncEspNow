#include "AsyncEspNow.h"


#if CONFIG_IDF_TARGET_ESP32
#define CORE_ESP    0

#elif CONFIG_IDF_TARGET_ESP32S2
#define CORE_ESP    1

#else
#error CORE no disponible para board objetivo!
#endif


//Puntero Callback
void (*punteroCallback)(char MAC[], char text[]);
bool status_send;


//Semaphoro
SemaphoreHandle_t SendDataSemaphore = xSemaphoreCreateCounting( 1, 0 );

//Estructura de los mensajes
struct ESPNOW_mensaje {
  const uint8_t *_address;
  char *msg;
};


AsyncEspNow::AsyncEspNow(String name)
{
  //Guardo el nombre a utilizar
  nameGroup = name;

  WiFi.mode(WIFI_AP_STA);

  String MAC = WiFi.macAddress();
  log_i("My MAC Address is: %s", MAC.c_str());
  status_send = false;

  //Configuramos el nombre de la red
  MAC.replace(":", "");

  String name_mac = "{NAME}-{MAC}";

  name_mac.replace("{NAME}", name);
  name_mac.replace("{MAC}", MAC);

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

  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);

  status_send = status == ESP_NOW_SEND_SUCCESS ? true : false;

  log_i("Last Packet Sent to: %s", macStr);
  log_i("Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  log_i("Se libera el semaphore");
  xSemaphoreGive(SendDataSemaphore);
}


bool AsyncEspNow::sentData(uint8_t peerAddress[], const String &message)
{
  //Copiamos la informacion
  memcpy(_peerAddressSend, peerAddress, 6);
  _message_send = message;

  //Iniciamos la tarea
  log_i("Iniciar RTOS");
  xTaskCreatePinnedToCore(
    this->_task_sendData,        //Function to implement the task
    "httpsTask",            //Name of the task
    5000,                   //Stack size in words
    this,                   //Task input parameter
    2,                      //Priority of the task
    &TaskHandSendData,      //Task handle.
    CORE_ESP);              //Core where the task should run

  log_i("Inicia el semaphore esperando");
  xSemaphoreTake(SendDataSemaphore, portMAX_DELAY);
  log_i("Se termina la tarea");
  return status_send;
}


void AsyncEspNow::_task_sendData(void *pvParameters) {
  //Variable que toma todas las demas variables

  AsyncEspNow *l_pThis = (AsyncEspNow *) pvParameters;


  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, l_pThis->_peerAddressSend, 6);


  if (!esp_now_is_peer_exist(l_pThis->_peerAddressSend))
  {
    esp_now_add_peer(&peerInfo);
  }

  String message = l_pThis->_message_send;

  esp_err_t result = esp_now_send(l_pThis->_peerAddressSend, (const uint8_t *)message.c_str(), message.length());
  vTaskDelay(500);  // one tick delay (15ms) in between reads for stability
  log_i("Se envio Data");

  vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
  vTaskDelete(l_pThis->TaskHandSendData);
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

  ESPNOW_mensaje msg= {macAddr,buffer}; 

  xTaskCreatePinnedToCore(
    _task_reciveData,        //Function to implement the task
    "httpsTask",            //Name of the task
    5000,                   //Stack size in words
    (void *) &msg,                   //Task input parameter
    2,                      //Priority of the task
    NULL,      //Task handle.
    CORE_ESP);              //Core where the task should run



}


void AsyncEspNow::_task_reciveData(void *pvParameters) {
  // debug log the message to the serial port
  ESPNOW_mensaje * mensaje = (ESPNOW_mensaje *) pvParameters;

  //Le damos formato a nuestra MAC
  char macStr[18];
  snprintf(macStr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mensaje->_address[0], mensaje->_address[1], mensaje->_address[2], mensaje->_address[3], mensaje->_address[4], mensaje->_address[5]);

  //Enviamos la informacion all Callback del usuario
  punteroCallback(macStr, mensaje->msg);
  vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
  
  //Anulamos La tarea
  vTaskDelete(NULL);
}


void AsyncEspNow::setReciveCallback(void (*puntero)(char MAC[], char text[]))
{
  punteroCallback = puntero;
}


/*-------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------- SCANER ESP ---------------------------------------------*/
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




// Global copy of slave
#define NUMSLAVES 20
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;

#define CHANNEL 3

// Scan for slaves in AP mode
void AsyncEspNow::ScanForSlave()
{
  //Escaneo los resultados
  int8_t scanResults = WiFi.scanNetworks();

  // reset slaves
  memset(slaves, 0, sizeof(slaves));
  SlaveCnt = 0;

  Serial.println("");
  if (scanResults == 0)
  {
    Serial.println("No WiFi devices in AP Mode found");
  }
  else
  {
    for (int i = 0; i < scanResults; ++i)
    {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);


      // Check if the current device starts with `Slave`
      if (SSID.indexOf("ESP-") == 0)
      {
        // SSID of interest
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" [");
        Serial.print(BSSIDstr);
        Serial.print("]");
        Serial.print(" (");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];

        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
        {
          for (int ii = 0; ii < 6; ++ii)
          {
            slaves[SlaveCnt].peer_addr[ii] = (uint8_t)mac[ii];
          }
        }
        slaves[SlaveCnt].channel = CHANNEL; // pick a channel
        slaves[SlaveCnt].encrypt = 0;       // no encryption
        SlaveCnt++;
      }
    }
  }

  if (SlaveCnt > 0)
  {
    Serial.print(SlaveCnt);
    Serial.println(" Slave(s) found, processing..");
  }
  else
  {
    Serial.println("No Slave Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}
