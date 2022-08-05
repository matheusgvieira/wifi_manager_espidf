#include <stdio.h>
#include "led.h"

static const char *TAG = "LED OCCHI";

void set_state_led(led_rgb *led, uint8_t state)
{
    gpio_set_level(led->pin, state); /* Set the GPIO level according to the state (LOW or HIGH)*/
}

void init_led(led_rgb *led)
{
    ESP_LOGI(TAG, "Configured GPIO LED %d %s!", led -> pin , led -> color );

    gpio_reset_pin(led -> pin);
    gpio_set_direction(led -> pin, GPIO_MODE_OUTPUT); /* Set the GPIO as a push/pull output */
}

void toggle_led(led_rgb *led, TickType_t time) {
    gpio_set_level(led->pin, 1);
    vTaskDelay(time / portTICK_PERIOD_MS);
    gpio_set_level(led->pin, 0);
    vTaskDelay(time / portTICK_PERIOD_MS);
}

void toggle_led_task(void *pvParameters) {
    led_rgb led = *(led_rgb *) pvParameters;

    while(1) {
        gpio_set_level(led.pin, 1);
        vTaskDelay(led.time / portTICK_PERIOD_MS);
        gpio_set_level(led.pin, 0);
        vTaskDelay(led.time / portTICK_PERIOD_MS);
    }
}