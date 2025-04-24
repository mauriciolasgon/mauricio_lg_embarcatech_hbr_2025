#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "../inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"


const uint I2C_SDA = 14;
const uint I2C_SCL = 15;


typedef struct {
    int v;  // velocidade da bola
    int posicao[3]; //posicaoes x1,x2 e y no display
    uint8_t face; // caractere que representa a bola

}bola;

// Movimenta a bolinha no display de acordo com a velocidade
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

// Função que retorna um número binário aleatório observando o ruido do adc

int binary_choice(){

    uint16_t adc_value = adc_read();
    return (adc_value & 0x01); // Retorna o bit menos significativo do valor lido
}

// Desenha as canaletas na posição central do display 
void draw_canaleta(uint8_t *ssd) {
    const int W = ssd1306_width;
    const int H = ssd1306_height;
    int y = H - 8;              // pixel vertical onde quer a “canaleta”
    int page = y >> 3;          // y/8
    int bit  = y &  0x7;        // y%8
    uint8_t mask = 1 << bit;    

    int row_offset = page * W;
    for (int x = W/4; x < W - W/4; x += 5) {
        ssd[row_offset + x] |= mask;
    }
}


// Desenha os pinos no display oled
void draw_pins(uint8_t *ssd) {
    const int W = ssd1306_width;
    const int H = ssd1306_height;

    for (int y = 1; y < H - 8; y++) {
        int pin_count = y - 1;
        int start_x   = W/2 - pin_count;
        int page      = y >> 3;      // qual página de 8 linhas
        int bit       = y & 0x7;     // qual bit dentro do byte
        uint8_t mask = 1 << bit;
        int base = page * W + start_x;

        // espaçar cada “pino” em 2 colunas
        for (int j = 0; j < pin_count; j++) {
            int idx = base + (j << 1);   // j*2 colunas
            ssd[idx] |= mask;            // OR para não apagar outros pixels
        }
    }
}





int main()
{
    stdio_init_all();


    adc_init();
    adc_select_input(3);


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
    
    draw_canaleta(ssd);

    
    draw_pins(ssd);

    render_on_display(ssd, &frame_area);

    printf("Pins drawn\n");


    while (true) {
        
        printf("Hello, world!\n");
        //move_bola(&b1, ssd, &frame_area);
        printf("Binary choice: %d\n", binary_choice());
        
        sleep_ms(1000);
    }
}
