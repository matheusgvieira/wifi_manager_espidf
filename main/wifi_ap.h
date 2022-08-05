#ifndef WEBSERVER_SETUP_WIFI_AP_H
#define WEBSERVER_SETUP_WIFI_AP_H

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "led.h"

// Defines
#define EXAMPLE_ESP_WIFI_SSID      "ESP_TCC"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4

// Struct
typedef struct {
    char * ssid;
    char * password;
} wifi_credentials;

// Functions
void setup_wifi(void);
void get_wifi_credentials(wifi_credentials *credentials);
int reset_wifi_credentials();


#endif //WEBSERVER_SETUP_WIFI_AP_H
