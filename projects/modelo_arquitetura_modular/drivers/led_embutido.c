#include "pico/cyw43_arch.h"
#include "../include/led_embutido.h"

int init_led(){
    return cyw43_arch_init() ;
}

void aciona_led(){
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

void desliga_led(){
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}