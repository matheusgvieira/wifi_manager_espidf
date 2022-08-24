#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_ap.h"
#include "button.h"
#include "driver/gpio.h"

buttons button_reset_wifi = {.pin = 32, .state = 1};

#define TAG     "WIFI_CONNECT"

void app_main(void)
{

    init_button(&button_reset_wifi);
    wifi_credentials credentials = {.ssid = "", .password = ""};

    get_wifi_credentials(&credentials);

    ESP_LOGI(TAG, "ssid = %s\n", credentials.ssid);
    ESP_LOGI(TAG, "password = %s\n", credentials.password);

    if (check_credentials(&credentials)) {
        setup_wifi();
    } else {
        if(wifi_connect_sta(&credentials)) {
            while(1) {
                get_state(&button_reset_wifi);
                if (button_reset_wifi.state == 1) {
                    reset_wifi_credentials();
                }

                printf("Button state => %d\n", button_reset_wifi.state);
            }
        }



    }




}