idf_component_register(SRCS "hello_world_main.c" "wifi_ap.c" "wifi.c" "led.c" "button.c"
                    INCLUDE_DIRS ""
                    REQUIRES "driver" "json" "mqtt" "spiffs" "esp_http_server" "esp_eth"
                    PRIV_REQUIRES nvs_flash)
