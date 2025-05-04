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

#define LED_PIN 12 


typedef struct {
    int v;  // velocidade da bola
    int posicao[3]; //posicaoes x1,x2 e y no display
    uint8_t face; // caractere que representa a bola
    int canaleta;
    int registrada;

}bola;
// Array que armazena a posição das canaletas sendo que as duas primeiras posições
// são a primeira haste e a segunda haste da canaleta 1 e assim por diante
int *posicao_canaletas=NULL; 
int *canaletas=NULL; // Array que armazena a quantidade de bolas em cada canaleta sendo que a primeira posição é a canaleta 1 e assim por diante
int total_hastes=0;



int binary_choice(); // Função que retorna um número binário aleatório observando o ruido do adc
int colisao(bola *b, uint8_t *ssd); // Função que detecta a colisão da bolinha com os pinos
void move_bola_x(bola *b,uint8_t *ssd, int direction); // Função que movimenta a bolinha horizontalmente
void move_bola_y(bola *b, uint8_t *ssd); // Função que movimenta a bolinha no display de acordo com a velocidade
void draw_canaleta(uint8_t *ssd); // Função que desenha as canaletas na posição central do display
void draw_pins(uint8_t *ssd); // Função que desenha os pinos no display oled


void blink(){
    gpio_put(LED_PIN, true);
    sleep_ms(250);  // Espera 250ms

    // Desliga o LED
    gpio_put(LED_PIN, false);
    sleep_ms(250);  // Espera 250ms
}


void registra_bola_canaleta(bola *b,uint8_t *ssd)
{
    static int *byte_somado=NULL; // Registra o byte a ser somado de cada canaleta

    if(byte_somado==NULL){
        byte_somado=(int*)malloc((total_hastes-1)*sizeof(int));
        for(int i=0;i<total_hastes-1;i++){
            if(posicao_canaletas[i] > ssd1306_width/2){
                byte_somado[i]=posicao_canaletas[i];
            }
            else{
                byte_somado[i]=posicao_canaletas[i+1];
            }
            
        }
    }

    if(posicao_canaletas[b->canaleta] > ssd1306_width/2){
        byte_somado[b->canaleta]++;
        if(byte_somado[b->canaleta]>= posicao_canaletas[b->canaleta+1]){ // Reinicia a linha ao atingir a outra haste
            byte_somado[b->canaleta]=posicao_canaletas[b->canaleta];
        }
        ssd[byte_somado[b->canaleta] + (b->posicao[2]/8) * ssd1306_width] = 
        ssd[byte_somado[b->canaleta] + (b->posicao[2]/8) * ssd1306_width] >> 1 | 0x80;
        

    }
    else{
        byte_somado[b->canaleta]--;
        if(byte_somado[b->canaleta]<= posicao_canaletas[b->canaleta]){ // Reinicia a linha ao atingir a outra haste
            byte_somado[b->canaleta]=posicao_canaletas[b->canaleta+1];
        }
        ssd[byte_somado[b->canaleta] + (b->posicao[2]/8) * ssd1306_width] = 
        ssd[byte_somado[b->canaleta] + (b->posicao[2]/8) * ssd1306_width]>> 1 | 0x80;
    }

    b->registrada=1; // Marca a bolinha como registrada

}

//Verifica qual canaleta a bola caiu e incrementa o contador
void verifica_canaleta(bola *b){
    for(int i=0; i<total_hastes-1;i++){
        if(b->posicao[0] >posicao_canaletas[i] && b->posicao[1] < posicao_canaletas[i+1]){
            canaletas[i]++;
            b->canaleta=i;
            break;
        }
    }
}


// Deteca a colisão da bolinha com os pinos sendo que a detecção é 1 
// Se não houver colisão, retorna 0
int colisao(bola *b, uint8_t *ssd)
{
    const int W = ssd1306_width;
    if(ssd[b->posicao[0] + (b->posicao[2]/8) * W] & b->face<<b->v //Verifica se colisão na página atual
        || ssd[b->posicao[1] + (b->posicao[2]/8) * W] & b->face<<b->v
        || ssd[b->posicao[0] + ((b->posicao[2]+8)/8) * W] &  b->face>>(8-b->v) // Verifica se colisão na página seguinte
        || ssd[b->posicao[1] + ((b->posicao[2]+8)/8) * W] &  b->face>>(8-b->v)){
           
            return 1; // colisão detectada
    }
    return 0; // sem colisão
}

