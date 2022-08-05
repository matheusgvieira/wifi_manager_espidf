//
// Created by Matheus Gois Vieira on 05/08/22.
//

#ifndef WEBSERVER_SETUP_LED_H
#define WEBSERVER_SETUP_LED_H

#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Structure of sensor
typedef struct {
    gpio_num_t pin;
    char* color;
    int time;
} led_rgb;

void init_led(led_rgb *led);
void set_state_led(led_rgb *led, uint8_t state);
void toggle_led(led_rgb *led, TickType_t time);
void toggle_led_task(void *pvParameters);


#endif //WEBSERVER_SETUP_LED_H
