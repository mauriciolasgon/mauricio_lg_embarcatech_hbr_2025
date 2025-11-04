

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
 
 int main() {
    // Inicializa as interfaces de entrada e saída padrão (UART)
    stdio_init_all();
    // Inicializa o conversor analógico-digital (ADC)
    adc_init();
    
    // Configura os pinos GPIO 26 e 27 como entradas de ADC (alta impedância, sem resistores pull-up)
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicia o loop infinito para leitura e exibição dos valores do joystick
    while (1) {
        // Seleciona a entrada ADC 0 (conectada ao eixo X do joystick)
        adc_select_input(1);
        // Lê o valor do ADC para o eixo X
        uint adc_x_raw = adc_read();
        
        // Seleciona a entrada ADC 1 (conectada ao eixo Y do joystick)
        adc_select_input(0);
        // Lê o valor do ADC para o eixo Y
        uint adc_y_raw = adc_read();

        printf("X: %d, Y: %d\n", adc_x_raw, adc_y_raw);

        
        // Pausa o programa por 50 milissegundos antes de ler novamente
        sleep_ms(50);
    }
 }