//Movimenta a bolinha horizontalmente
void move_bola_x(bola *b, uint8_t *ssd ,int direction)
{

    
    static int dir=1; // a bola comeca a esquerda do primeiro pino
    int pos_l=0,pos_r=0;
    uint8_t mask0 =ssd[b->posicao[0] + (b->posicao[2]/8) * ssd1306_width];
    uint8_t mask1 =ssd[b->posicao[1] + (b->posicao[2]/8) * ssd1306_width];
    ssd[b->posicao[0] + (b->posicao[2]/8) * ssd1306_width]= mask0 & ~ b->face ; // Apaga apenas a bolinha na posição anterior preservando os pinos
    ssd[b->posicao[1] + (b->posicao[2]/8) * ssd1306_width]= mask1 & ~ b->face ;
    if(dir== 1){// verifica se a bolinha colidiu pelo lado esquerdo e direito e adpta a nova posicao da bolinha
        pos_l=2;
        pos_r=4;
    }else{
        pos_l=4;
        pos_r=2;
    }
    if(direction){// Se for esquerda
        b->posicao[0]-=pos_l;
        b->posicao[1]= b->posicao[0]-1;
        dir=0;
    }else{ // Se for direita
        b->posicao[0]+=pos_r;
        b->posicao[1]= b->posicao[0]+1;
        dir=1;
    } 
}

// Movimenta a bolinha no display de acordo com a velocidade
void move_bola_y(bola *b, uint8_t *ssd)
{
    static int posicao=0;
    int bits1_pagina_atual=b->posicao[0] + (b->posicao[2]/8) * ssd1306_width; //Cálculo para encontrar o byte da pagina atual
    int bits2_pagina_atual=b->posicao[1] + (b->posicao[2]/8) * ssd1306_width;
    int bits1_pagina_futura=b->posicao[0] + ((b->posicao[2]+8)/8) * ssd1306_width; // Cálculo para encontrar o byte da proxima pagina
    int bits2_pagina_futura=b->posicao[1] + ((b->posicao[2]+8)/8) * ssd1306_width;

     
    uint8_t pag1_atual = ssd[bits1_pagina_atual], pag2_atual = ssd[bits2_pagina_atual]; 
    uint8_t pag1_futura = ssd[bits1_pagina_futura], pag2_futura = ssd[bits2_pagina_futura];

    uint8_t mask1_pagina_sem_bola_atual= pag1_atual & ~b->face; //Mascara para apagar a bola na pagina atual
    uint8_t mask2_pagina_sem_bola_atual= pag2_atual & ~b->face;
    uint8_t mask1_pagina_sem_bola_futura= pag1_futura & ~b->face; //Mascara para apagar a bola na pagina futura
    uint8_t mask2_pagina_sem_bola_futura= pag2_futura & ~b->face;
    

    // Desloca circularmente os bits  referentes a construção da bola de acordo com a velocidade
    if(b->face>>(8-b->v) > 0){// Verifica se a bolinha mudou de página
        ssd[bits1_pagina_atual] =  mask1_pagina_sem_bola_atual | b->face<<b->v;
        ssd[bits2_pagina_atual] =  mask2_pagina_sem_bola_atual | b->face<<b->v;
        b->face = b->face >> (8-b->v);
        posicao=b->posicao[2]; // guarda a posição da ultima página 
        b->posicao[2]+=8;
        ssd[bits1_pagina_futura] =  mask1_pagina_sem_bola_futura | b->face;
        ssd[bits2_pagina_futura] =  mask2_pagina_sem_bola_futura | b->face;
                
    }else{
        if(posicao!=0){ // Verifica se o pixel da bolinha já foi apagado na pagina anterior
            uint8_t mask0 =ssd[b->posicao[0] + (posicao/8) * ssd1306_width];
            uint8_t mask1 =ssd[b->posicao[1] + (posicao/8) * ssd1306_width];
            ssd[b->posicao[0] + (posicao/8) * ssd1306_width] = mask0 & ~ (b->face<<(8-b->v)) |  mask0 & (b->face<<(8-b->v)); // apaga os pixels da bola na pagina anterior sem apagar os pinos
            ssd[b->posicao[1] + (posicao/8) * ssd1306_width] = mask1 & ~ (b->face<<(8-b->v)) |  mask1 & (b->face<<(8-b->v));
            posicao=0;
            
            
        }
        if(b->face==0x01){// ajusta a face da bolinha para continuar com 2 casas
            b->face=b->face<<1;
            b->face=b->face+1;
            
            
        }else{
            b->face=b->face << b->v; //Deslocamento da bolinha com base na velocidade
        }
    
        ssd[bits1_pagina_atual] = mask1_pagina_sem_bola_atual | b->face;
        ssd[bits2_pagina_atual] = mask2_pagina_sem_bola_atual | b->face;
    }



    // Verifica se a bola atingiu as canaletas
    if (b->posicao[2] >= ssd1306_height-8) {
        verifica_canaleta(b);
        b->v=0; // Para a bolinha 
        ssd[bits1_pagina_futura] = 0x00; // Apaga a bolinha na proxima pagina
        ssd[bits2_pagina_futura] = 0x00;       
    }
    
    


}

