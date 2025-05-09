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

#define BOTAO_A 5


typedef struct {
    int v;  // velocidade da bola
    int posicao[3]; //posicaoes x1,x2 e y no display
    uint8_t face; // caractere que representa a bola
    int canaleta;
    int registrada;
    int dir_colisao; // Direção da colisão
    int pag_anterior;
    int troca_pagina;
    int ativa;

}bola;

typedef struct {
    int coluna_inicial;
    int coluna_final;
    int coluna_somada;
    int *bit_alterado;
    uint8_t bolas;
}canaleta;

int total_canaletas=0;
canaleta *canaletass=NULL; // Array que armazena as canaletas
int tela_distribuicao=0;


int binary_choice(); // Função que retorna um número binário aleatório observando o ruido do adc
int colisao(bola *b, uint8_t *ssd); // Função que detecta a colisão da bolinha com os pinos
void move_bola_x(bola *b,uint8_t *ssd, int direction); // Função que movimenta a bolinha horizontalmente
void move_bola_y(bola *b, uint8_t *ssd); // Função que movimenta a bolinha no display de acordo com a velocidade
void draw_canaleta(uint8_t *ssd); // Função que desenha as canaletas na posição central do display
void draw_pins(uint8_t *ssd); // Função que desenha os pinos no display oled a cada página
void inicializar_bolas(bola * bolas,int * posicao_inicial,int num_bolas); // Inicializa a estrutura da bola padrão
void ssd1306_draw_small_number(uint8_t *ssd_buffer, int x, int y, uint8_t number); //Desenha um numero em uma escala menor
void ssd1306_draw_small_number_sequence(uint8_t *ssd_buffer, int x, int y, int number); // Desenha uma sequencia de numeros em menor escala
void exibir_contagem(uint8_t *ssd, int x ,int y, int bolas); //Exibe a contagem de bolas em cada canaleta
void reset(bola *bolas,int num_bolas, uint8_t *ssd,int *posicao_inicial); //Reinicia o sistema
void exibir_bola_canaleta(canaleta *canaleta, uint8_t *ssd, int posicao_y); // Acende um pixel representando uma bola ao atingir a canaleta
void atualizar_coluna_bit_alterado(canaleta *canaleta, int *posicao_y_inical); //Atualiza campo necessário para gerenciar a exibição de bolas na canaleta
void exibir_distribuicao(uint8_t *ssd, struct render_area *frame_area); // Exibe o total de bolas em cada canaleta e a distribuição completa 
void registra_bola_canaleta(bola *b,uint8_t *ssd); // Registra que uma bola atingiu a canaleta e acende um pixel dentro da canaleta
void verifica_canaleta(bola *b); //Verifica qual canaleta a bola atingiu e incrementa a quantidade de bolas naquela canaleta
void apagar_bola(uint8_t *ssd, int x0, int x1, int y, uint8_t face); // apaga a bola ao se movimentar para os lados ou na troca de pagina
void ajusta_deslocamento_x(int dir_colisao, int *pos_l, int *pos_r); // Organiza o deslocamento horizontal para que a bola atinja o máximo de pinos
void desloca_x(bola *b, int pos_l, int pos_r, int direction); // Movimenta em x de acordo com o deslocamento acima
void calcula_endereco_pagina(bola *b,int *addr1,int *addr2, int*addr1_futuro, int* addr2_futuro); // Calulo dos bytes necessários para o deslocamento em y
void gerar_mascaras(uint8_t *pag_atual1, uint8_t *pag_atual2, uint8_t *pag_futura1, uint8_t *pag_futura2, // Máscaras dos bytes analisados sem a bola presente
    bola *b, uint8_t *mask_atual1, uint8_t *mask_atual2, uint8_t *mask_futura1, uint8_t *mask_futura2);
