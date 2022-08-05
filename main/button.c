#include "button.h"

void init_button(buttons *button) {
    gpio_set_direction(button->pin, GPIO_MODE_INPUT);
};

int8_t get_state(buttons *button){

    button->state = gpio_get_level(button->pin);

    return button->state;
};
