#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "../inc/ssd1306.h"
#include "hardware/i2c.h"


const uint I2C_SDA = 14;
const uint I2C_SCL = 15;


typedef struct {
    int v;  // velocidade da bola
    int posicao[3]; //posicaoes x1,x2 e y no display
    uint8_t face; // caractere que representa a bola

}bola;


void move_bola(bola *b, uint8_t *ssd, struct render_area *area)
{
    static int posicao=0; 
    // Desloca circularmente os bits  referentes a construção da bola de acordo com a velocidade
    if(b->face>>(8-b->v) > 0){
        ssd[b->posicao[0] + (b->posicao[2]/8) * ssd1306_width] = b->face<<b->v;
        ssd[b->posicao[1] + (b->posicao[2]/8) * ssd1306_width] = b->face<<b->v;
        b->face = b->face >> (8-b->v);
        posicao=b->posicao[2];
        b->posicao[2]+=8;
        ssd[b->posicao[0] + (b->posicao[2]/8) * ssd1306_width] = b->face;
        ssd[b->posicao[1] + (b->posicao[2]/8) * ssd1306_width] = b->face;

    }else{
        if(posicao!=0){
            ssd[b->posicao[0] + (posicao/8) * ssd1306_width] = 0x00;
            ssd[b->posicao[1] + (posicao/8) * ssd1306_width] = 0x00;
            posicao=0;
        }
        if(b->face==0x01){
            b->face=b->face<<1;
            b->face=b->face+1;
            
            
        }else{
            b->face=b->face << b->v;
        }
       
        ssd[b->posicao[0] + (b->posicao[2]/8) * ssd1306_width] = b->face;
        ssd[b->posicao[1] + (b->posicao[2]/8) * ssd1306_width] = b->face;
    }
    
    // Verifica se a bola saiu do display
    if (b->posicao[0] >= ssd1306_height) {
        b->posicao[0] = 0;
        b->posicao[1] = 1;
        b->posicao[2] = 0;
    }
    
 
    // Renderiza o display
    render_on_display(ssd, area);


}

int main()
{
    stdio_init_all();


    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    
    bola b1;
    b1.v = 8;
    b1.posicao[0] = 5;
    b1.posicao[1] = b1.posicao[0]+1;
    b1.posicao[2] = 8;
    b1.face = 0x03; // 0xC0 é o caractere de preenchimento do display

    ssd[b1.posicao[0] + (b1.posicao[2]/8) * ssd1306_width] = b1.face;

    ssd[b1.posicao[1] + (b1.posicao[2]/8) * ssd1306_width] = b1.face;
    render_on_display(ssd, &frame_area);


    while (true) {
        
        printf("Hello, world!\n");
        move_bola(&b1, ssd, &frame_area);
        sleep_ms(1000);
    }
}
