#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <string.h>
#include "hardware/pwm.h"

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 2000
#define ADC_SAMPLE_RATE  (48000000 / ADC_CLOCK_DIV)
#define SAMPLES ((int)(ADC_SAMPLE_RATE / 4)) // 0,25 segundos de amostras
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f/5.f) // Intervalos de volume do microfone.

//BUZZERS
#define BUZZER_A 21 // Pino para o buzzer A
#define PWM_WRAP 4095

// LED RGB
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13

//BOTÔES
#define BUTTON_A 5
#define BUTTON_B 6

#define RECORD_TIME 5 // Tempo de gravação em segundos
#define TOTAL_SAMPLES (RECORD_TIME * ADC_SAMPLE_RATE) // Total de amostras a serem gravadas
// Canal e configurações do DMA
uint dma_channel;
dma_channel_config dma_cfg;



uint16_t buffer[TOTAL_SAMPLES]; // Buffer A

volatile int posicao = 0; // Posição atual no buffer

enum Flags{
    FLAG_BUFFER = 1 << 0,
    FLAG_RECORD = 1 << 1,
    FLAG_DMA = 1 << 2,
    FLAG_TRATAMENTO = 1 << 3,
    FLAG_BUZZER = 1 << 4
};

volatile uint8_t status = 0;

void init_record(); //captura os dados do adc e 
void init_adc();
void init_dma();
void init_rgb();
void init_buttons();
void gpio_callback(uint gpio, uint32_t events);
int64_t callback_timer(alarm_id_t id, void *user_data);
void dma_handler(); 
void init_pwm_buzzers();

// usar pwm buzzer
// fazer a exibicao da onda
// iniciar construcao pwm
    int i = 0;
int main()
{
    stdio_init_all();

    init_rgb();
    init_buttons();
    init_adc();
    init_dma();
    init_pwm_buzzers();

    volatile int posicao_atual = 0;

    while (true) {
        if(status & FLAG_RECORD){

            posicao_atual = 0;
            init_record();
        } 
        // enquanto grava processa os dados
        if(status & FLAG_BUFFER){

            status &= ~FLAG_BUFFER;

         
        }

        if(status & FLAG_BUZZER){
            gpio_put(LED_BLUE, true); // Acende o LED azul
            // Aciona o buzzer
            int delay_us = 1000000 / ADC_SAMPLE_RATE;
            for(int i = 0; i<TOTAL_SAMPLES;i++){
                // Converte o valor do ADC para um nível de PWM
                pwm_set_gpio_level(BUZZER_A, buffer[i]);
                sleep_us(delay_us); // Aguarda 1ms para simular a frequência do buzzer
               
            }
            pwm_set_gpio_level(BUZZER_A, 0); // Desliga o PWM após a reprodução
            status &= ~FLAG_BUZZER;
            gpio_put(LED_BLUE, false); // Acende o LED azul
        }

           
    }
}

void init_adc(){
    //Configura o ADC
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(MIC_CHANNEL);


    adc_fifo_setup(
        true, // Habilitar FIFO
        true, // Habilitar request de dados do DMA
        1, // Threshold para ativar request DMA é 1 leitura do ADC
        false, // Não usar bit de erro
        false // Não fazer downscale das amostras para 8-bits, manter 12-bits.
    );

    adc_set_clkdiv(ADC_CLOCK_DIV);
}

void init_buttons(){
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    //Configura a interrupcao do botao A para o timer
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
}

void init_rgb(){
    //Inicialização do LED RGB
    gpio_init(LED_BLUE);
    gpio_init(LED_GREEN);
    gpio_init(LED_RED);
    gpio_set_dir(LED_BLUE,GPIO_OUT);
    gpio_set_dir(LED_GREEN,GPIO_OUT);
    gpio_set_dir(LED_RED,GPIO_OUT);
}

void init_dma(){
    // Tomando posse de canal do DMA.
    dma_channel = dma_claim_unused_channel(true);
    // Configurações do DMA.
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16); // Tamanho da transferência é 16-bits (usamos uint16_t para armazenar valores do ADC)
    channel_config_set_read_increment(&dma_cfg, false); // Desabilita incremento do ponteiro de leitura (lemos de um único registrador)
    channel_config_set_write_increment(&dma_cfg, true); // Habilita incremento do ponteiro de escrita (escrevemos em um array/buffer)
    channel_config_set_dreq(&dma_cfg, DREQ_ADC); // Usamos a requisição de dados do ADC
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

}

void init_record() {
    
    adc_fifo_drain(); // Limpa o FIFO do ADC.
    adc_run(false); // Desliga o ADC (se estiver ligado) para configurar o DMA.

    status |= FLAG_DMA;
    dma_channel_configure(dma_channel, &dma_cfg,
        buffer, // Escreve no buffer.
        &(adc_hw->fifo), // Lê do ADC.
        SAMPLES, // Faz "SAMPLES" amostras.
        true // Liga o DMA.
    );
    irq_set_enabled(DMA_IRQ_0, true); 
    // Liga o ADC e espera acabar a leitura.
    adc_run(true);
    

}

void gpio_callback(uint gpio, uint32_t events) {

    if(!(status & FLAG_RECORD)){
        gpio_put(LED_RED,true);
        gpio_put(LED_BLUE, false); // Apaga o LED azul
        add_alarm_in_ms(RECORD_TIME*1000, callback_timer, NULL, false);
        status |= FLAG_RECORD;
        
    }
    
}

int64_t callback_timer(alarm_id_t id, void *user_data) {
    
    status &= ~FLAG_RECORD;
    status &= ~FLAG_DMA; 
    gpio_put(LED_RED,false);
    adc_run(false);
    dma_channel_abort(dma_channel);
    dma_hw->ints0 = 1u << dma_channel;  // limpa a flag de interrupção pendente
    irq_set_enabled(DMA_IRQ_0, false);  // desativa a interrupção
    
    
    //verificar se há dados para processar
    status |= FLAG_BUZZER;
    printf("Tempo esgotado!\n");
    return 0; // 0 = não repetir
}

void dma_handler() {
    if (!(status & FLAG_RECORD)){
        dma_hw->ints0 = 1u << dma_channel; // limpa a flag de interrupção    
        return;
    } 
    
    // Verifica se a interrupção veio do canal específico
    if (dma_hw->ints0 & (1u << dma_channel)) {
        dma_hw->ints0 = 1u << dma_channel; // limpa a flag de interrupção    
        // Aqui entra sua lógica: processar, copiar, iniciar nova transferência...
        status |= FLAG_BUFFER;
        dma_channel_set_write_addr(dma_channel, buffer+posicao, true);
        posicao += SAMPLES;
        if(posicao >= TOTAL_SAMPLES){
            posicao = 0;
        }


    }
}

void init_pwm_buzzers(){
    float clkdiv = 125000000.0f / (20000 * (PWM_WRAP + 1));
    // Inicializa os pinos dos buzzers
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_A);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clkdiv); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_wrap(slice_num, PWM_WRAP); // Define o valor máximo do contador
    pwm_set_gpio_level(BUZZER_A, 0); // Desliga o PWM inicialmente
}



