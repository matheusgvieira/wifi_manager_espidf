#ifndef WEBSERVER_SETUP_WIFI_H
#define WEBSERVER_SETUP_WIFI_H


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_ap.h"

// Defines
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
//#define SSID        CONFIG_ESP_WIFI_SSID
//#define PASSWORD    CONFIG_ESP_WIFI_PASSWORD
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
// Variables


extern TaskHandle_t taskHandle;
extern const uint32_t WIFI_CONNEECTED;

// Functions
int8_t wifi_init(wifi_credentials *credentials);


#endif //WEBSERVER_SETUP_WIFI_H