void deslocar_bola(bola * b); // Movimenta a bola de acordo com sua velocidade
void desenhar_haste_canaleta(uint8_t *ssd, int coluna, int page); // Acende todos bits da haste da canaleta
int calcular_linha_canaleta(); // Calcula a pagina que as canaletas estarão, no caso a última
canaleta inicializar_canaleta(int x, int W);
uint8_t gerar_mask_vertical(int y);  //Calcula qual bit vai ser acesso para representar o pino de acordo com y
int calcular_page_offset(int y); //Calcula a pagina para preenchimentos de pinos
void atualizar_bolinha(bola *b, uint8_t *ssd, struct render_area *frame_area, int *primeira_bola);  // Atualiza o movimento da bolinho ou registra dentro da canaleta 
bool funil(bola *b, uint8_t *ssd, int *posicao_inicial); // Verifição se as posições inicias estão livres para receber novas bolas
//Trata a interação com o botão 
bool tratar_botao(uint8_t *ssd, struct render_area *frame_area, int *tela_distribuicao, bola *bolas, int num_bolas, int *primeira_bola, int *bola_atual,int *posicao_inicial);

const uint8_t numeros_bitmap[10][3] = {
    {0b11111, 0b10001, 0b11111}, // 0
    {0b00000, 0b00000, 0b11111}, // 1
    {0b11001, 0b10101, 0b10011}, // 2
    {0b10001, 0b10101, 0b11111}, // 3
    {0b00111, 0b00100, 0b11111}, // 4
    {0b10011, 0b10101, 0b11101}, // 5
    {0b11111, 0b10101, 0b11101}, // 6
    {0b00001, 0b00001, 0b11111}, // 7
    {0b11111, 0b10101, 0b11111}, // 8
    {0b10011, 0b10101, 0b11111}  // 9
};


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


    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    

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

    int posicao_inicial[3]={(ssd1306_width/2)-1,ssd1306_width/2, 0}; // Posição inicial da bolinha



    
    draw_canaleta(ssd);
    draw_pins(ssd);

    render_on_display(ssd, &frame_area);
    const int num_bolas=500;
    int bola_atual=0;
    bola bolas[num_bolas];
    inicializar_bolas(bolas,posicao_inicial,num_bolas);
    

    int primeira_bola=0;

    

    while (true) {
        
        if (primeira_bola == num_bolas) {
            if (tratar_botao(ssd, &frame_area, &tela_distribuicao, bolas, num_bolas, &primeira_bola, &bola_atual,posicao_inicial))
                continue;
            continue;
        }

        int i=bola_atual%num_bolas; 

        if(bolas[i].registrada==1){ // Verifica se a bolinha já foi registrada e pula para a proxima
            bola_atual++;
            continue;   
        }
        
        if (funil(&bolas[i], ssd, posicao_inicial)) {
            i = primeira_bola;
            bola_atual=primeira_bola;
        }
        
        atualizar_bolinha(&bolas[i], ssd, &frame_area, &primeira_bola);
   
        bola_atual++;
        
        if(num_bolas<=20){
            render_on_display(ssd, &frame_area);
        }
    
    }
   
}




// Função para desenhar um número pequeno (3x5) no display
void ssd1306_draw_small_number(uint8_t *ssd_buffer, int x, int y, uint8_t number) {
    if (number > 9) return; // Apenas 0 a 9

    for (int col = 0; col < 3; col++) {
        uint8_t column_bits = numeros_bitmap[number][col];
        for (int row = 0; row < 5; row++) {
            if (column_bits & (1 << row)) {
                ssd1306_set_pixel(ssd_buffer, x + col, y + row, true);
            }
        }
    }
}

void ssd1306_draw_small_number_sequence(uint8_t *ssd_buffer, int x, int y, int number) {
    if (number < 0) number = -number; // Ignora sinal negativo (ou pode desenhar um '-' se preferir)

    // Calcula quantos dígitos o número tem
    int temp = number;
    int digits = 1;
    while (temp >= 10) {
        temp /= 10;
        digits++;
    }

    // Determina o fator de deslocamento horizontal (3 pixels por dígito + 1 pixel de espaço)
    int digit_width = 4; 

    // Ajusta a posição inicial para alinhar à direita, se necessário
    x += digit_width * (digits - 1);

    // Desenha cada dígito, começando do menos significativo
    while (digits--) {
        int current_digit = number % 10;
        ssd1306_draw_small_number(ssd_buffer, x, y, current_digit);
        number /= 10;
        x -= digit_width; // Move para a esquerda para o próximo dígito
    }
}


