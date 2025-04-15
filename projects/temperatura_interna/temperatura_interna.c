#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"



int main()
{
    stdio_init_all();
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Select the temperature sensor input

    while (true) {
        uint16_t raw_temp = adc_read();
        float ADC_voltage = raw_temp*(3.3f/(1<<12)); // (valor adc*3.3v)/4096
        float temperatura = 27 - (ADC_voltage - 0.706)/0.001721; // formula apresentada no datasheet
        printf("Temperatura: %.2f °C\n", temperatura);
        sleep_ms(1000);
    }
}
