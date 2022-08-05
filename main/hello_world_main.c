#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_ap.h"
#include "wifi.h"
#include "button.h"
#include "driver/gpio.h"

buttons button_reset_wifi = {.pin = GPIO_NUM_32, .state = 1};

#define TAG     "WIFI_CONNECT"

void app_main(void)
{

    init_button(&button_reset_wifi);
    wifi_credentials credentials = {.ssid = "", .password = ""};

    get_wifi_credentials(&credentials);
    ESP_LOGI(TAG, "ssid = %s\n", credentials.ssid);
    ESP_LOGI(TAG, "password = %s\n", credentials.password);

    if (strlen(credentials.ssid) <= 0 || strlen(credentials.password) <= 0) {
        setup_wifi();
    } else {
        int8_t is_connect_wifi = wifi_init(&credentials);

        if (is_connect_wifi) {
            while(1) {
                get_state(&button_reset_wifi);
                if (button_reset_wifi.state == 0) {
                    reset_wifi_credentials();
                }
                printf(" Connected!!!!!\n ");
            }
        }

        while(1) {
            get_state(&button_reset_wifi);
            if (button_reset_wifi.state == 0) {
                reset_wifi_credentials();
            }
            printf(" Not connected!!!!!\n ");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }





}