void exibir_contagem(uint8_t *ssd, int x ,int y, int bolas){
    ssd1306_draw_small_number_sequence(ssd, x, y, bolas);
}

void reset(bola *bolas,int num_bolas, uint8_t *ssd,int *posicao_inicial){
    free(canaletass);

    canaletass=NULL;
    total_canaletas=0;
    tela_distribuicao=0;
    inicializar_bolas(bolas,posicao_inicial,num_bolas);
    draw_canaleta(ssd);
    draw_pins(ssd);
}



void exibir_bola_canaleta(canaleta *canaleta, uint8_t *ssd, int posicao_y){
    int posicao_x= canaleta->coluna_somada;
    int coluna_canaleta = posicao_x - canaleta->coluna_inicial; 
    int bit_alterado = canaleta->bit_alterado[coluna_canaleta] & 0x07;
    
    ssd[posicao_x + (posicao_y/8) * ssd1306_width] |= ( 0x80 >> (bit_alterado)); // Incrementa o bit alterado
    canaleta->bit_alterado[coluna_canaleta]++; // Incrementa o bit alterado
}

void atualizar_coluna_bit_alterado(canaleta *canaleta, int *posicao_y_inical){
    if(canaleta->coluna_inicial > ssd1306_width/2  ){ //Verifica se a canaleta esta a direita ou a esquerda do display ou se esta no centro
        canaleta->coluna_somada++;
         if(canaleta->coluna_somada >= canaleta->coluna_final){ // Reinicia a linha ao atingir a outra haste
             canaleta->coluna_somada=canaleta->coluna_inicial+1;
             if(posicao_y_inical!=NULL){ // Verifica se a posição y inicial é diferente de NULL
                (*posicao_y_inical)--;
             }
             
         }
     }
     else{
        canaleta->coluna_somada--;
         if(canaleta->coluna_somada<=canaleta->coluna_inicial){ // Reinicia a linha ao atingir a outra haste
             canaleta->coluna_somada=canaleta->coluna_final-1;
             if(posicao_y_inical!=NULL){ // Verifica se a posição y inicial é diferente de NULL
                (*posicao_y_inical)--;
             }
         }

     }
}

void exibir_distribuicao(uint8_t *ssd, struct render_area *frame_area) {
    
    const int H = ssd1306_height;
    
    int x=0;
    
    for(int c=0; c<total_canaletas; c++){
        canaleta *canaleta=&canaletass[c];
        int coluna_canaleta;
        int num_bolas=canaleta->bolas;
        int posicao_y_inical = H; // Posição inicial da canaleta
        int troca_pagina=posicao_y_inical/8; // Verifica se a bolinha trocou de pagina
        
        if(canaleta->coluna_inicial > ssd1306_width/2 ){ //Atualiza a posicao inicial da coluna a ser somada
            canaleta->coluna_somada=canaleta->coluna_final; 
        }else{
            canaleta->coluna_somada=canaleta->coluna_inicial; 
        }
        
       memset(canaleta->bit_alterado,0,sizeof(int)*(canaleta->coluna_final-canaleta->coluna_inicial)); // Zera o array

       exibir_contagem(ssd, x, 2, canaleta->bolas); // Exibe a contagem de bolinhas
       x+=17;
       render_on_display(ssd, frame_area); // Renderiza o display
       sleep_ms(1000); // Espera 1 segundo
        for(int i=0;i<num_bolas;i++){
            
            atualizar_coluna_bit_alterado(canaleta, &posicao_y_inical); // Atualiza a coluna e o bit alterado
            
            exibir_bola_canaleta(canaleta, ssd, posicao_y_inical); // Exibe a bolinha na canaleta
            render_on_display(ssd, frame_area); // Renderiza o display
            
            
        }
    }
}

void registra_bola_canaleta(bola *b,uint8_t *ssd)
{
    canaleta *canaleta=&canaletass[b->canaleta];
    atualizar_coluna_bit_alterado(canaleta, NULL); // Atualiza a coluna e o bit alterado

    exibir_bola_canaleta(canaleta, ssd, b->posicao[2]); // Exibe a bolinha na canaleta
    
    b->registrada=1; // Marca a bolinha como registrada

}