// Função que retorna um número binário aleatório observando o ruido do adc
//Retorna 1 para esquerda e 0 para direita
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

    uint8_t mask = 0xFF; // máscara para preencher a linha inteira
    posicao_canaletas=(int* )malloc(sizeof(int));

    int row_offset = page * W;
    int index=0;
    for (int x = W/4 +4 ; x < W - W/4; x += 8) {
        posicao_canaletas=realloc(posicao_canaletas, sizeof(int)*(index+1)); // realoca o espaço para armazenar novas hastes
        ssd[row_offset + x] = mask;
        posicao_canaletas[index++]=x; // armazena a posição da canaleta
       
    }
    total_hastes=index;
    canaletas=(int*)malloc(sizeof(int)*(total_hastes-1)); // aloca o espaço para armazenar as canaletas
}


// Desenha os pinos no display oled
void draw_pins(uint8_t *ssd) {
    const int W = ssd1306_width;
    const int H = ssd1306_height;
    int pin_count = 0;
    int start_x=W/2; 
    for (int y = 8; y < H - 8; y+=8) {

        int page      = y >> 3;      // qual página de 8 linhas
        int bit       = y & 0x7;     // qual bit dentro do byte
        uint8_t mask = 1 << bit;
        int base = page * W + start_x;
        
        pin_count++;
        // espaçar cada “pino” em 2 colunas
        for (int j = 0; j < pin_count; j++) {
            int idx = base + (j << 3 );   // j*2 colunas
            ssd[idx] |= mask;            // OR para não apagar outros pixels
        }
        start_x-=4;
       
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

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    

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
    
    bola b;
    b.v = 2;
    b.posicao[0] = (ssd1306_width/2) -1; // Posição inicial da bolinha
    b.posicao[1] = b.posicao[0]+1;
    b.posicao[2] = 0;
    b.face = 0x03; //preenchimento dos dois primeiros bits de cada coluna
    b.canaleta=0;
    b.registrada=0;

    ssd[b.posicao[0] + (b.posicao[2]/8) * ssd1306_width] |= b.face;

    ssd[b.posicao[1] + (b.posicao[2]/8) * ssd1306_width] |= b.face;
    
    draw_canaleta(ssd);
    draw_pins(ssd);

    render_on_display(ssd, &frame_area);
    int num_bolas=10, bola_atual=0;;
    bola bolas[num_bolas]; // Array que armazena as bolinhas
    

    for(int i=0;i<num_bolas;i++){
        bolas[i]=b;
    }

    int espera_bola=0,i=0;
    while (true) {
    
    if(espera_bola==10){
        bola_atual++;
        i=bola_atual%num_bolas;
        espera_bola=0;
    }
    
       if(bolas[i].registrada==0){
        if(bolas[i].v==0){
            if(bolas[i].registrada==0){ // Verifica se a bolinha já foi registrada
                registra_bola_canaleta(&bolas[i],ssd); // Verifica se a bolinha caiu na canaleta
               
            }

              
        }else{
            if(colisao(&bolas[i], ssd)){ // Verifica se a bolinha colidiu com os pinos
                move_bola_x(&bolas[i], ssd, binary_choice());
            }else{
                move_bola_y(&bolas[i], ssd);
                
            }
        }
    }
    espera_bola++;
    // Renderiza o display
    render_on_display(ssd, &frame_area);
    }
   
}
