#include "wifi_ap.h"

static const char *TAG = "ESP_WIFI_AP";

static httpd_handle_t server = NULL;

led_rgb wifi_ap_led = {.pin = 2, .time = 1000, .color = "blue"};

TaskHandle_t xHandle;

static int save_wifi_credentials(wifi_credentials *credentials) {
    esp_err_t err = nvs_flash_init();
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Updating ssid in NVS ... %s\n", credentials->ssid);
        err = nvs_set_str(my_handle, "ssid", credentials->ssid);

        printf((err != ESP_OK) ? "ssid no update nvs!\n" : "SSID update nvs.\n");

        printf("Updating password in NVS ... %s\n", credentials->password);
        err = nvs_set_str(my_handle, "password", credentials->password);

        printf((err != ESP_OK) ? "password no update nvs!\n" : "PASSWORD update nvs.\n");

        nvs_close(my_handle);

        return 1;
    }

    return -1;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

/* An HTTP GET handler */
static esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = "ESP_AP_OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* An HTTP POST handler */
static esp_err_t post_handler(httpd_req_t *req)
{
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);

    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Body post = %s", content);
    cJSON *body = cJSON_Parse(content);

    char *ssid = cJSON_GetObjectItem(body,"ssid")->valuestring;
    ESP_LOGI(TAG, "ssid=%s",ssid);
    char *password = cJSON_GetObjectItem(body, "password")->valuestring;
    ESP_LOGI(TAG, "password=%s", password);

    wifi_credentials credentials = {.ssid = ssid, .password = password};

    if (save_wifi_credentials(&credentials) == -1) {
        ESP_LOGI(TAG, "Erro ao salvar as credentials do wifi.");
    }

    cJSON_Delete(body);

    /* Send a simple response */
    const char resp[] = "Setup wireless successfully!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    stop_webserver(server);
    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskSuspend( xHandle );

    return ESP_OK;
}


static const httpd_uri_t uri_get = {
        .uri       = "/ap",
        .method    = HTTP_GET,
        .handler   = get_handler,
        .user_ctx  = NULL
};


static const httpd_uri_t uri_post = {
        .uri       = "/config",
        .method    = HTTP_POST,
        .handler   = post_handler,
        .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                MAC2STR(event->mac), event->aid);
    }
}

static void wifi_init_softap(wifi_credentials *credentials)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t * p_netif =esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    esp_netif_set_ip4_addr(&ipInfo.ip, 192,168,1,1);
    esp_netif_set_ip4_addr(&ipInfo.gw, 192,168,1,1);
    esp_netif_set_ip4_addr(&ipInfo.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(p_netif);
    esp_netif_set_ip_info(p_netif, &ipInfo);
    esp_netif_dhcps_start(p_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t ap_config = {
            .ap = {
                    .ssid = EXAMPLE_ESP_WIFI_SSID,
                    .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
                    .channel = EXAMPLE_ESP_WIFI_CHANNEL,
                    .password = EXAMPLE_ESP_WIFI_PASS,
                    .max_connection = EXAMPLE_MAX_STA_CONN,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                    .pmf_cfg = {
                            .required = false,
                    },
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    wifi_config_t sta_config;
    memcpy(sta_config.sta.ssid, (u_int8_t *)credentials->ssid, strlen(credentials->ssid));
    memcpy(sta_config.sta.password, (u_int8_t *)credentials->password, strlen(credentials->ssid));

    ESP_LOGI(TAG, "connecting to ap SSID:%s password:%s",

             sta_config.sta.ssid, sta_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Starting access point, SSID=%s\n", "Test_Transmitter");
    ESP_ERROR_CHECK(esp_wifi_connect()); //AP

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void setup_wifi(wifi_credentials *credentials) {

    // Init led
    init_led(&wifi_ap_led);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap(credentials);

    xTaskCreate(toggle_led_task, "toggle_led_task", 1024*4, &wifi_ap_led, 2, &xHandle);

    /* Start the server for the first time */
    server = start_webserver();
}

void get_wifi_credentials(wifi_credentials *credentials) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // read ssid in nvs
        printf("Reading ssid from NVS ... ");
        size_t required_size_ssid;
        nvs_get_str(my_handle, "ssid", NULL, &required_size_ssid);
        char *ssid = malloc(required_size_ssid);
        err = nvs_get_str(my_handle, "ssid", ssid, &required_size_ssid);

        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Get in nvs ssid = %s\n", ssid);
                credentials -> ssid = ssid;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The ssid is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        // read password in nvs
        printf("Reading password from NVS ... ");
        size_t required_size_password;
        nvs_get_str(my_handle, "password", NULL, &required_size_password);
        char *password = malloc(required_size_password); // value will default to 0, if not set yet in NVS
        err = nvs_get_str(my_handle, "password", password, &required_size_password);

        switch (err) {
            case ESP_OK:
                printf("Get in nvs password = %s\n", password);
                credentials -> password = password;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The password is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }
}

int reset_wifi_credentials() {
    esp_err_t err = nvs_flash_init();
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Erasing wifi credentials... \n");
        err = nvs_erase_key(my_handle, "ssid");
        printf((err != ESP_OK) ? "SSID no erase in nvs!\n" : "SSID erase in nvs!\n");
        err = nvs_erase_key(my_handle, "password");
        printf((err != ESP_OK) ? "PASSWORD no erase in nvs!\n" : "PASSWORD erase in nvs!\n");

        nvs_commit(my_handle);

        nvs_close(my_handle);
        return 1;
    }

    return -1;
}

