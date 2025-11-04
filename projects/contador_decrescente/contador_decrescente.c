
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

#define BOTAO_A 5
#define BOTAO_B 6

// Contador
volatile int contador = 9;
volatile int contadorB=0;

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
int x_b=95;
char contB='0';

uint8_t ssd[ssd1306_buffer_length];

// Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};


int count_dezenas=0;

void botao_callback(uint gpio, uint32_t events)
{
    if(gpio == BOTAO_A){
        contador=9;
        contadorB=0;
        contB='0';
        x_b=95;
        // Zera o display na posição que o contador de B está
        for(int i=0;i<8;i++){
            ssd[(1 * 128 + 95)+i] = 0x00;
            ssd[(1 * 128 + 105)+i] = 0x00;
        }

        render_on_display(ssd, &frame_area);
        count_dezenas=0;

    }
    if(gpio == BOTAO_B){
        if(contador > 0){
            contadorB++;
        }
    }

}

int main()
{
    stdio_init_all();

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &botao_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &botao_callback);

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();


    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);


    
    ssd1306_draw_string(ssd, 5, 0, "Tempo: ");
    ssd1306_draw_string(ssd, 5, 8, "Contador B: ");

    while (true) {

        ssd1306_draw_char(ssd, 55, 0, (char)('0'+contador));
        
        // Organiza dezenas
        if(contadorB>9 ){
            contB++;
            ssd1306_draw_char(ssd, 95, 8, (char)(contB));
            contadorB=0;
            if(count_dezenas==0){
                x_b+=10;
            }
            count_dezenas++;
        }
        ssd1306_draw_char(ssd, x_b, 8, (char)('0'+contadorB));
        render_on_display(ssd, &frame_area);
        if(contador>0){
            contador--; 
        }
        sleep_ms(1000);
    }
}

