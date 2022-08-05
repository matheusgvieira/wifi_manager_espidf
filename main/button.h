//
// Created by Matheus Gois Vieira on 05/08/22.
//

#ifndef WEBSERVER_SETUP_BUTTON_H
#define WEBSERVER_SETUP_BUTTON_H

#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Structure of sensor
typedef struct {
    gpio_num_t pin;
    int state;
} buttons;

void init_button(buttons *button);
int8_t get_state(buttons *button);
#endif //WEBSERVER_SETUP_BUTTON_H