//Verifica qual canaleta a bola caiu e incrementa o contador
void verifica_canaleta(bola *b){
    for(int i=0; i<total_canaletas;i++){
        if(b->posicao[0] >= canaletass[i].coluna_inicial && b->posicao[1] <= canaletass[i].coluna_final){ // Verifica qual canaleta a bolinha caiu 
            canaletass[i].bolas++;
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
    const int H = ssd1306_height; 
    if( b->posicao[2]/8 == (H-9)/8 && b->face & 0xC0){ // Verifica se a colisao é na canaleta
        return 0;
    }
    if(((ssd[b->posicao[0] + (b->posicao[2]/8) * W] & (b->face<<b->v)) //Verifica se colisão na página atual
        || (ssd[b->posicao[1] + (b->posicao[2]/8) * W] & (b->face<<b->v))
        || (ssd[b->posicao[0] + ((b->posicao[2]+8)/8) * W] &  (b->face>>(8-b->v))) // Verifica se colisão na página seguinte
        || (ssd[b->posicao[1] + ((b->posicao[2]+8)/8) * W] & ( b->face>>(8-b->v)))) && b->ativa){ // Verifica se a bolinha não atingiu a canaleta
           
            return 1; // colisão detectada
    }
    return 0; // sem colisão
}

void apagar_bola(uint8_t *ssd, int x0, int x1, int y, uint8_t face) {
    int offset = (y / 8) * ssd1306_width;
    ssd[x0 + offset] &= ~face;
    ssd[x1 + offset] &= ~face;
}

void ajusta_deslocamento_x(int dir_colisao, int *pos_l, int *pos_r) {
    if (dir_colisao == 1) {
        *pos_l = 2;
        *pos_r = 4;
    } else {
        *pos_l = 4;
        *pos_r = 2;
    }
}

void desloca_x(bola *b, int pos_l, int pos_r, int direction) {
    if (direction) {  // esquerda
        b->posicao[0] -= pos_l;
        b->posicao[1] = b->posicao[0] - 1;
        b->dir_colisao = 0;
    } else {  // direita
        b->posicao[0] += pos_r;
        b->posicao[1] = b->posicao[0] + 1;
        b->dir_colisao = 1;
    }
}
//Movimenta a bolinha horizontalmente
void move_bola_x(bola *b, uint8_t *ssd ,int direction)
{
    int dir = b->dir_colisao; // a bola comeca a esquerda do primeiro pino
    int pos_l=0,pos_r=0;
    apagar_bola(ssd,b->posicao[0],b->posicao[1],b->posicao[2],b->face);
    ajusta_deslocamento_x(b->dir_colisao,&pos_l,&pos_r);
    desloca_x(b,pos_l,pos_r,direction);
}

void calcula_endereco_pagina(bola *b,int *addr1,int *addr2, int*addr1_futuro, int* addr2_futuro){
     *addr1=b->posicao[0] + (b->posicao[2]/8) * ssd1306_width; //Cálculo para encontrar o byte da pagina atual
     *addr2=b->posicao[1] + (b->posicao[2]/8) * ssd1306_width;
     *addr1_futuro=b->posicao[0] + ((b->posicao[2]+8)/8) * ssd1306_width; // Cálculo para encontrar o byte da proxima pagina
     *addr2_futuro=b->posicao[1] + ((b->posicao[2]+8)/8) * ssd1306_width;
}

void gerar_mascaras(uint8_t *pag_atual1, uint8_t *pag_atual2, uint8_t *pag_futura1, uint8_t *pag_futura2,
    bola *b, uint8_t *mask_atual1, uint8_t *mask_atual2, uint8_t *mask_futura1, uint8_t *mask_futura2) {
    *mask_atual1 = *pag_atual1 & ~b->face;
    *mask_atual2 = *pag_atual2 & ~b->face;
    *mask_futura1 = *pag_futura1 & ~(b->face >> (8 - b->v));
    *mask_futura2 = *pag_futura2 & ~(b->face >> (8 - b->v));
}




void deslocar_bola(bola * b){
    if(b->face==0x01){// ajusta a face da bolinha para continuar com 2 casas
        b->face=b->face<<1;
        b->face=b->face+1;
    }else{
        b->face=b->face << b->v; //Deslocamento da bolinha com base na velocidade
    }
}


// Movimenta a bolinha no display de acordo com a velocidade
void move_bola_y(bola *b, uint8_t *ssd)
{
    int byte_atual_1,byte_atual_2,byte_futuro_1,byte_futuro_2;
    calcula_endereco_pagina(b,&byte_atual_1,&byte_atual_2,&byte_futuro_1,&byte_futuro_2);

    int posicao=0;     
    uint8_t pag1_atual = ssd[byte_atual_1], pag2_atual = ssd[byte_atual_2]; 
    uint8_t pag1_futura = ssd[byte_futuro_1], pag2_futura = ssd[byte_futuro_2];

    uint8_t m_at1, m_at2, m_fut1, m_fut2;
    gerar_mascaras(&pag1_atual, &pag2_atual, &pag1_futura, &pag2_futura, b, &m_at1, &m_at2, &m_fut1, &m_fut2); // Gera as mascaras para apagar a bolinha na pagina atual e futura


    uint8_t r1 = pag1_atual, r2 = pag2_atual, rf1 = pag1_futura, rf2 = pag2_futura;

    // Desloca circularmente os bits  referentes a construção da bola de acordo com a velocidade
    if(b->face>>(8-b->v) > 0){// Verifica se a bolinha mudou de página
        r1 =  m_at1 | b->face<<b->v;
        r2 =  m_at2 | b->face<<b->v;
        b->face = b->face >> (8-b->v);
        b->troca_pagina=1; // bola trocou de pagina
        b->pag_anterior=b->posicao[2]; // guarda a posição da ultima página 
        b->posicao[2]+=8;
        rf1 =  m_fut1 | b->face;
        rf2 =  m_fut2 | b->face;
                
    }else{
        if(b->troca_pagina){
            apagar_bola(ssd, b->posicao[0], b->posicao[1], b->pag_anterior, b->face << (8 - b->v));
            b->troca_pagina=0;
        }
        

        deslocar_bola(b);
        r1 = m_at1 | b->face;
        r2 = m_at2 | b->face;
    }

    // Verifica se a bola atingiu as canaletas
    if (b->posicao[2] >= ssd1306_height-8) {
        b->ativa=0;
        apagar_bola(ssd, b->posicao[0], b->posicao[1], b->pag_anterior, b->face << (8 - b->v));
        b->v=0; // Para a bolinha 
        
    }else{
        ssd[byte_atual_1] = r1; // Atualiza o display com a nova posição da bolinha
        ssd[byte_atual_2] = r2;
        ssd[byte_futuro_1] = rf1; // Atualiza o display com a nova posição da bolinha
        ssd[byte_futuro_2] = rf2;
    }
    

}

// Função que retorna um número binário aleatório observando o ruido do adc
//Retorna 1 para esquerda e 0 para direita
int binary_choice(){

    uint16_t adc_value = adc_read();
    return (adc_value & 0x01); // Retorna o bit menos significativo do valor lido
}

// Desenha as canaletas na posição central do display 
void desenhar_haste_canaleta(uint8_t *ssd, int coluna, int page) {
    int offset = page * ssd1306_width;
    ssd[offset + coluna] = 0xFF;
}
int calcular_linha_canaleta() {
    return (ssd1306_height - 8) >> 3;  // última linha visível (em página)
}
canaleta inicializar_canaleta(int x, int W) {
    canaleta c;
    c.coluna_inicial = x;
    c.coluna_final = x + 8;
    c.coluna_somada = (x > W / 2) ? x : x + 8;
    c.bolas = 0;
    int tamanho = c.coluna_final - c.coluna_inicial;
    c.bit_alterado = (int*)malloc(sizeof(int) * tamanho);
    memset(c.bit_alterado, 0, sizeof(int) * tamanho);
    return c;
}

void draw_canaleta(uint8_t *ssd) {
    const int W = ssd1306_width;
    int page = calcular_linha_canaleta();

    canaletass = (canaleta*)malloc(sizeof(canaleta));
    int index = 0;

    for (int x = W/4 + 4; x < W - W/4; x += 8) {
        canaletass = realloc(canaletass, sizeof(canaleta) * (index + 1));
        canaletass[index] = inicializar_canaleta(x, W);
        desenhar_haste_canaleta(ssd, x, page);
        index++;
    }

    total_canaletas = index - 1;
}

uint8_t gerar_mask_vertical(int y) {
    return 1 << (y & 0x7);  // qual bit dentro do byte
}

int calcular_page_offset(int y) {
    return (y >> 3) * ssd1306_width;  // qual página * largura
}

void draw_pins(uint8_t *ssd) {
    const int W = ssd1306_width;
    const int H = ssd1306_height;
    int pin_count = 0;
    int start_x = W / 2;

    for (int y = 8; y < H - 8; y += 8) {
        pin_count++;
        for (int j = 0; j < pin_count; j++) {
            int x = start_x + (j << 3);  // j * 8 colunas
            int offset = calcular_page_offset(y);
            uint8_t mask = gerar_mask_vertical(y);
            ssd[offset + x] |= mask;
        }
        start_x -= 4;  // desloca os pinos para formar o triângulo
    }
}




void inicializar_bolas(bola * bolas,int * posicao_inicial,int num_bolas){
    bola b;
    b.v = 2;
    b.posicao[0] = posicao_inicial[0]; 
    b.posicao[1] = posicao_inicial[1]; 
    b.posicao[2] = posicao_inicial[2];
    b.face = 0x03; //preenchimento dos dois primeiros bits de cada coluna
    b.canaleta=0;
    b.registrada=0;
    b.dir_colisao=1; // Bola começa a cair pela esquerda
    b.pag_anterior=b.posicao[2]; // Pagina anterior da bolinha
    b.troca_pagina=0; // Verifica se a bolinha trocou de pagina
    b.ativa=1;
    for(int i=0;i<num_bolas;i++){
        bolas[i]=b; 
    }


}



void atualizar_bolinha(bola *b, uint8_t *ssd, struct render_area *frame_area, int *primeira_bola) {
    if (b->v == 0) {
        char contagem[5];
        sprintf(contagem, "%d", *primeira_bola+1);
        ssd1306_draw_string(ssd, 5, 2, contagem);

        verifica_canaleta(b);
        registra_bola_canaleta(b, ssd);
        render_on_display(ssd, frame_area);
        (*primeira_bola)++;
        return;
    }

    if (colisao(b, ssd)) {
        move_bola_x(b, ssd, binary_choice());
        move_bola_y(b, ssd);
    } else {
        move_bola_y(b, ssd);
    }
}

// Trata a colisão inicial impedindo que diversas bolas ocupem o mesmo espaço
bool funil(bola *b, uint8_t *ssd, int *posicao_inicial) {
    if(b->posicao[2]!=0){
        return false;
    }
    return ((ssd[posicao_inicial[0] + (posicao_inicial[2]/8) * ssd1306_width] & (b->face << b->v)) 
           || (ssd[posicao_inicial[1] + (posicao_inicial[2]/8) * ssd1306_width] & (b->face << b->v)) 
           || (ssd[posicao_inicial[0] + (posicao_inicial[2]/8) * ssd1306_width] & (b->face << (b->v)*2)) 
           || (ssd[posicao_inicial[1] + (posicao_inicial[2]/8) * ssd1306_width] & (b->face << (b->v)*2))); 
}


bool tratar_botao(uint8_t *ssd, struct render_area *frame_area, int *tela_distribuicao, bola *bolas, int num_bolas, int *primeira_bola, int *bola_atual,int *posicao_inicial) {
    if (gpio_get(BOTAO_A) == 0 && !(*tela_distribuicao)) {
        memset(ssd, 0, ssd1306_buffer_length);
        exibir_distribuicao(ssd, frame_area);
        *tela_distribuicao = true;
        return true;
    }

    if (gpio_get(BOTAO_A) == 0 && *tela_distribuicao) {
        memset(ssd, 0, ssd1306_buffer_length);
        reset(bolas,num_bolas,ssd,posicao_inicial);
        *tela_distribuicao = false;
        *primeira_bola = 0;
        *bola_atual = 0;
        return true;
    }

    return false;
}
