#include "pico/stdlib.h"
#include "../include/hal_led.h"



int main() {
    if(hal_init_led()){
        return -1;
    }

    while (true) {
        hal_led_toggle();
    }
}