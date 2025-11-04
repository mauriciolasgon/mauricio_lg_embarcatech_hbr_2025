#include "../include/led_embutido.h"
#include "../include/hal_led.h"
#include "pico/stdlib.h"


int hal_init_led(){
    if(init_led()){
        return 1;
    }
    return 0;
}

void hal_led_toggle(){
    aciona_led();
    sleep_ms(500);
    desliga_led();
    sleep_ms(500);
}   