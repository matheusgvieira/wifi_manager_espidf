#include "wifi_ap.h"

led_rgb wifi_led = {.pin = 2, .time = 1000, .color = "blue"};

static const char *TAG = "ESP_WIFI";

static int s_retry_num = 0;
const int CONNECTED_BIT = BIT0;

TaskHandle_t xHandle;
static httpd_handle_t server = NULL;
static EventGroupHandle_t wifi_event_group;


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

void get_wifi_credentials(wifi_credentials *credentials) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... \n");
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Reading ssid from NVS ... \n");

        size_t required_size_ssid;
        nvs_get_str(nvs_handle, "ssid", NULL, &required_size_ssid);
        char *ssid = malloc(required_size_ssid);
        err = nvs_get_str(nvs_handle, "ssid", ssid, &required_size_ssid);

        switch (err) {
            case ESP_OK:
                printf("Get in nvs ssid = %s\n", ssid);
                credentials -> ssid = ssid;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The ssid is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        printf("Reading password from NVS ... \n");

        size_t required_size_password;
        nvs_get_str(nvs_handle, "password", NULL, &required_size_password);
        char *password = malloc(required_size_password);
        err = nvs_get_str(nvs_handle, "password", password, &required_size_password);

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

        nvs_close(nvs_handle);
    }
}

int reset_wifi_credentials() {
    esp_err_t err = nvs_flash_init();
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Erasing wifi credentials... \n");

        err = nvs_erase_key(nvs_handle, "ssid");
        printf((err != ESP_OK) ? "SSID no erase in nvs!\n" : "SSID erase in nvs!\n");

        err = nvs_erase_key(nvs_handle, "password");
        printf((err != ESP_OK) ? "PASSWORD no erase in nvs!\n" : "PASSWORD erase in nvs!\n");

        nvs_commit(nvs_handle);

        nvs_close(nvs_handle);
        return 1;
    }

    return -1;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    return httpd_stop(server);
}

static esp_err_t get_handler(httpd_req_t *req)
{
    const char resp[] = "ESP_AP_OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t post_handler(httpd_req_t *req)
{
    char content[100];

    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
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
        return ESP_FAIL;
    }

    cJSON_Delete(body);

    const char resp[] = "Setup wireless successfully!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    stop_webserver(server);
    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskSuspend( xHandle );
    esp_restart();

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
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    return server;
}

static void wifi_event_ap_handler(void* arg, esp_event_base_t event_base,
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

static void wifi_ap()
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
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_ap_handler,
                                                        NULL,
                                                        NULL));



    wifi_config_t ap_config = {
            .ap = {
                    .ssid = CONFIG_AP_WIFI_SSID,
                    .ssid_len = strlen(CONFIG_AP_WIFI_SSID),
                    .channel = EXAMPLE_ESP_WIFI_CHANNEL,
                    .password = CONFIG_AP_WIFI_PASSWORD,
                    .max_connection = EXAMPLE_MAX_STA_CONN,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                    .pmf_cfg = {
                            .required = false,
                    },
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "WIFI_MODE_AP started. SSID:%s password:%s channel:%d",
             CONFIG_AP_WIFI_SSID, CONFIG_AP_WIFI_PASSWORD, CONFIG_AP_WIFI_CHANNEL);
}

static void wifi_event_sta_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

int8_t wifi_connect_sta(wifi_credentials *credentials) {
    // Init led
    init_led(&wifi_led);

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_sta_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_sta_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t sta_config = { 0 };
    strcpy((char *)sta_config.sta.ssid, credentials->ssid);
    strcpy((char *)sta_config.sta.password, credentials->password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config) );

    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");


    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 credentials->ssid, credentials->password);
        set_state_led(&wifi_led, 1);
        return 1;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 credentials->ssid, credentials->password);
        set_state_led(&wifi_led, 0);
        return 0;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    return 0;
}

bool check_credentials(wifi_credentials *credentials) {
    return (strlen(credentials->ssid) <= 0 || strlen(credentials->password) <= 0) ? true : false;
}

void setup_wifi() {

    init_led(&wifi_led);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_APSTA");
    wifi_ap();

    xTaskCreate(toggle_led_task, "toggle_led_task", 1024*4, &wifi_led, 2, &xHandle);

    server = start_webserver();

}






